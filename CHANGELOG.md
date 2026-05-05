# Changelog

All notable changes to this project will be documented in this file.

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
