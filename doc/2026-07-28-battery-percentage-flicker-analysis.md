# Battery% flickers by several percentage points between wake cycles

Analysis: 2026-07-28
Status: **fixed in 5.5.9**

## Symptom

Home Assistant history for `sensor.higrow17_battery` over 7 days shows the
value jumping around inside a 75-89% band between consecutive wake cycles
(30-60 min apart), instead of the steady decline expected from LiPo
discharge:

```
2026-07-24 00:59  84%
2026-07-24 01:59  82%
2026-07-24 02:59  85%
2026-07-24 03:59  86%
2026-07-24 04:59  85%
...
2026-07-25 20:18  83%
2026-07-26 00:00  76%   <- drops 7 points, then stays flat ~75-76% for 3 days
```

`sensor.higrow3_battery` did not show this (it sat at a clean, constant
`100%` throughout — the unit was on charge the whole time, so its voltage
stayed above the 4.2V full-charge point regardless of ADC noise).

## Root cause

`readBattery()` (`src/read-sensors.h`) took a **single, unfiltered** ADC
sample for both the raw ADC count and the calibrated voltage:

```cpp
uint16_t volt = analogRead(BAT_ADC);
...
float battery_voltage = ((float)analogReadMilliVolts(BAT_ADC) / 1000.0) * 2.0;
```

`readSoil()`/`readSalt()` have used a 120-sample sorted trimmed mean
(`readAnalogTrimmedMean()`) since release 5.4.1 specifically because "the
ESP32 ADC is noisy (±20-50 counts)" — see
[2026-07-11-soil-battery-sag-analysis.md](2026-07-11-soil-battery-sag-analysis.md).
`readBattery()` was never updated to match, because the 2026-07-11 analysis
assumed the battery channel "is supposed to track the battery and stays
correct" — an assumption the 2026-07-25 calibration analysis
([2026-07-25-battery-voltage-calibration-issue.md](2026-07-25-battery-voltage-calibration-issue.md))
already flagged as never directly verified.

The `Battery%` mapping (`map(battery_voltage*100, 420, 330, 100, 0)`, endpoints
since 5.5.6) spans only **0.9V end-to-end**, i.e. ~1.1%/mV. The LiPo discharge
curve is very flat in the 3.6-3.8V region where these devices spend most of
their time, so ordinary ADC sample noise (a few mV) that would be invisible
on an absolute-voltage reading turns into multi-percent swings once run
through this steep mapping — consistent with the observed 75-89% band on
higrow17.

A secondary, much smaller contributor: `map()` takes `long` parameters, so
passing the `float battery_voltage` truncated (not rounded) at the call
boundary, adding up to ~1 unit of avoidable rounding error on top of the ADC
noise.

## Fix in 5.5.9

1. **Trimmed-mean sampling for battery**: added
   `readAnalogMilliVoltsTrimmedMean()` (`src/read-sensors.h`), structurally
   identical to `readAnalogTrimmedMean()` but sampling
   `analogReadMilliVolts()` (which does its own read internally, so the
   existing raw-count helper can't be reused for the calibrated path).
   `readBattery()` now uses `readAnalogTrimmedMean(BAT_ADC)` for `RawBattAdc`
   (consistent with `SoilRaw`, which is likewise already the trimmed-mean
   result) and the new helper for the calibrated voltage feeding `Battery%`.
2. **Round instead of truncate** before `map()`: `battery_voltage` is now
   rounded with `roundf()` (already used elsewhere in this codebase, see
   `src/save-configuration.h`) before the cast to `long`.

Cost: +120 samples × 2ms ≈ 240ms additional awake time per cycle — the same
cost already accepted for Soil/Salt in 5.4.1, negligible against WiFi
connect timeouts and deep-sleep intervals.

## Not addressed here

- Divider ratio (`2.0`) and AVDD (`3.3V`) assumptions in the
  voltage formula — already noted in the 2026-07-25 doc as "not pursued
  further since the achieved accuracy already met the practical need".
- Persistent smoothing across boot cycles (e.g. an EMA persisted to SPIFFS).
  Not needed: the cause is intra-boot sample noise, not a systematic drift,
  so intra-boot averaging (like Soil/Salt) is sufficient.

## Verification

After deploying 5.5.9, watch `sensor.higrow17_battery` (or any device
sitting in the flat mid-charge region) over several hours: values should
only change in line with actual charge state, without the several-percent
jumps between consecutive wake cycles seen before. Charging detection
(`config.batcharge` switching to `"charging"`) should still work as before —
the upper-bound overshoot behavior removed in 5.5.8 was not reintroduced.

## Files involved

- [src/read-sensors.h](../src/read-sensors.h) — `readAnalogMilliVoltsTrimmedMean()`,
  `readBattery()`
- [src/main.cpp](../src/main.cpp) — release history comment
