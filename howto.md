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
- **Windows 10**, macOS, or Linux
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
├── module-parameter-management.h    # MQTT & device setup
├── connect-to-network.h            # WiFi connection
├── read-sensors.h                  # Sensor reading functions
├── read-batt-info.h                # Battery level calculation
├── home-assistant-discovery.h      # Home Assistant integration
├── go-to-deep-sleep.h              # Sleep mode management
├── time-management.h               # NTP time synchronization
└── [other helper modules]
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

2. **Measure Dry Value**:
   - Place the sensor in completely dry air/soil
   - Note the `soil_min` value from serial output
   - Update the variable:
     ```cpp
     int soil_min = 1535;  // Your dry value
     ```

3. **Measure Wet Value**:
   - Submerge the sensor up to the electronics in distilled water
   - Note the `soil_max` value from serial output
   - Update the variable:
     ```cpp
     int soil_max = 3300;  // Your wet value
     ```

4. **Finalize**:
   - Set `calibrate_soil = false`
   - Recompile and upload

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

If you have an external DS18B20 soil temperature sensor connected to GPIO 21:

```cpp
const bool USE_18B20_TEMP_SENSOR = true;  // Set to true if available
```

### Step 8: Reporting Interval

Control how often the sensor sends data (in seconds):

```cpp
#define TIME_TO_SLEEP  3600  // 60 minutes
//#define TIME_TO_SLEEP  7200    // 120 minutes = 2h
//#define TIME_TO_SLEEP  10800   // 180 minutes = 3h
//#define TIME_TO_SLEEP  21600   // 360 minutes = 6h
```

### Step 9: Logging Configuration

For debugging and troubleshooting:

```cpp
const bool  logging = false;         // Enable serial logging
const bool  readLogfile = false;     // Read stored log from SPIFFS
const bool  deleteLogfile = false;   // Delete stored log from SPIFFS
```

**Note:** Enable only when debugging - logging significantly increases power consumption and drains battery faster.

### Step 10: Fertilizer Thresholds

Configure soil fertilizer (salt content) alert levels:

```cpp
int fertil_needed = 200;    // Below this: "needed"
int fertil_low = 201;       // 201-250: "low"
int fertil_opt = 251;       // 251-350: "optimal"
int fertil_high = 351;      // Above 351: "too high"
```

### Step 11: Soil Moisture Thresholds

```cpp
int water_min = 0;          // Minimum moisture level for alerts
int water_max = 9999;       // Maximum moisture level
```

### Step 12: Watering Valve Number (Optional)

If using with automatic watering system:

```cpp
int plantValveNo = 1;  // Valve number for this plant
```

## Home Assistant Auto-Discovery

### Discovery Process (v5.0.0+)

The firmware provides **native Home Assistant integration** using the ArduinoHA library:

1. **First Run**: Connect device to power with WiFi & MQTT configured
2. **Automatic Discovery**: Device publishes discovery topics to MQTT
3. **Sensor Creation**: All sensors automatically appear in Home Assistant
4. **Zero Configuration**: No manual entity setup needed in HA

### Sensors Published

- **Environment**: Temperature, Humidity, Pressure, Light (Lux)
- **Soil**: Moisture %, Raw values, Soil Temperature
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
platform = espressif32@6.8.1
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
