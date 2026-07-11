# Soil readings shrink on weak battery (false 100% soil)

Analysis: 2026-07-11
Implemented: Release 5.4.1, 2026-07-11

## Symptom

As a device's battery drains, the raw soil reading (`SoilRaw`) drifts down.
Once it falls below the calibrated `soil_min` (the "in water" value), the
mapped soil percentage clamps at 100% and stays there until the battery is
recharged — regardless of the actual soil moisture.

## Root cause

The measurement is **not ratiometric**:

- The analog soil front-end on the T-HiGrow (see `images/schematic.pdf`) is a
  CD4060 oscillator (~1.545 kHz excitation through the capacitive probe)
  followed by a TL034 amplifier chain (attenuator → inverting amplifier →
  absolute-value rectifier with 1N4148 diodes) whose filtered DC output goes
  to GPIO32. Its supply rails (V3V/VREF, −VREF via LM2660 charge pump, +5V
  via HT7750S boost) all derive from the **battery rail**, switched by
  PWR_EN (GPIO4).
- The ESP32 ADC, however, measures against its **fixed internal reference**
  (~1.1 V band gap). A sagging supply rail therefore scales the whole signal
  down instead of cancelling out.
- On a weak battery (rising internal resistance) the WiFi TX bursts sag the
  rail exactly while the ADC samples, because the measurement used to run
  *after* `connectToNetwork()`. The diode drops in the rectifier are
  constant, so the output shrinks even faster than the rail itself.
- The battery measurement itself (resistor divider on GPIO33) is *supposed*
  to track the battery and stays correct — which is why the battery
  percentage looks fine while soil pegs at 100%.

In `readSoil()` the calibration maps "small raw = wet":
`map(soilRead, soil_min, soil_max, 100, 0)` with a hard clamp at 100. An
electrically shrunken raw value is indistinguishable from real wetness.

## Evidence (Home Assistant history, higrow12, calibration 1584:3504)

- The two lowest `SoilRaw` daily means in 60 days (1347 on 19.5., 1373 on
  14.5.2026) coincide exactly with the lowest battery days (20% / 25%).
- After every recharge `SoilRaw` steps **up** within a day, without any
  plausible change in the soil: 19.–21.5. battery 20%→67%, raw 1347→1767
  (+420 counts); 25.5. +227; 26.6. +130. Soil drying that fast right at each
  recharge is physically implausible — the step is electrical.
- At the time of analysis: raw 1520 < `soil_min` 1584 ⇒ soil clamped at
  100%, battery 36%.

## Fix in 5.4.1

1. **Measure before WiFi**: `readSoil()`/`readSalt()` moved in `setup()` to
   run right after `read_batt_info()` and before `connectToNetwork()`, so the
   ADC samples without WiFi load. (The SPIFFS calibration load still happens
   earlier, so `soil_min`/`soil_max` are valid.)
2. **Trimmed-mean sampling for soil**: soil now uses the same 120-sample
   trimmed mean (sort, drop extremes, average) that salt always used —
   shared helper `readAnalogTrimmedMean()` in `src/read-sensors.h`. The
   noisy ESP32 ADC (±20–50 counts) previously caused ±1–3% jitter from the
   single sample. Costs ~240 ms awake time per cycle.

This removes the WiFi-load component of the sag. The remaining slow
dependence of the front-end rail on the open-circuit battery voltage is a
hardware property and can only be fully removed in hardware (ratiometric
measurement) or approximated in software by normalizing `soilRead` with the
measured `batvoltage` (first-order compensation — not implemented, the
per-device factor could be fitted from existing `SoilRaw`/battery history).

## Verification

After deploying 5.4.1 watch a weak-battery device (e.g. higrow12): `SoilRaw`
should step up on the first post-update cycle and the soil percentage should
come off the 100% clamp without recharging.
