# Battery Discovery and MQTT Summary

## Goal
The battery values should be available both in the MQTT payload and as dedicated Home Assistant discovery sensors.

## What was implemented
- The existing battery readout logic remains in place.
- The values `battvolt` and `battvoltage` are still included in the MQTT payload.
- Additional Home Assistant discovery sensors were added for:
  - `RawBattAdc`
  - `RawBattVoltage`
- These discovery sensors are updated with the latest values on each publish cycle.

## Notes
- `battvolt` is the raw ADC reading.
- `battvoltage` is the converted battery voltage in volts.
- `floatToString` rounds the value to one decimal place and converts it to a string. This is suitable for display and JSON/MQTT payloads, but it slightly reduces precision.

## Affected files
- [src/save-configuration.h](../src/save-configuration.h)
- [src/home-assistant-discovery.h](../src/home-assistant-discovery.h)

## Status
The changes have been implemented in the codebase and the firmware build completed successfully.
