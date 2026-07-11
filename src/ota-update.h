// Pull-OTA via Home Assistant /local (see doc/2026-07-10-ota-update-design.md)
// Called right after WiFi connect, before NTP/measurement/MQTT. Never blocks the
// normal cycle: any error just returns and the device continues as usual.
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

struct OtaManifest
{
  String version;
  String file;
  String md5;
  size_t size;
};

bool otaParseManifest(const String &json, OtaManifest &out)
{
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err)
  {
    return false;
  }
  out.version = doc["version"] | "";
  out.file = doc["file"] | "";
  out.md5 = doc["md5"] | "";
  out.size = doc["size"] | 0;
  return out.version.length() > 0 && out.file.length() > 0 && out.md5.length() == 32 && out.size > 0;
}

// Deliberately "!=" instead of ">" so putting an older version into the
// manifest rolls devices back (downgrade support).
bool otaUpdateRequired(const String &manifestVersion, const String &currentRel)
{
  if (manifestVersion.length() == 0)
  {
    return false;
  }
  return manifestVersion != currentRel;
}

void checkForOtaUpdate(const String &currentRel)
{
  if (strlen(otaBaseUrl) == 0)
  {
    return;
  }

  Serial.println(F("\n  [OTA] Checking for firmware update..."));

  // HA serves /local via SSL; no cert validation on the ESP32 (local network),
  // firmware integrity is ensured by the manifest MD5 check instead.
  WiFiClientSecure secureClient;
  bool useTls = String(otaBaseUrl).startsWith("https");
  if (useTls)
  {
    secureClient.setInsecure();
  }

  HTTPClient http;
  http.setConnectTimeout(3000);
  http.setTimeout(5000);

  String manifestUrl = String(otaBaseUrl) + "/manifest.json";
  if (!(useTls ? http.begin(secureClient, manifestUrl) : http.begin(manifestUrl)))
  {
    Serial.println(F("  [OTA] Invalid manifest URL - skipping"));
    return;
  }
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    Serial.print(F("  [OTA] Manifest not available (HTTP "));
    Serial.print(httpCode);
    Serial.println(F(") - skipping"));
    http.end();
    return;
  }
  String payload = http.getString();
  http.end();

  OtaManifest manifest;
  if (!otaParseManifest(payload, manifest))
  {
    Serial.println(F("  [OTA] Invalid manifest content - skipping"));
    return;
  }

  if (!otaUpdateRequired(manifest.version, currentRel))
  {
    Serial.print(F("  [OTA] Firmware up to date ("));
    Serial.print(currentRel);
    Serial.println(F(")"));
    return;
  }

  Serial.print(F("  [OTA] Update available: "));
  Serial.print(currentRel);
  Serial.print(F(" -> "));
  Serial.println(manifest.version);

  String firmwareUrl = String(otaBaseUrl) + "/" + manifest.file;
  if (!(useTls ? http.begin(secureClient, firmwareUrl) : http.begin(firmwareUrl)))
  {
    Serial.println(F("  [OTA] Invalid firmware URL - skipping"));
    return;
  }
  http.setTimeout(10000); // download read timeout, firmware is ~1 MB
  httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    Serial.print(F("  [OTA] Firmware download failed (HTTP "));
    Serial.print(httpCode);
    Serial.println(F(") - skipping"));
    http.end();
    return;
  }

  if (!Update.begin(manifest.size))
  {
    Serial.print(F("  [OTA] Update.begin failed: "));
    Update.printError(Serial);
    http.end();
    return;
  }
  Update.setMD5(manifest.md5.c_str());
  Update.onProgress([](size_t done, size_t total) {
    static int lastReported = -1;
    int percent = (total > 0) ? (int)((done * 100) / total) : 0;
    if (percent / 10 > lastReported)
    {
      lastReported = percent / 10;
      Serial.print(F("  [OTA] Progress: "));
      Serial.print(percent);
      Serial.println(F("%"));
    }
  });

  unsigned long flashStart = millis();
  size_t written = Update.writeStream(http.getStream());
  http.end();

  if (written != manifest.size)
  {
    Serial.print(F("  [OTA] Incomplete download ("));
    Serial.print(written);
    Serial.print(F("/"));
    Serial.print(manifest.size);
    Serial.println(F(" bytes) - aborting, keeping current firmware"));
    Update.abort();
    return;
  }

  if (!Update.end())
  {
    // Covers MD5 mismatch as well - old slot stays active.
    Serial.print(F("  [OTA] Update failed: "));
    Update.printError(Serial);
    Update.abort();
    return;
  }

  Serial.print(F("  [OTA] SUCCESS after "));
  Serial.print(millis() - flashStart);
  Serial.println(F("ms - rebooting into new firmware"));
  Serial.flush();
  ESP.restart();
}
