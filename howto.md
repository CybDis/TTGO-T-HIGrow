# TTGO T-HiGrow Setup & Configuration Guide

## Overview

This project enables the LilyGo T-Higrow plant monitoring sensor to communicate with Home Assistant via MQTT with automatic device discovery capabilities.

## Hardware & Software Requirements

### Hardware
- **LilyGo T-Higrow V1.1** microcontroller
- **USB-C cable** for connectivity and programming
- **MQTT server** (Docker deployment options available)
- **Optional:** Lithium battery (~1100mAh for extended battery life)

### Software
- **VSCode** with **PlatformIO** extension (recommended)
  - Or use **Arduino IDE** (v1.8.12 or later) as alternative
- **Windows 11**, macOS, or Linux
- **MQTT broker** (e.g., Mosquitto)

## Development Environment Setup

### Using PlatformIO (Recommended)

1. **Install VSCode**: Download from https://code.visualstudio.com/
2. **Install PlatformIO Extension**:
   - Open VSCode
   - Go to Extensions (Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Click Install
3. **Open Project**:
   - File → Open Folder
   - Select the TTGO-T-HIGrow_CybDis directory
   - PlatformIO will automatically detect `platformio.ini` and set up the environment
4. **Build and Upload**:
   - Click the PlatformIO icon in the sidebar
   - Connect device via USB-C
   - Click "Upload and Monitor" to compile, upload, and open serial monitor

### Using Arduino IDE

While Arduino IDE can be used, PlatformIO provides better project management and library handling. If using Arduino IDE:
- Install ESP32 board support via Arduino Board Manager
- Manually add required libraries via Library Manager
- Configure board as "ESP32 Dev Module"

## Project Structure

The project uses a modular architecture with header files in the `src/` directory:

```
src/
├── main.cpp                         # Main firmware logic
├── user-variables.h                 # Your configuration (created from template)
├── connect-to-network.h            # WiFi connection
├── read-sensors.h                  # Sensor reading functions
├── read-batt-info.h                # Battery level calculation
├── home-assistant-discovery.h      # Home Assistant integration
├── go-to-deep-sleep.h              # Sleep mode management
├── ota-update.h                    # OTA firmware update check
├── save-configuration.h            # Legacy MQTT JSON publish
├── file-management.h               # SPIFFS read/write helpers
├── get-string-value.h              # String parsing helpers
├── floatConv.h                     # Float truncation helper
└── 18B20_class.h                   # DS18B20 sensor class (currently unused, see Step 7)
```

## Configuration Setup

### Step 1: Create Configuration File

1. Navigate to the `include/` directory
2. Rename the file `RENAME TO user-variables.h` to **`user-variables.h`**
3. Open the renamed file in your editor

### Step 2: Configure Sensor Calibration

Soil sensor calibration is critical for accurate readings:

1. **Calibration Mode**:
   ```cpp
   bool calibrate_soil = false;  // Set to true for calibration
   ```

2. **Measure Wet Value** (`soil_min` — low raw value):
   - Submerge the sensor up to the electronics in distilled water
   - Note the raw value from serial output
   - Update the variable:
     ```cpp
     int soil_min = 1535;  // Your wet (water) value
     ```

3. **Measure Dry Value** (`soil_max` — high raw value):
   - Place the sensor in completely dry air
   - Note the raw value from serial output
   - Update the variable:
     ```cpp
     int soil_max = 3300;  // Your dry (air) value
     ```

4. **Finalize**:
   - Set `calibrate_soil = false`
   - Recompile and upload

> **Note**: Calibrate with a full (or charging) battery. The analog sensor
> rail follows the battery voltage, so raw readings shrink as the battery
> drains (see `doc/2026-07-11-soil-battery-sag-analysis.md`). Since v5.4.1
> soil/salt are measured before WiFi starts and averaged over 120 samples,
> which removes most of this effect.

### Step 3: Plant Name Configuration

1. Set your plant identifier:
   ```cpp
   String plant_name = "Padron_1";  // Change to your plant name
   ```

2. Enable plant name update mode (one-time):
   ```cpp
   const bool update_plant_name = true;  // Upload once with true
   ```

3. Upload the firmware, then change back to `false` and re-upload

### Step 4: WiFi Configuration

Configure your WiFi networks (the device tries them in order):

```cpp
String ssidArr[] = {"Wifi1", "Wifi2", "BackupWifi"};
const char* password = "password";  // Password used for all SSIDs
```

**Note:** Current implementation uses the same password for all WiFi networks. Modify `connect-to-network.h` if you need different passwords.

### Step 5: MQTT Broker Configuration

```cpp
const char broker[] = "192.168.1.23";  // IP or hostname of MQTT broker
int        port     = 1883;            // MQTT port (default: 1883)
const char mqttuser[] = "";            // MQTT username (empty if not needed)
const char mqttpass[] = "";            // MQTT password (empty if not needed)
```

### Step 6: Timezone Configuration

NTP server for time synchronization:

```cpp
const char* ntpServer = "pool.ntp.org";  // NTP server (default works for most)
```

The timezone is handled automatically via NTP.

### Step 7: Sensor Configuration

#### DHT Sensor Type

Uncomment your DHT sensor type (exactly one must be active):

```cpp
#define DHT_TYPE DHT11      // Uncomment if using DHT11
//#define DHT_TYPE DHT12    // Uncomment if using DHT12
//#define DHT_TYPE DHT22    // Uncomment if using DHT22
```

Enable/disable DHT sensor:

```cpp
bool dht_found = true;  // Set to false if no DHT sensor present
```

#### External DS18B20 Temperature Sensor

The firmware declares a DS18B20 sensor object on GPIO 21 and a
`USE_18B20_TEMP_SENSOR` flag in `user-variables.h`, but nothing in `main.cpp`
currently reads the sensor or evaluates the flag — this feature is wired up
but not implemented yet. Leave the flag as-is; changing it has no effect.

### Step 8: Reporting Interval

Control how often the sensor sends data (in seconds):

```cpp
#define TIME_TO_SLEEP  3600  // 60 minutes
//#define TIME_TO_SLEEP  7200    // 120 minutes = 2h
//#define TIME_TO_SLEEP  10800   // 180 minutes = 3h
//#define TIME_TO_SLEEP  21600   // 360 minutes = 6h
```

**Clock-aligned wake-ups (since v5.5.0):** the sensor does not simply sleep the configured number of seconds — it computes the sleep duration so the next wake-up lands on the clock grid defined by the interval: on the full hour with `3600`, on the half hour (`:00`/`:30`) with `1800`, on every second full hour with `7200`, and so on. Alignment is based on NTP (UTC) time; if the NTP sync fails, the sensor falls back to sleeping the plain interval. If the next grid point is less than two minutes away, the sensor skips ahead to the following one so it does not wake up again almost immediately.

### Step 9: Logging Configuration

For debugging and troubleshooting:

```cpp
const bool  logging = false;         // Enable serial logging
const bool  readLogfile = false;     // Read stored log from SPIFFS
const bool  deleteLogfile = false;   // Delete stored log from SPIFFS
```

**Note:** Enable only when debugging - logging significantly increases power consumption and drains battery faster.

### Step 10: Fertilizer Thresholds

`user-variables.h` declares `fertil_needed`, `fertil_low`, `fertil_opt` and
`fertil_high`, but the firmware does not currently read them — the salt
advice breakpoints are hardcoded in `main.cpp` and always classify as
"needed" (<201), "low" (201-250), "optimal" (251-350) or "too high" (>350).
Editing these variables has no effect; change the thresholds in `main.cpp`
directly if you need different breakpoints.

### Step 11: OTA Updates (Optional, Recommended)

Enable over-the-air firmware updates via Home Assistant (see [OTA Firmware Updates](#ota-firmware-updates)):

```cpp
const char otaBaseUrl[] = "http://192.168.1.23:8123/local/higrow";  // empty = OTA disabled
```

Use `https://` if your Home Assistant serves port 8123 via SSL.

## OTA Firmware Updates

Since v5.4.0 the sensors can update their firmware over WiFi (pull principle: each sensor checks for a new version on every wake-up). The firmware is hosted on your Home Assistant under `config/www/higrow/`.

### One-Time Setup

1. Set `otaBaseUrl` in `user-variables.h` (Step 11 above)
2. Flash every device **once via USB** with the OTA-capable firmware (v5.4.0+): `pio run -t upload`
3. For automated deployment: install the **Terminal & SSH addon** on Home Assistant and set up key-based SSH access; adjust `OTA_HOST`, `OTA_PORT` and `OTA_REMOTE_DIR` at the top of `platformio_extra.py`

### Rolling Out a New Version

1. Increment the release version in `src/main.cpp` (`const String rel = "..."`)
2. Run:
   ```
   pio run -t ota_deploy
   ```
   This builds the firmware, generates `manifest.json` (version, MD5, size) and copies both to Home Assistant via scp — the binary first, the manifest last, so the manifest never points to an incomplete file.
3. Each sensor picks up the new version on its next wake-up (within one sleep interval; every 5 minutes for devices in charging mode), flashes it and reboots. The new version appears in the `Software version` sensor in Home Assistant.

**Manual alternative** (no SSH addon): copy `firmware.bin` and `manifest.json` from `.pio/build/esp32dev/` to `config/www/higrow/` via the Samba share — same order, binary first.

### Behavior & Safety

- **OTA never blocks**: if the manifest is unreachable or invalid, the download aborts or the MD5 does not match, the sensor just continues its normal measurement cycle on the old firmware and retries on the next wake-up
- **Downgrades work**: the version check is "different", not "newer" — deploying an older build rolls devices back
- **SPIFFS survives**: plant name, soil calibration and battery info are per-device and untouched by updates
- **No automatic rollback**: if a new firmware boots but crashes, the device stays on it — test each release on one device via USB before deploying it to the manifest
- Verify what is deployed: `curl -k https://<ha-ip>:8123/local/higrow/manifest.json`

## Home Assistant Auto-Discovery

### Discovery Process (v5.0.0+)

The firmware provides **native Home Assistant integration** using the ArduinoHA library:

1. **First Run**: Connect device to power with WiFi & MQTT configured
2. **Automatic Discovery**: Device publishes discovery topics to MQTT
3. **Sensor Creation**: All sensors automatically appear in Home Assistant
4. **Zero Configuration**: No manual entity setup needed in HA

### Sensors Published

- **Environment**: Temperature, Humidity, Pressure, Light (Lux)
- **Soil**: Moisture %, Raw values, Calibration values
- **Fertilizer**: Salt levels with advice (needed/low/optimal/too high)
- **Battery**: Percentage, Voltage, Charging status, Days on battery
- **System**: Boot count, Sleep count, MAC ID, WiFi SSID
- **Device**: Plant name, Software version, Last update timestamp

### Reinstalling Device

If you need to re-add the device:

1. Remove the device from Home Assistant
2. Delete the device entry from MQTT discovery
3. Restart the device (or let it wake from sleep)
4. Device will publish discovery topics again

## Monitoring & Operation

### Battery Life

- Approximately **2-3 months** on a single 1100mAh lithium battery
- With hourly reporting interval (3600 seconds)
- Battery voltage is monitored and reported to Home Assistant

### Extending Battery Life

1. **Increase Reporting Interval**:
   ```cpp
   #define TIME_TO_SLEEP  21600  // 6 hours instead of 1 hour
   ```

2. **Disable Debug Logging**:
   ```cpp
   const bool logging = false;  // Must be false in production
   ```

3. **Optimize WiFi Signal**:
   - Place device closer to router
   - Reduce environment interference
   - Low RSSI (signal strength) increases power consumption

### Sensor Updates in Home Assistant

- All measurements automatically update on the defined interval
- Update frequency matches `TIME_TO_SLEEP` setting
- Last update timestamp shows when device last reported

## Troubleshooting

### Device Not Appearing in Home Assistant

**Check Checklist**:

1. **MQTT Broker**:
   - Is it running? (`mosquitto -v` or check Docker container)
   - Can the device reach it? Verify IP/hostname and port
   - Are username/password correct?

2. **WiFi Connection**:
   - Verify SSID and password in `user-variables.h`
   - Check device is in range and not blocked
   - Confirm WiFi network exists and is broadcasting SSID

3. **Firewall**:
   - Port 1883 (MQTT) must be open between device and broker
   - Check device firewall allows outbound MQTT

4. **Serial Output**:
   - Open serial monitor (115200 baud)
   - Look for connection errors
   - Device prints IP, WiFi SSID, and MQTT status on startup

### Incorrect Sensor Readings

**Troubleshooting**:

1. **Soil Moisture**:
   - Re-calibrate using `calibrate_soil = true`
   - Ensure sensor is fully buried in soil
   - Check sensor is not damaged or corroded

2. **Temperature/Humidity**:
   - Verify correct DHT type uncommented in config
   - Ensure sensor is not obstructed
   - DHT sensors are sensitive - give stabilization time after power on

3. **Battery Reading**:
   - Battery reading taken at startup
   - Ensure battery is properly connected
   - ADC pin 33 should show voltage proportional to battery

### Battery Drains Too Quickly

**Solutions**:

1. **Increase Sleep Time**:
   - Default 1 hour is aggressive for battery life
   - Try 6-hour interval: `#define TIME_TO_SLEEP 21600`

2. **Check WiFi Signal**:
   - Weak signal (low RSSI) requires more power for connection
   - Device logs RSSI value in serial output
   - Try repositioning device closer to router

3. **Disable Logging**:
   ```cpp
   const bool logging = false;  // Logging drains battery fast
   ```

4. **Check for Crashes**:
   - Enable logging temporarily to see error messages
   - Crashes cause device to restart repeatedly

### Serial Monitor Shows Garbage

- **Fix**: Change baud rate to **115200**
- PlatformIO Monitor sets this automatically
- Arduino IDE: Tools → Serial Monitor → 115200 baud

### Device Resets or Crashes

- Check for stack overflow in logs
- Ensure sufficient free SPIFFS space
- Look for sensor read failures or timeout errors

## Firmware Build & Upload

### Using PlatformIO

1. **Build**:
   - Click PlatformIO → Build
   - Check for compilation errors

2. **Upload**:
   - Connect device via USB-C
   - Click PlatformIO → Upload and Monitor
   - Wait for "Leaving... Hard resetting via RTS pin" message

3. **Monitor**:
   - Serial monitor opens automatically
   - View startup messages and debug output
   - Ctrl+C to stop monitoring

### Advanced Build Options

Edit `platformio.ini` to customize:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

## Home Assistant Integration Examples

### Automation: Low Moisture Alert

```yaml
automation:
  - alias: "Plant Moisture Alert"
    trigger:
      platform: numeric_state
      entity_id: sensor.padron_1_soil
      below: 30
      for:
        hours: 2
    action:
      service: notify.mobile_app_phone
      data:
        message: "Padron needs water - moisture below 30%"
```

### Dashboard Card

Use the [Flower Card (Mod)](https://github.com/CybDis/lovelace-flower-card-mod) with [Custom Plant Integration](https://github.com/Olen/homeassistant-plant) for visual plant monitoring.

## Getting Help

### Debug Serial Output

Monitor serial output while device boots:

```
Void Setup
Connected to network!
MAC: AA:BB:CC:DD:EE:FF
IP: 192.168.1.100
DNS: 192.168.1.1
Hostname: Padron_1
lux: 245
Soil: 1843 (45%)
Temp: 22.5C
Battery: 87%
```

### Check Home Assistant

1. Navigate to Settings → Devices & Services → MQTT
2. Look for discovered devices
3. Check entity status and last update timestamp
4. Review Home Assistant logs for errors

### Firmware Version

View current firmware version in serial output or Home Assistant device info.

See [CHANGELOG.md](CHANGELOG.md) for version history and upgrade notes.
