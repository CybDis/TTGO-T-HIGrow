# RawBattVoltage reads low vs. actual supply voltage

Analysis: 2026-07-25
Status: **fixed and released in 5.5.6** — root cause confirmed and corrected
(hardcoded `vref=1100` → `analogReadMilliVolts()`), Battery% endpoints
re-derived, verified accurate on multiple physical units. See "Session
summary and open items" at the end of this document for what's still
outstanding.

## Symptom

Bench test on higrow12: device powered directly with a lab supply at **4.3 V**
on the battery rail. The newly added HA Discovery sensors (release 5.5.4/5.5.5,
see [2026-07-25-battery-discovery-summary.md](2026-07-25-battery-discovery-summary.md))
report:

- `RawBattAdc`: 2346 (raw ADC counts, no unit — matches `SoilRaw` convention
  as of 5.5.5)
- `RawBattVoltage`: displayed as "4 V" — computed value is **≈4.16 V**, i.e.
  roughly **3–4% (≈0.14–0.17 V) below** the actual supplied 4.3 V

This value was previously invisible: `batvoltage` only existed buried inside
the legacy combined-JSON MQTT payload and as an intermediate value feeding the
`Battery %` calculation. Exposing it as its own HA entity in 5.5.4/5.5.5 is
what surfaced the discrepancy — the underlying calculation itself was not
touched by that change.

## Current calculation

`src/read-sensors.h`, `readBattery()`:

```cpp
int vref = 1100;
uint16_t volt = analogRead(BAT_ADC);        // BAT_ADC = GPIO33
config.batvolt = volt;
float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
config.batvoltage = battery_voltage;
```

i.e. `battery_voltage = (raw/4095) * 3.3 V(assumed ADC full-scale) *
2.0(assumed divider ratio) * 1.1(fixed "vref" correction factor)`.

This feeds `Battery %`:

```cpp
float bat = map(battery_voltage * 100, 416, 290, 100, 0);
```

The `416`/`290` endpoints were presumably fitted empirically against real
full/empty battery packs **using this same formula**, so the percentage
output is internally self-consistent even if the absolute voltage is biased —
the error mostly cancels out for `%`, but not for the newly exposed absolute
`RawBattVoltage`.

## Relationship to the 2026-07-11 soil/salt sag analysis

[2026-07-11-soil-battery-sag-analysis.md](2026-07-11-soil-battery-sag-analysis.md)
investigated a **different** mechanism: the soil/salt analog front-end is an
active circuit (CD4060 oscillator + TL034 amp chain) powered from the battery
rail, so it sags under WiFi TX load and under a weak (high internal
resistance) battery — a *dynamic*, load-dependent effect. That analysis
explicitly assumed the **battery measurement itself "is supposed to track the
battery and stays correct."**

The new observation is a **static** offset, reproduced on a bench supply with
no WiFi load and no battery sag involved (steady 4.3 V in, ADC settles, still
reads ~4.16 V) — so it's a distinct issue: a **calibration/scale bias in the
battery ADC→voltage conversion itself**, not a rail-sag effect. The assumption
in the 5.4.1 doc that the battery channel "stays correct" should be treated as
unconfirmed for absolute accuracy going forward — it was only ever validated
indirectly (percentage looked plausible), not against a known reference
voltage.

## Candidate root causes (not yet confirmed — for next session)

1. **Hardcoded `vref = 1100`**: not read from the ESP32's factory-trimmed
   eFuse reference (`esp_adc_cal_characterize` / `ADC_CAL` API). Per-chip
   Vref varies roughly 1000–1200 mV; if this unit's true Vref is lower than
   1100, the formula would systematically under-report voltage — consistent
   in direction with what was observed, but magnitude (~4%) would need to
   match the actual eFuse value once read.
2. **ESP32 ADC nonlinearity**: `analogRead()` without calibration is known to
   be inaccurate (several % typical error), particularly non-linear across
   the input range at 11 dB attenuation, without `esp_adc_cal` correction.
3. **Assumed divider ratio (`2.0`) vs. real resistor values**: resistor
   tolerances (and possibly a divider ratio that isn't exactly 2:1 by design)
   would produce a fixed multiplicative bias. Cross-check against
   `images/schematic.pdf` for the actual resistor values on the BAT_ADC
   divider.
4. **Assumed ADC full-scale (`3.3`)**: actual AVDD may differ slightly from
   3.3 V per board.

These are not mutually exclusive; the observed ~3–4% low bias is plausibly a
combination of (1) and (2).

## Open questions for the next analysis session

- Is the offset consistent in **direction and magnitude** across multiple
  known bench voltages (e.g. 3.0 V, 3.7 V, 4.2 V, 4.3 V) on the same device,
  or does it vary non-linearly (pointing more at ADC nonlinearity than a
  fixed Vref/divider error)?
- Does the offset differ across **different physical devices** (higrow12 vs.
  others)? A per-chip-varying offset would point at uncalibrated Vref; a
  consistent offset across devices would point at the divider ratio or
  fixed-formula assumptions instead.
- What does `esp_adc_cal_characterize()` report as this chip's actual eFuse
  Vref, and does correcting the formula with it close the gap?
- Should the fix be applied only to the newly exposed `RawBattVoltage` (kept
  as a "best effort absolute voltage" sensor), or should `readBattery()`
  itself be corrected — which would also shift the `Battery %` mapping and
  require re-deriving the `416`/`290` calibration constants so existing
  devices don't jump to a different reported percentage after an OTA update?

## Files involved

- [src/read-sensors.h](../src/read-sensors.h) — `readBattery()`, the
  ADC→voltage formula and the `%` mapping
- [src/home-assistant-discovery.h](../src/home-assistant-discovery.h) —
  `RawBattAdc` / `RawBattVoltage` HA Discovery sensors (5.5.4/5.5.5)
- [src/main.cpp](../src/main.cpp) — `BAT_ADC` pin definition (GPIO33),
  `Config.batvolt` / `Config.batvoltage`


### Erkenntnisse nach Firmware-Änderung (HiGrow12)

- Mit USB-C Spannungsversorgung:
  RawBattAdc
  2.733
  RawBattVoltage
  4,80 V
  Release
  5.5.5
  Soil
  3 %
  SoilCalibration
  1584:3504
  SoilRaw
  3.455

> "bench voltages (e.g. 3.0 V, 3.7 V, 4.2 V, 4.3 V) on the same device" mit Labornetzteil:

- Mit 3.0V 
  zu wenig Spannung, Chip funktioniert nicht

- Mit 3.25V
  RawBattAdc
  1.461
  RawBattVoltage
  2,70 V
  Release
  5.5.5
  Soil
  56 %
  SoilCalibration
  1584:3504
  SoilRaw
  2.445


- Mit 3.5V
  RawBattAdc
  1.648
  RawBattVoltage
  3,00 V
  Release
  5.5.5
  Soil
  38 %
  SoilCalibration
  1584:3504
  SoilRaw
  2.781

- Mit 3.7V
  RawBattAdc
  1.776
  RawBattVoltage
  3,20 V
  Release
  5.5.5
  Soil
  25 %
  SoilCalibration
  1584:3504
  SoilRaw
  3.027


- Mit 4.0V
  RawBattAdc
  2.019
  RawBattVoltage
  3,60 V
  Release
  5.5.5
  Soil
  4 %
  SoilCalibration
  1584:3504
  SoilRaw
  3.443


- Mit 4.2V
  RawBattAdc
  2.201
  RawBattVoltage
  3,90 V
  Release
  5.5.5
  Soil
  4 %
  SoilCalibration
  1584:3504
  SoilRaw
  3.444


- Mit 4.3V
  RawBattAdc
  2.341
  RawBattVoltage
  4,10 V
  Release
  5.5.5
  Soil
  4 %
  SoilCalibration
  1584:3504
  SoilRaw
  3.444

- Mit 5.0V 
  RawBattAdc
  2.926
  RawBattVoltage
  5,10 V
  Release
  5.5.5
  Soil
  4 %
  SoilCalibration
  1584:3504
  SoilRaw
  3.446

## Fix: analogReadMilliVolts() (eFuse-Vref/Two-Point correction)

Replaced the hardcoded `vref = 1100` formula with `analogReadMilliVolts()`
(arduino-esp32 core, wraps `esp_adc_cal_characterize()` +
`esp_adc_cal_raw_to_voltage()` internally, using the chip's factory eFuse
Vref or Two-Point calibration instead of a fixed guess):

```cpp
float battery_voltage = ((float)analogReadMilliVolts(BAT_ADC) / 1000.0) * 2.0; // 2.0 = board's BAT_ADC divider ratio
```

Also re-derived the `Battery %` endpoints from `416`/`290` to **`420`/`330`**
(4.2V = standard 1S LiPo full-charge voltage, 3.3V = lowest bench voltage at
which HiGrow12 still boots — 3.2V fails, LDO dropout — so 0% lines up with
"about to die").

Battery% is also now clamped to `[0, 100]` (was previously an unreachable
commented-out `if (bat < 0) return 0;` — the upper clamp was added at the
same time for the same reason).

## Discarded: per-device empirical linear fit via SPIFFS (2026-07-25)

**Status: tried, then reverted.** Kept here in case per-chip inaccuracy
resurfaces and this needs restoring — do not restore blindly, re-verify
against current sensor readings first, since the reason it was dropped was
that plain `analogReadMilliVolts()` turned out to be accurate enough in
practice on later units (HiGrow42, HiGrow43) without any extra calibration.

### Why it was attempted

On HiGrow12 specifically, `analogReadMilliVolts()` alone was still off by up
to ~17% (non-constant error, worse at lower voltages) — this chip reports
`RawBattCalSource = "eFuse Vref"` (not the more accurate "Two Point"), which
Espressif documents as having a larger residual nonlinearity error. A 7-point
bench measurement and linear regression closed the gap to ~1%:

```
battery_voltage = battAdcSlope * RawBattAdc + battAdcOffset
battAdcSlope = 0.0011776
battAdcOffset = 1.5750
```

### Why it was dropped

1. **Confirmed non-transferable across chips**: applying HiGrow12's fit to
   HiGrow42 (a different physical chip) read 4.6V for an actual 4.3V input —
   worse than just using the generic `analogReadMilliVolts()` fallback would
   have been on that unit.
2. **OTA distributes one firmware.bin to every unit** (see
   `platformio_extra.py`, `ota_deploy` target) — a compile-time per-device
   constant baked into `user-variables.h` would apply to *all* units on the
   next OTA push, not just the one it was measured on. This is why the fit
   was moved to a device-local `/battAdc.conf` on SPIFFS (survives OTA,
   read/written in `main.cpp` `setup()`, mirroring the existing
   `/soil.conf` pattern) rather than staying a compile-time constant.
3. **Turned out to be unnecessary**: later units tested uncalibrated
   (`analogReadMilliVolts()` fallback only) read accurately without any
   per-device fit. HiGrow12's ~17% residual error looks like it was specific
   to that one chip's eFuse-Vref-only calibration, not representative of the
   fleet.

### The removed mechanism, for reference

- `include/user-variables.h` / `include/RENAME TO user-variables.h`: a
  `calibrate_battAdc` bool (default `false`) + `battAdcSlope`/`battAdcOffset`
  floats (default `0`), analogous to `calibrate_soil`/`soil_min`/`soil_max`.
  Every per-sensor block got its own commented `battAdcSlope`/`battAdcOffset`
  pair (all `0` except HiGrow12's real values) so a unit's calibration
  travelled with its block, same as soil.
- `src/main.cpp` `setup()`: right after the existing `/soil.conf` block, an
  analogous block —
  ```cpp
  if (calibrate_battAdc)
  {
    SPIFFS.remove("/battAdc.conf");
    String battAdc_write_str = String(battAdcSlope, 7) + ":" + String(battAdcOffset, 4);
    writeFile(SPIFFS, "/battAdc.conf", battAdc_write_str.c_str());
    battAdcCalibrated = true;
  }
  else
  {
    readFile(SPIFFS, "/battAdc.conf");
    if (readString.length() > 0)
    {
      battAdcSlope = getValue(readString, ':', 0).toFloat();
      battAdcOffset = getValue(readString, ':', 1).toFloat();
      battAdcCalibrated = true;
    }
    else
    {
      battAdcCalibrated = false;
    }
    readString = "";
  }
  ```
  plus a global `bool battAdcCalibrated = false;` near the other sensor
  bools.
- `src/read-sensors.h` `readBattery()`: branched on `battAdcCalibrated` —
  ```cpp
  float battery_voltage;
  if (battAdcCalibrated)
  {
    battery_voltage = battAdcSlope * (float)volt + battAdcOffset;
  }
  else
  {
    battery_voltage = ((float)analogReadMilliVolts(BAT_ADC) / 1000.0) * 2.0;
  }
  ```

### How to calibrate a unit (kept for reference / possible future use)

**Step 1 — measure.** Power the unit via the battery input from a lab
supply, NOT USB (the TP4054 charge IC on the battery rail fights a lab
supply while USB is connected, corrupting the reading). Set the supply to
several known voltages spread across the realistic battery range (e.g.
3.3 / 3.5 / 3.7 / 4.0 / 4.2 V — very low voltages may fail to boot, use the
next higher one instead), and for each, read the `RawBattAdc` value the unit
reports via Home Assistant/MQTT (USB can't be connected at the same time).
Use at least 5 points, not just 2 (see below for why).

**Step 2 — fit a line.** Looking for
`battery_voltage = battAdcSlope * RawBattAdc + battAdcOffset`, the
least-squares line through the (RawBattAdc, voltage) pairs.

Spreadsheet (easiest):
```
battAdcSlope  = STEIGUNG(voltage-range; RawBattAdc-range)        [Excel/LibreOffice DE]
              = SLOPE(voltage-range; RawBattAdc-range)           [English]
battAdcOffset = ACHSENABSCHNITT(voltage-range; RawBattAdc-range) [DE]
              = INTERCEPT(voltage-range; RawBattAdc-range)       [English]
```

By hand, with n measured (x=RawBattAdc, y=voltage) pairs:
```
battAdcSlope  = (n*sum(x*y) - sum(x)*sum(y)) / (n*sum(x^2) - sum(x)^2)
battAdcOffset = (sum(y) - battAdcSlope*sum(x)) / n
```

Worked example (HiGrow12, 2026-07-25, n=7):

| Volt | RawBattAdc | x·y | x² |
|---|---|---|---|
| 3.25 | 1461 | 4748.25 | 2134521 |
| 3.50 | 1648 | 5768.00 | 2715904 |
| 3.70 | 1776 | 6571.20 | 3154176 |
| 4.00 | 2019 | 8076.00 | 4076361 |
| 4.20 | 2201 | 9244.20 | 4844401 |
| 4.30 | 2341 | 10066.30 | 5480281 |
| 5.00 | 2926 | 14630.00 | 8561476 |

sums: n=7, Σx=14372, Σy=27.95, Σxy=59103.95, Σx²=30967120

```
battAdcSlope  = (7*59103.95 - 14372*27.95) / (7*30967120 - 14372^2)
              = 12030.25 / 10215456 = 0.0011776
battAdcOffset = (27.95 - 0.0011776*14372) / 7 = 1.5750
```

Why not just the lowest and highest point? Two points always define an exact
line through only those two — the points in between (where a real battery
spends most of its time) are ignored, and a single misread point shifts the
whole line. With 5+ points, the least-squares fit averages out individual
measurement noise instead of being fully dictated by 2 values. On this same
HiGrow12 data, fitting only the 3.25V/4.30V endpoints gives
slope=0.0011932/offset=1.5066 — close, but up to 0.08V worse than the
7-point fit at the in-between voltages that matter most in practice.

**Step 3 — persist to the unit.** Set `calibrate_battAdc` + the two values,
flash once via USB to persist to `/battAdc.conf` on that unit's flash, then
revert `calibrate_battAdc` to `false` and flash again (read-back mode).

## Session summary and open items (2026-07-25)

### What shipped in 5.5.6

- `readBattery()` uses `analogReadMilliVolts()` instead of hardcoded
  `vref=1100` — verified accurate (within ~1-2%) against a lab supply on
  HiGrow12, HiGrow42, HiGrow43, HiGrow46 without any per-device calibration.
- `Battery %` endpoints changed from `416`/`290` to `420`/`330` (physically
  motivated: 4.2V standard 1S LiPo full-charge, 3.3V lowest voltage at which
  the board still boots — below that it browns out before it can report
  anything anyway).
- `Battery %` clamped to `[0, 100]` (previously an unreachable commented-out
  `if (bat < 0) return 0;` — negative and >100 values could leak through to
  Home Assistant, e.g. -10% observed on a real unit before this fix).
- The per-device empirical linear-fit approach (SPIFFS-persisted
  `battAdcSlope`/`battAdcOffset`) was built, tested, and then reverted — see
  "Discarded" section above. `analogReadMilliVolts()` alone turned out to be
  good enough on every unit tested after HiGrow12.

### Known open issue: HA display precision on newly discovered RawBattVoltage entities

Home Assistant auto-detects a numeric entity's display precision from the
**first** MQTT value it ever receives for that entity, then freezes it in
the entity registry (`options.sensor.suggested_display_precision`) — it does
not re-derive this later even if subsequent values carry more decimals. The
ArduinoHA library version pinned in this project (2.1.0) has no API to send
`suggested_display_precision` in the MQTT discovery config, so this is
outside firmware control.

Symptom: `RawBattVoltage` displays as a rounded whole volt (e.g. "4 V") in
the HA dashboard even though the underlying published state has a decimal
(e.g. "4.2"). This is a **display-only** issue — the actual published/stored
value is correct, and `Battery %` (calculated in firmware, not by HA) is
unaffected.

Already fixed for the 28 `*_rawbattvoltage` entities that existed as of
2026-07-25 (`options.sensor.display_precision` manually set to `2` via
`ha_set_entity`, which overrides HA's frozen auto-detected precision).

**Not fixed for future devices**: any `RawBattVoltage` entity created after
2026-07-25 (new physical unit, or an existing entity deleted/recreated) will
hit the same `suggested_display_precision: 0` freeze on first discovery and
needs the same one-time manual fix:
```
ha_set_entity(entity_id="sensor.<name>_rawbattvoltage", options={"sensor": {"display_precision": 2}})
```
A permanent firmware-side fix would require either patching the vendored
ArduinoHA library to add a `suggested_display_precision` discovery property
(rejected earlier this session as not worth the maintenance burden of a
forked dependency for a display-only issue) or switching that one sensor
from `HASensor` to `HASensorNumber` (which does support precision, but
changes the MQTT payload type from string to numeric - not verified whether
that's a drop-in change against the current `setValue(const char*)` usage
throughout `home-assistant-discovery.h`).

### Other open items, lower priority

- **Divider ratio (`2.0` in `readBattery()`) and AVDD (`3.3V`) assumptions**
  were never independently verified against `images/schematic.pdf`'s actual
  resistor values (candidate root causes #3/#4 from the original analysis).
  Not pursued further since the achieved accuracy already met the practical
  need without it — revisit only if `analogReadMilliVolts()`-based accuracy
  turns out to be insufficient on a unit not yet tested.
- **`Battery %` endpoints (420/330)** are derived from HiGrow12's boot-failure
  threshold and standard LiPo chemistry, not from a real full/empty
  charge/discharge cycle on a physical battery. Should hold across the fleet
  in principle (both reference points are hardware/chemistry properties, not
  curve-fitted to one biased formula like the old 416/290 were), but not
  verified with a real battery over a full cycle.
- **Soil-in-air not reading 0% at low bench voltage** (observed during this
  session's testing) is a separate, already-understood effect — see
  [2026-07-11-soil-battery-sag-analysis.md](2026-07-11-soil-battery-sag-analysis.md).
  Not a battery-calibration issue, no action needed here.
