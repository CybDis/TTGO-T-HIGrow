# CHANGELOG

## [5.0.0] - 2025-09-05

### 🚀 MAJOR RELEASE: Native Home Assistant Integration

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

## [4.7.0] - Previous Release
- Removed external water level sensor

## [4.6.x] - Previous Releases
- Various bug fixes and optimizations
- Battery management improvements
- Sensor reading enhancements

---

### Upgrade Instructions

1. **Flash new firmware** (v5.0.0) to your TTGO T-HIGrow device
2. **Remove Python autodiscovery script** if previously used
3. **Restart Home Assistant** to discover new sensors
4. **Verify sensor entities** appear automatically in Home Assistant

### Support

For issues or questions regarding the new Home Assistant integration, please create an issue on GitHub.
