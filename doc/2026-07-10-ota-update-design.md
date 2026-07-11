# Design: OTA-Update über Home Assistant `/local` (Pull-OTA)

Datum: 2026-07-10
Status: Umgesetzt in Release 5.4.0 (2026-07-11)

## Ziel

Die TTGO-T-HiGrow-Sensoren sollen neue Firmware ohne USB-Kabel erhalten. Die
Firmware liegt lokal auf dem Home Assistant (kein Internet-Hosting). Da die
Geräte fast immer im Deep Sleep sind, prüft jeder Sensor beim Aufwachen selbst
(Pull-Prinzip), ob eine neue Version bereitliegt.

## Entscheidungen (mit Nutzer abgestimmt)

- **Trigger:** Prüfung bei jedem Aufwachen (1×/h, im Lademodus alle 5 min).
- **Rollout:** Eine gemeinsame Firmware für alle Sensoren. Gerätespezifisches
  (Pflanzenname, Boden-Kalibrierung, Batterie-Info) liegt im SPIFFS und
  überlebt das Update.
- **Zeitpunkt im Zyklus:** Direkt nach dem WLAN-Connect, **vor** NTP, Messung
  und MQTT. Im Update-Fall wird sofort geflasht und rebootet; Messung und
  Publish macht bereits die neue Firmware. So ist eine neue Version ohne
  1-h-Verzögerung aktiv und Tests ziehen sich nicht in die Länge.
- **Versionsvergleich:** Manifest-Version ≠ eigene `rel` ⇒ Update. Bewusst
  „ungleich" statt „größer", damit Downgrades durch Zurücklegen einer alten
  Version möglich sind.
- **Deployment:** Automatisiert per PlatformIO-Target (`pio run -t ota_deploy`).

## Architektur

### 1. Ablage auf Home Assistant

Verzeichnis `config/www/higrow/` auf dem HA:

- `firmware.bin` — aktuelles Build
- `manifest.json`:

```json
{
  "version": "5.4.0",
  "file": "firmware.bin",
  "md5": "<md5-hex>",
  "size": 123456
}
```

HA liefert `config/www` unter `https://<ha-ip>:8123/local/higrow/…` ohne
Authentifizierung aus — der ESP32 braucht nur HTTP-GET, keinen Token. Die
HA-Basis-URL/IP steht als `otaBaseUrl` in `user-variables.h`.

**SSL-Hinweis (5.4.0):** Diese HA-Instanz bedient Port 8123 nur per HTTPS.
Der ESP32 nutzt daher `WiFiClientSecure` mit `setInsecure()` (keine
Zertifikatsprüfung im lokalen Netz); die Integrität der Firmware sichert die
MD5-Prüfung aus dem Manifest.

### 2. Update-Logik auf dem Sensor (`src/ota-update.h`)

Aufruf in `setup()` unmittelbar nach `connectToNetwork()`:

1. HTTP-GET `manifest.json` (Timeout ~3 s). Fehler/nicht erreichbar ⇒ Funktion
   kehrt zurück, normaler Zyklus läuft weiter. OTA ist nie ein Blocker.
2. Manifest parsen (ArduinoJson, ist bereits Dependency). Version mit `rel`
   vergleichen; gleich ⇒ zurück zum normalen Zyklus.
3. Bei Abweichung: `firmware.bin` per HTTP streamen und mit der Arduino-
   `Update`-Library in den inaktiven OTA-Slot schreiben. `Update.setMD5()` mit
   dem Manifest-Wert, Größe aus dem Manifest. Serielle Fortschrittsausgabe.
4. `Update.end()` erfolgreich ⇒ `ESP.restart()`. Der Reboot startet die neue
   Firmware, die dann Messung, MQTT-Publish (inkl. neuer `rel` über den
   bestehenden Release-Sensor) und Deep Sleep übernimmt.

SPIFFS bleibt bei OTA unangetastet (nur die App-Partition wird getauscht);
`/name.conf`, `/soil.conf`, `/batinfo.conf` bleiben erhalten.

### 3. Partitionsschema

OTA braucht zwei App-Slots. Das Standard-Schema von `esp32dev` (default.csv)
hat 2 × 1.310.720 Bytes App-Slots. **Ergebnis der Prüfung (5.4.0):** Das Build
ist mit OTA-Code 1.176.845 Bytes (89,8 % des Slots; HTTPClient zieht mbedTLS
mit) und passt — kein Wechsel auf `min_spiffs.csv` nötig, SPIFFS bleibt
unverändert erhalten. Achtung: nur noch ~130 KB Luft. Wächst das Build über
das Slot-Limit, muss auf `board_build.partitions = min_spiffs.csv` umgestellt
werden (dann geht der SPIFFS-Inhalt einmalig verloren und alle Geräte müssen
per USB neu geflasht werden).

**Migration:** Das OTA-Grundupdate erfordert einmalig ein letztes USB-Flashen
aller Geräte (`pio run -t upload`). Da kein Partitionswechsel nötig ist,
bleiben `/name.conf`, `/soil.conf` und `/batinfo.conf` dabei erhalten.

### 4. Deployment: PlatformIO-Target `ota_deploy`

In `platformio_extra.py`, analog zum bestehenden `reset`-Target:

1. Baut die Firmware (Abhängigkeit auf das Build-Target).
2. Liest die Version aus `src/main.cpp` (`const String rel = "…"`).
3. Berechnet MD5 und Größe von `firmware.bin`, schreibt `manifest.json`.
4. Kopiert beides per `scp` (laufendes Terminal-&-SSH-Addon) nach
   `/config/www/higrow/` auf dem HA. Zielhost/Pfad als Variablen am Anfang des
   Skripts. Samba-Share als dokumentierte manuelle Alternative.

Reihenfolge beim Kopieren: erst `firmware.bin`, dann `manifest.json` — so
zeigt das Manifest nie auf eine noch unvollständige Binärdatei.

### 5. Bedienung (Stand 5.4.0)

- **Neue Version ausrollen:** Version in `src/main.cpp` (`const String rel`)
  hochzählen, dann `pio run -t ota_deploy`. Das Target baut, erzeugt
  `manifest.json` (Version, MD5, Größe) und kopiert beides per `scp` nach
  `root@192.168.1.6:/config/www/higrow/` (Variablen `OTA_HOST`, `OTA_PORT`,
  `OTA_REMOTE_DIR` am Anfang von `platformio_extra.py`).
- **Manuelle Alternative:** `firmware.bin` und `manifest.json` aus
  `.pio/build/esp32dev/` über den Samba-Share nach `config/www/higrow/`
  kopieren (gleiche Reihenfolge: erst Binärdatei, dann Manifest).
- **OTA deaktivieren:** `otaBaseUrl` in `include/user-variables.h` leer setzen.
- **Kontrolle:** `curl -k https://192.168.1.6:8123/local/higrow/manifest.json`
  bzw. der Release-Sensor in Home Assistant nach dem nächsten Aufwachen.

## Fehlerfälle

| Fall | Verhalten |
|---|---|
| Manifest nicht erreichbar / ungültiges JSON | Normaler Zyklus, Sleep wie gehabt |
| Download bricht ab / MD5 falsch | `Update.abort()`, alter Slot bleibt aktiv, nächster Versuch beim nächsten Aufwachen |
| Neue Firmware bootet, crasht aber | Restrisiko: kein automatisches Rollback. Absicherung organisatorisch: jede Version zuerst per USB an einem Gerät testen, dann ins Manifest legen. Echtes esp_ota-Rollback ist spätere Härtung |

## Tests

- Unit-artige Prüfung der Manifest-Parse-/Versionsvergleichslogik (soweit ohne
  Hardware sinnvoll; Logik dafür in kleine, testbare Funktionen fassen).
- Manuell auf Hardware: (1) kein Manifest ⇒ normaler Zyklus; (2) gleiche
  Version ⇒ kein Update; (3) neue Version ⇒ Flash + Reboot + neue `rel` in HA;
  (4) absichtlich falsche MD5 ⇒ Abbruch, Gerät läuft mit alter Version weiter.
- Serieller Monitor als primäres Kontrollwerkzeug (bestehende Debug-Ausgaben).

## Verworfene Alternativen

- **ArduinoOTA (Push):** Gerät ist fast immer im Deep Sleep, nicht erreichbar.
- **MQTT-getriggertes Update** (retained Topic + HA-Button): mächtiger, aber
  mehr bewegliche Teile; bei stündlichem Auto-Check unnötig.
- **HA-Update-Entity** („Update verfügbar" in HA): optionaler späterer Ausbau,
  bewusst weggelassen (YAGNI).
