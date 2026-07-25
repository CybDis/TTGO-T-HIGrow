# Soil readings shrink on weak battery (false 100% soil)

Analysis: 2026-07-11
Implemented: Release 5.4.1, 2026-07-11
Root cause corrected: 2026-07-25 (see "Correction" at the end)

## Symptom

As a device's battery drains, the raw soil reading (`SoilRaw`) drifts down.
Once it falls below the calibrated `soil_min` (the "in water" value), the
mapped soil percentage clamps at 100% and stays there until the battery is
recharged - regardless of the actual soil moisture.

## The two analog channels

The board has two independent analog measurement chains. They are easy to mix
up, so both are listed here (see `images/schematic.pdf`, and `doc/board-rails-and-headers.md`
for the supply rails):

**Soil moisture - `SOIL_PIN` = GPIO32, net `Humi`, ESP32 pin 12**

`U11` (TLC555) runs as an astable oscillator at roughly 875 kHz
(`R34` 300R, `R35` 1.6K, `C60` 470pF; `f = 1.44 / ((R34 + 2 * R35) * C60)`).
Its VCC (pin 8) and RESET (pin 4) are tied to **`V3V`**. The output (pin 3)
drives the capacitive probe electrode through `R31` 10K. The AC amplitude at
the electrode is peak-detected by **`D5` (1N4148)** into `C53` 1uF, with
`R28` 1M as the discharge path. That DC node is net `Humi` and goes straight
to GPIO32.

Wetter soil means more probe capacitance, lower impedance at the electrode,
smaller AC amplitude, and therefore a *lower* raw value - which is why
`readSoil()` maps `soil_min` (in water) to 100%.

**Salt / EC - `SALT_PIN` = GPIO34, net `AD`, ESP32 pin 10**

`U5` (CD4060) oscillates at 1.545 kHz (frequency annotated on the schematic)
and drives the EC electrodes. `U6` (TL034) follows with the three stages
labelled on the schematic: attenuator, inverting amplifier, and an
absolute-value rectifier using `D2`/`D3`/`D4` (1N4148). The filtered DC output
is net `AD` on GPIO34. This chain is supplied from **`VREF`** (about +5 V) and
**`-VREF`** (about -5 V), not from `V3V` directly.

LilyGO's own pinmap in `images/T-Higrow.jpg` agrees with this assignment:
"Fertility: IO34, Watering: IO32".

## Root cause

The soil measurement is **not ratiometric**, and its supply has a hard knee.

`V3V` is the output of **`U4` (JC5333)**, a 3.3 V linear regulator whose input
sits directly on the cell node `BAT`. There is no switch in series: the
regulator input is the battery.

- While the cell is above `3.3 V + dropout`, `U4` regulates and `V3V` is
  exactly 3.3 V. The TLC555 output amplitude is rail-to-rail on `V3V`, so the
  peak-detected DC is constant and `SoilRaw` is stable. This is the observed
  stable region.
- Below that point `U4` is in dropout and `V3V` follows the cell voltage. The
  ESP32 ADC measures against its fixed internal band gap reference (~1.1 V),
  so nothing cancels out: every millivolt lost on `V3V` is a millivolt lost in
  the reading.
- `D5`'s forward drop is constant, so the output shrinks slightly *faster*
  than proportionally as the rail falls.

The transfer is approximately:

```
SoilRaw ~ (probe divider ratio * V3V - U_D5) scaled by the ADC
```

with roughly 1200 counts per volt at the 11 dB attenuator setting. This is
consistent with the calibration: the air value of 3504 counts corresponds to
about 2.9 V, which is `3.3 V` minus D5's forward drop - the dry reading is
simply the rail minus one diode.

The salt channel shares the knee, because `VREF` is boosted from `V3V` by
`U12` (HT7750S, via `L3` 47uH and `D8` 1N5817) and `-VREF` is generated from
`VREF` by `U10` (LM2660). Below the dropout point the salt reading must drift
too, and potentially earlier, since the boost loses both headroom and current
capability at the same time.

The battery measurement itself (`R10` / `R78`, both 100K 1%, on GPIO33) tracks
the cell correctly, which is why the battery percentage looks plausible while
soil pegs at 100%.

In `readSoil()` the calibration maps "small raw = wet":
`map(soilRead, soil_min, soil_max, 100, 0)` with a hard clamp at 100. An
electrically shrunken raw value is indistinguishable from real wetness.

## Why the apparent threshold differs between devices

The dropout point is a hardware constant, but the *reported* battery voltage
is not comparable between devices. `readBattery()` in `src/read-sensors.h`
hardcodes `vref = 1100` and uses raw `analogRead()` with the default 11 dB
attenuation. The real band gap reference of an individual ESP32 lies roughly
between 1000 and 1200 mV, and the attenuator curve is markedly non-linear near
full scale. The same physical dropout point can therefore be reported as
anything from about 3.5 V to about 3.9 V depending on the chip.

The divider itself is accurate (two 100K 1% resistors, so the `* 2.0` in the
code is correct); the error is entirely in the ADC path. Cell internal
resistance adds to the spread: `readBattery()` runs at the very top of
`setup()`, close to open circuit, while `V3V` is already loaded when soil is
sampled, so an aged cell widens the gap between reported and effective
voltage.

## Evidence (Home Assistant history, higrow12, calibration 1584:3504)

- The two lowest `SoilRaw` daily means in 60 days (1347 on 19.5., 1373 on
  14.5.2026) coincide exactly with the lowest battery days (20% / 25%).
- After every recharge `SoilRaw` steps **up** within a day, without any
  plausible change in the soil: 19.-21.5. battery 20%->67%, raw 1347->1767
  (+420 counts); 25.5. +227; 26.6. +130. Soil drying that fast right at each
  recharge is physically implausible - the step is electrical.
- The +420 count step corresponds to about 0.35 V at 1200 counts/V, i.e. `V3V`
  had sagged from 3.30 V to roughly 2.95 V. That is the expected dropout of an
  inexpensive LDO at this load, and it matches the reported voltages: 20%
  maps to 3.15 V and 67% to 3.74 V through `map(..., 416, 290, 100, 0)`.
- At the time of analysis: raw 1520 < `soil_min` 1584 => soil clamped at
  100%, battery 36%.

## Fix in 5.4.1

1. **Measure before WiFi**: `readSoil()`/`readSalt()` moved in `setup()` to
   run right after `read_batt_info()` and before `connectToNetwork()`, so the
   ADC samples without WiFi load. (The SPIFFS calibration load still happens
   earlier, so `soil_min`/`soil_max` are valid.)
2. **Trimmed-mean sampling for soil**: soil now uses the same 120-sample
   trimmed mean (sort, drop extremes, average) that salt always used -
   shared helper `readAnalogTrimmedMean()` in `src/read-sensors.h`. The
   noisy ESP32 ADC (+-20-50 counts) previously caused +-1-3% jitter from the
   single sample. Costs ~240 ms awake time per cycle.

This removed the load-transient component: WiFi TX bursts on a weak cell pull
the cell down while the ADC samples, which pushes `U4` into dropout earlier
than the quiescent cell voltage would suggest. It does not remove the dropout
knee itself, which is a property of the supply topology.

## Remaining options

- **Stable rail (hardware).** Supplying `V3V` from a regulator that holds
  3.3 V across the whole cell range removes the effect entirely and
  independently of where an individual unit's knee sits. A plain step-down is
  the wrong topology - it cannot regulate 3.3 V from a 3.4 V input either. A
  buck-boost/SEPIC, or a boost to 5 V followed by a linear 3.3 V regulator,
  does work. Note that `V3V` is reachable on `P6` pin 1 and `P8` pin 1, and
  that neither side-header `M3V` pin carries it - both of those are `VDD3V3`.
  See `doc/board-rails-and-headers.md`.
- **Software normalisation.** `RawBattVoltage` has been published since 5.5.5,
  so a per-device correction can be fitted from existing history. Because of
  D5's constant offset an affine model
  (`SoilRaw + k * (V_knee - V_batt)` below the knee) fits better than a purely
  multiplicative one.
- **Validity flag.** Alternatively, mark soil invalid below a configurable
  battery threshold instead of publishing a false 100%.

None of these are implemented.

## Verification

After deploying 5.4.1 watch a weak-battery device (e.g. higrow12): `SoilRaw`
should step up on the first post-update cycle and the soil percentage should
come off the 100% clamp without recharging.

To confirm the dropout mechanism on hardware:

- With the cell at about 3.3 V and USB connected, the system rail is supplied
  from VBUS, so `VDD3V3` is a solid 3.3 V while `V3V` still comes from `U4` on
  the cell. `SoilRaw` should still be in its drifted state, which isolates the
  cause to `V3V`.
- Measuring `V3V` against cell voltage while the cell discharges locates the
  knee and gives the actual dropout of `U4` at this load.
- Measuring `V3V` during deep sleep confirms whether the rail is ungated. The
  schematic shows `U4`'s input on `BAT` without a series switch, which implies
  the whole analog section is powered continuously.

## Correction

The version of this document written on 2026-07-11 attributed the soil path to
the CD4060 oscillator and the TL034 absolute-value chain, and described the
front-end rails as deriving directly from the battery rail switched by
`PWR_EN` (GPIO4). Both were wrong. The CD4060/TL034 chain is the salt/EC
channel on GPIO34; soil on GPIO32 is the TLC555 peak detector. `V3V` is
regulated by `U4` and is not gated by `PWR_EN` - GPIO4 drives `Q3` (SI2301)
between `BAT` and the system rail, bypassing its body diode, which is why
readings are wrong unless GPIO4 is held high.

The symptom, the evidence and the 5.4.1 change are unaffected by the
correction.
