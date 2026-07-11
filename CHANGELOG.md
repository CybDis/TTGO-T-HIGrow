# Changelog

All notable changes to this project will be documented in this file.

## [5.4.1] - 2026-07-11

### Fixed
- **Soil/Salt readings shrink on weak battery**: the analog soil/salt front-end (CD4060 oscillator + TL034 amplifier chain) is supplied from the battery rail, while the ESP32 ADC measures against its fixed internal reference. With WiFi active, TX current bursts sag the rail on a weak battery, the raw soil value drops below `soil_min` and the soil sensor falsely pegs at 100%. Soil and salt are now measured **before** `connectToNetwork()`, so the ADC sampling happens without WiFi load (confirmed via HA history: recharge events stepped `SoilRaw` up by 130–420 counts within a day)
- **Soil was a single ADC sample**: soil now uses the same 120-sample trimmed mean (sort, drop extremes, average) that salt always used — the noisy ESP32 ADC caused ±1–3% jitter on the soil percentage. Shared helper `readAnalogTrimmedMean()` in `read-sensors.h`, adds ~240 ms awake time per cycle

---

## [5.4.0] - 2026-07-11

### MAJOR RELEASE: OTA Firmware Updates via Home Assistant

#### Added
- **Pull-OTA**: On every wake-up (right after WiFi connect, before NTP/measurement/MQTT) the sensor fetches `manifest.json` from Home Assistant `/local`, compares the version to its own release and, if it differs, streams `firmware.bin` into the inactive OTA slot (MD5-verified) and reboots into the new firmware
- **Downgrade support**: Version check is deliberately `!=` instead of `>` — putting an older build into the manifest rolls devices back
- **HTTPS support** for HA instances serving port 8123 via SSL (`WiFiClientSecure`, integrity ensured by manifest MD5)
- **PlatformIO deploy target** `pio run -t ota_deploy`: builds, generates `manifest.json` (version, MD5, size) and copies both via scp to `/config/www/higrow/` on HA (binary first, manifest last)
- New user variable `otaBaseUrl` in `user-variables.h` (empty = OTA check disabled)
- Implementation notes and operating guide in `doc/2026-07-10-ota-update-design.md`

#### Changed
- OTA never blocks: unreachable manifest, invalid JSON, aborted download or MD5 mismatch simply continue the normal measurement cycle on the old firmware

#### Removed
- Unused `ArduinoHttpClient` dependency (its `HttpClient.h` shadowed the ESP32 framework's `HTTPClient.h` on case-insensitive filesystems and broke the build on Windows)

#### Migration Notes
- **One last USB flash per device** (`pio run -t upload`) to get the OTA-capable firmware on board — all later versions arrive over the air
- No partition change: the build fits the default scheme's two OTA app slots, SPIFFS (plant name, soil calibration, battery info) is preserved
- No automatic rollback if a new firmware boots but crashes — test each release on one device via USB before deploying it to the manifest

#### Memory Usage
- **RAM**: 16.2% (53,056 bytes of 327,680 bytes)
- **Flash**: 89.8% (1,176,845 bytes of 1,310,720 bytes) — HTTPClient/TLS adds ~180 KB

---

## [5.3.0] - 2026-06-28

### Added
- **Charging mode**: sleep interval reduced to 5 minutes while the battery is charging
- `SoilCalibration` sensor (min:max) published to Home Assistant to compare against `SoilRaw` when readings look strange on battery

### Removed
- SoilTemp sensor (no hardware present for it)

### Changed
- Dependency updates

## [5.2.1] - 2026-06-01

### Changed
- Optimized serial debug outputs
- Removed unnecessary sleeps

## [5.2.0] - 2026-06-01

### Added
- **Local-only / no-internet mode**: optional `ntpServerIp` uses a local NTP server IP (e.g. Fritzbox) instead of an internet NTP hostname

## [5.1.0] - 2026-05-29

### Changed
- Enabled `force_update` on all measurement sensors in HA Discovery so `last_changed` updates even when the value stays the same

---

## [5.0.1] - 2026-05-05

### Added
- `state_class` attribute (measurement/total_increasing) to all Home Assistant Discovery payloads
- Unit "count" for boot count and sleep5 count sensors

### Fixed
- Pressure unit: `Hpa` → `hPa` correction
- Device class assignments for proper Home Assistant categorization

---

## [5.0.0] - 2026-05-04

### MAJOR RELEASE: Native Home Assistant Integration

#### Added
- **Native Home Assistant Auto Discovery** using ArduinoHA library
- **Direct ESP32 Integration** - No external Python script dependency required
- **Automatic Sensor Registration** - All sensors automatically appear in Home Assistant
- **Complete Device Information** with manufacturer, model, and software version
- **All Sensor Types Supported**:
  - Environment: Temperature, Humidity, Pressure, Light (Lux)
  - Soil: Moisture %, Raw values, Soil Temperature
  - Fertilizer: Salt levels with advice (needed/low/optimal/too high)
  - Battery: Percentage, Voltage, Charging status, Days on battery
  - System: Boot count, Sleep count, MAC ID, WiFi SSID
  - Device: Plant name, Software version, Last update timestamp

#### Changed
- **BREAKING**: Migrated from external Python autodiscovery to native ESP32 integration
- **Improved Memory Efficiency**: Direct MQTT communication without intermediate scripts
- **Enhanced Reliability**: Eliminates external script dependencies and potential failure points
- **Better Performance**: Reduced network overhead and faster sensor updates

#### Technical Details
- Added ArduinoHA library dependency (`dawidchyrzynski/home-assistant-integration @ ^2.1.0`)
- Created `home-assistant-discovery.h` with complete HA integration
- Implemented `sendDiscoveryTopic()` and `updateHASensors()` functions
- Maintained backward compatibility with existing MQTT JSON messages
- Updated PlatformIO configuration with stable ESP32 platform (6.8.1)

#### Migration Notes
- **For existing users**: The external Python script (`TTGO-T-HiGrow-aut.py`) is no longer needed
- **Backward Compatibility**: Original MQTT messages are still sent for existing integrations
- **Home Assistant**: Sensors will be automatically discovered on first run
- **Configuration**: No changes required to existing device configuration

#### Memory Usage
- **RAM**: 15.4% (50,408 bytes of 327,680 bytes)
- **Flash**: 76.0% (995,801 bytes of 1,310,720 bytes)

---

## [4.7.0]

### Removed
- External water level sensor support

---

## [4.6.7]

### Changed
- External wake deactivated (non-functional on HiGrow3 variant, not used)

## [4.6.6] - ~2024-10-2024

### Fixed
- Read battery voltage directly at start of setup

## [4.6.5] - 2024-09-29

### Fixed
- Fixed unsigned integer handling for soil sensor not readable condition
- Added `soilRaw` reading for debugging
- Fixed battery reading and moved to first after startup

## [4.6.4] - 2024-04-21

### Changed
- Removed Date/Time fields, replaced with Updated timestamp (UNIX epoch)

### Fixed
- Fixed faultive readings in soil and battery
- Removed obsolete DST (Daylight Saving Time) code

## [4.6.3]

### Fixed
- Fixed sensor limits (battery <0, soil not readable edge cases)

## [4.6.2]

- Minor updates

## [4.6.1]

### Changed
- Removed Date/Time fields
- Replaced with Updated timestamp (UNIX epoch)

## [4.6.0] - 2023-05-31

### Added
- Custom water sensor support (soldering required)
- Soil limited to Min/Max 0/100%
- Optimized MQTT JSON output (strip/limit decimals to 1)
- Optimized and corrected debug output

### Changed
- Interval changed from default to 1h

---

## [4.5.1] - ~2023-05-31

### Changed
- Reporting interval changed to 2 hours

## [4.5.0] - 2023-05-31

### Changed
- Soil moisture limited to Min/Max 0/100% range

---

## [4.3.2] - 2021-04-03

### Fixed
- Corrected DST calculation

## [4.3.1] - 2021-03-05

### Fixed
- Fixed days since last charging calculation

---

## [4.2.3]

### Removed
- Removed battery day counter (use BeardedTingers solution if needed)

## [4.2.2] - 2021-02-19

### Added
- Plant valve number support for Greenhouse auto-watering system (in development)

## [4.2.1] - 2021-02-17

## [4.2.0] - 2021-02-15

### Added
- BME280 environmental sensor implementation (temperature, humidity, pressure)

---

## [4.1.0] - 2021-02-10

### Added
- Support for external DS18B20 temperature sensor

---

## [4.0.7] - 2021-02-08

### Added
- Plant name now used as device hostname (visible in router)

## [4.0.6] - 2020-12-30

### Fixed
- Corrected days counter calculation (again!)

## [4.0.5] - 2020-12-26

### Changed
- Merged contribution from @reenari
- Corrected days since last charge counter

## [4.0.4] - 2020-12-06

### Added
- Battery charge date tracking
- Days since last charge calculation
- Data persistence to SPIFFS (survives reboot)

## [4.0.3] - 2020-12-05

### Added
- Battery charge date tracking
- Days since last charge calculation

## [4.0.2] - 2020-12-04

### Changed
- Reorganized subroutines
- Improved functional code snippets

## [4.0.1]

### Fixed
- Network connection error correction

## [4.0.0]

### Major Changes
- Migrated from Arduino IDE to VS Code with PlatformIO
- Complete project restructuring for better development experience

---

## [3.0.6]

### Fixed
- Corrected DST (Daylight Saving Time) calculation (was significantly wrong)

## [3.0.5]

### Added
- Plant name now saved to SPIFFS (persistent storage)

## [3.0.4]

### Changed
- Adapted to HACS frontend card: Battery State Card

## [3.0.3]

### Fixed
- Small error corrections

## [3.0.2]

### Fixed
- DST (Daylight Saving Time) switch-over now works correctly

## [3.0.1]

### Added
- Home Assistant MQTT Autodiscovery implementation
- Salt level calibration
- Salt level advice messages

## [3.0.0]

### Major Changes
- Implemented Home Assistant MQTT Autodiscovery
- Automatic sensor registration in Home Assistant

---

## [2.0.2]

### Added
- Automatic WiFi SSID detection and connection to available networks

## [2.0.1]

### Added
- Name index sensor icon prefix (`_` + name)

### Fixed
- Corrected missing leading zero in HEX address formatting

## [2.0] - 2020-11-09

### Major Changes
- Implemented MAC address as unique device identifier
- Device name standardized to `Tgrow_HIGrow` (frozen parameter)
