# Temperature sensor occasionally publishes 'nan' over MQTT

Analysis: 2026-07-26
Issue: [#8 — Temperature sensor occasionally publishes 'nan' over MQTT, causing HA sensor errors](https://github.com/CybDis/TTGO-T-HIGrow/issues/8)

**Resolved in 5.5.7** with option 1 below (guard at the sink).

## Symptom

Home Assistant intermittently rejects the temperature state update with:

```
Value error while updating state of sensor.higrow6_temperature, topic:
'aha/4417938f3b8c/TTGO_4417938f3b8c_temp/stat_t' with payload: b'nan':
Sensor sensor.higrow6_temperature has device class 'temperature', state
class 'measurement' unit '°C' and suggested precision 'None' thus
indicating it has a numeric value; however, it has the non-finite
value: 'nan'
```

Observed on at least three different physical devices (`4417938f3b8c`,
`083af2660b5c`, `2cbcbba80834`), intermittently rather than on one specific
unit. The entity recovers automatically on the next cycle.

## Root cause

Default board configuration uses the onboard DHT sensor
(`include/user-variables.h:301-306`, `dht_found = true`, `DHT_TYPE DHT11`).

In `src/main.cpp:433-440` (`setup()`):

```cpp
if (dht_found)
{
  float t12 = dht.readTemperature();
  config.temp = t12;
  delay(2000);
  float h12 = dht.readHumidity();
  config.humid = h12;
}
```

The Adafruit DHT sensor library returns `NAN` — not a previous/cached
value, not an error code — whenever the underlying bit-bang `read()` fails
(`.pio/libdeps/esp32dev/DHT sensor library/DHT.cpp:85-124`):

```cpp
float DHT::readTemperature(bool S, bool force) {
  float f = NAN;
  if (read(force)) {
    ...              // f only gets a real value on success
  }
  return f;
}
```

`read()` fails on a start-signal timeout, a per-bit pulse timeout, or a
checksum mismatch (`DHT.cpp:289-353`) — all of which can happen on a
single bad transaction of the timing-critical one-wire protocol.
`readTemperature()` and `readHumidity()` both trigger the same underlying
`read()`, so a failed transaction produces `NAN` for **both** fields in the
same cycle.

`config.temp` (and `config.humid`) is a plain `float` with no validity
flag. The `NAN` is stored unconditionally and flows straight through to
MQTT publish with no check anywhere in between:

```cpp
// src/home-assistant-discovery.h:24-30
String formatSensorValue(float value, bool isInteger = false) {
  if (isInteger) {
    return String((int)value);
  } else {
    return String(value, 1);   // String(NAN, 1) == "nan" on the ESP32 core
  }
}
...
sensorTemp->setValue(formatSensorValue(config.temp).c_str());   // line 282
mqttLog(sensorTemp, formatSensorValue(config.temp));            // line 314
```

`String(float, decimals)` on the ESP32 Arduino core formats via
`dtostrf`/`vsnprintf`, which render a non-finite float as the literal
string `"nan"` — exactly the payload Home Assistant logs. There is no
`isnan()` guard anywhere in the codebase (`grep -rn isnan src/` is empty).

### Why it's intermittent and hits multiple devices

The DHT read happens *after* `connectToNetwork()` and the NTP sync loop
(`main.cpp:309-373`), i.e. while the WiFi radio is actively associated and
transmitting. This project has already diagnosed and fixed the same class
of problem for the analog soil sensor
([doc/2026-07-11-soil-battery-sag-analysis.md](2026-07-11-soil-battery-sag-analysis.md)):
WiFi TX bursts cause supply-rail sag and timing/interrupt jitter that
disturb sensitive, timing-critical readings taken while the radio is on.
The DHT one-wire protocol is exactly this kind of timing-critical read
(microsecond-scale pulse widths, `InterruptLock` only disables interrupts
on the current core and does not stop WiFi-driven electrical noise on the
supply rail). This explains why the failure is sporadic and not tied to
one unit: any device can hit a bad transaction when a WiFi TX burst lands
during the ~4-5 ms one-wire transaction.

### Secondary, currently dormant risk

`bmp.readTemperature()` (Adafruit BME280 path, `main.cpp:442-452`, only
active when `bme_found`) writes into the same unguarded `config.temp` /
`formatSensorValue()` sink. Any future failure mode of that sensor that
returns `NAN` would reproduce the identical symptom. The fix should guard
the sink, not just the DHT call site.

## Possible fixes

1. **Guard at the sink (recommended, minimal, covers all sensors):** in
   `formatSensorValue()`, check `isnan(value)` and signal "no valid
   reading" instead of formatting it — e.g. return an empty string, and
   have the caller skip `setValue()`/`mqttLog()` for that sensor this
   cycle, leaving the previous retained MQTT value in place (this is the
   behavior the issue explicitly asks for). Cheapest, single choke point,
   also protects `config.humid`, `config.lux`, `config.pressure`, etc.
   against the same class of bug with no per-sensor code changes.

2. **Guard at the read site:** after `dht.readTemperature()` /
   `dht.readHumidity()`, check `isnan()` and keep the previous
   `config.temp`/`config.humid` (or a documented sentinel) instead of
   overwriting it. More localized, but doesn't protect other sensors added
   later against the same sink defect, and needs to be duplicated for the
   BME280 path.

3. **Retry once on failure (implemented, `src/main.cpp:433-451`):** since a
   single bad one-wire transaction is the trigger, a re-read after a `NAN`
   result recovers most of the time without any behavior change
   downstream. The retry uses `force=true` to bypass the DHT library's
   `MIN_INTERVAL` throttle (otherwise it would just return the same cached
   failed result), preceded by a `delay(300)` to give the sensor some time
   to settle before the retry — shorter than the ~1-2 s the datasheet
   recommends between transactions, a deliberate trade-off to keep
   `setup()` runtime short. This is a defense-in-depth measure, not a
   replacement — an `isnan()` guard at the sink (option 1) is still needed
   for the case where the retry also fails.

4. **Move the DHT read earlier, before `connectToNetwork()`:** mirrors the
   soil/salt fix in 5.4.1 and would reduce *how often* WiFi-induced
   transaction failures occur in the first place, but does not eliminate
   the possibility of a `NAN` (checksum failures/timeouts can happen for
   other reasons, e.g. a marginal sensor or wiring), so it does not remove
   the need for a validity check — it only lowers the failure rate.

   **Decision: not implemented (deliberate).** Options 1 and 3 already
   guard and mitigate the failure; reordering `setup()` for a further
   reduction in failure rate was judged not worth the added complexity/risk
   of touching the boot sequence. Left as a known, accepted possible
   follow-up, not a gap.

Option 1 directly satisfies the behavior requested in the issue ("skip
publishing that reading for the cycle, leave the previous state topic
value in place") and closes the gap for every current and future sensor
in one place. Options 3 and 4 are worthwhile complements to reduce how
often the guard has to trigger, but neither is a substitute for it.

## Verification (once fixed)

Watch the MQTT `.../temp/stat_t` and `.../humid/stat_t` topics (or the HA
logbook for `sensor.higrowN_temperature`) across the previously affected
devices (`higrow6`, `higrow1`, `higrow32`) for a few days: the HA "non-finite
value" error should no longer appear, and on a cycle where the DHT
transaction fails, the entity should simply retain its last value instead
of erroring out.
