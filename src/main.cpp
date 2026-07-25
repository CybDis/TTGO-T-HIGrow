#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <Esp.h>
#include <time.h>
#include <TimeLib.h>

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include "user-variables.h"
#include <18B20_class.h>
#include <Adafruit_BME280.h>

// Logfile on SPIFFS
#include "SPIFFS.h"

//           rel = "2.0;    // Implemented MAC id as unique identifier for the device, at same time device_name is frozen to Tgrow_HIGrow.
//           rel = "2.0.1"; // Implemented "_" + name index for sensor icon. Corrected missing leading zero in HEX address.
//           rel = "2.0.2"; // Implemented automatic search for feaseable WIFI SSID, and connect to this.
//           rel = "3.0.0"; // Implemented Home-Assistant MQTT Autodiscover.
//           rel = "3.0.1"; // Implemented Home-Assistant MQTT Autodiscover, Salt calibration and advice included.
//           rel = "3.0.2"; // DST switch over now works
//           rel = "3.0.3"; // Small error corrections
//           rel = "3.0.4"; // Adapting to HACS frontend card: Battery State Card
//           rel = "3.0.5"; // Implemented name of plant saved to SPIFFS
//           rel = "3.0.6"; // DST calculation was way wrong, corrected now.
//           rel = "4.0.0"; // Changed from Arduino EDI to VS Code - PlatformIO
//           rel = "4.0.1"; // Error correction in connect network
//           rel = "4.0.2"; // Organising subroutines, and functional code snippets.
//           rel = "4.0.3"; // Adding battery charged date, and days since last charge
//           rel = "4.0.4"; // Adding battery charged date, and days since last charge, added to SPIFFS so that data do not dissapear at reboot.
//           rel = "4.0.5"; // Merged change from @reenari, and corrected counter days since last change
//           rel = "4.0.6"; // Corrected counter days !!! AGAIN !!!
//           rel = "4.0.7"; // The plant name is now used as hostname, so it is more visible in your router
//           rel = "4.1.0"; // Possibility to add the external 18B20 temperature sensor
//           rel = "4.2.0"; // BME280 sensor implemented
//           rel = "4.2.2"; // For the Greenhouse auto watering, the plantValveNo have been introduced. (Greenhouse auto watering is in development)
//           rel = "4.2.3"; // Removed the battery day counter - for good, use BeardedTingers solution if you need it.
//           rel = "4.3.1"; // Finally the days since last charging works correctly.
//           rel = "4.3.2"; // Corrected an error in DST.
//           rel = "4.5.0"; // Soil limited to Min/Max 0/100%
//           rel = "4.5.1"; // Intervall to 2h
//           rel = "4.6.0"; // Included external water level sensor and optimized debug output
//           rel = "4.6.1"; // Removed Date/Time and replaced with Updated (timestamp)
//           rel = "4.6.2"; // ...
//           rel = "4.6.3"; // Fixed limits (batt <0, soil not readable...)
//           rel = "4.6.4"; // Removed obsolete DST code
//           rel = "4.6.5"; // fixed unsigned integer on soil not readable, added 'soilRaw' reading
//           rel = "4.6.6"; // read battery direkt on start of setup
//           rel = "4.6.7"; // external wake deactivated, as it is not working on HiGrow3, and we do not use it anyways
//           rel = "4.7.0"; // removed external water level sensor
//           rel = "5.0.0"; // MAJOR: Native Home Assistant Auto Discovery integration with ArduinoHA library, replaces external Python script dependency
//           rel = "5.0.1"; // Add state_class (measurement/total_increasing) to all HA Discovery payloads; unit "count" for boot-/sleep5Count; fix pressure unit Hpa→hPa + device_class
//           rel = "5.1.0"; // Enable force_update on all measurement sensors in HA Discovery so last_changed updates even when value stays the same
//           rel = "5.2.0"; // Local-Only no-internet-mode supported by using IP of local ntp server, e.g. Fritzbox.
//           rel = "5.2.1"; // Optimized serial debug outputs and removed unnecessary sleeps
//           rel = "5.3.0"; // Charging mode: sleep reduced to 5 minutes while charging. Removed SoulTemp as there is no sensor for it. Sending SoilCalibration value to compare to SoilRaw when on battery and measurements are strange
//           rel = "5.4.0"; // OTA update: pull firmware from Home Assistant /local on every wake, deploy via "pio run -t ota_deploy"
//           rel = "5.4.1"; // Soil/Salt measured BEFORE WiFi (weak battery + WiFi load sags sensor rail, false 100% soil); soil now uses the same 120-sample trimmed mean as salt
//           rel = "5.5.0"; // Wake-ups aligned to the clock grid: sleep duration is computed to the next full hour (3600), half hour (1800), etc. based on NTP time
//           rel = "5.5.1"; // Fixed corrupted/truncated HA discovery MQTT payloads: removed a second, never-connected PubSubClient that was injecting publishes onto ArduinoHA's live MQTT session on the same socket; legacy JSON topic now published via the single shared ArduinoHA client
//           rel = "5.5.2"; // Skipped locally/remotely due to conflicting unreleased cleanup builds
//           rel = "5.5.3"; // Removed unused plantValveNo and dead Sleep5Count telemetry
const String rel = "5.5.5"; // Added RawBattAdc/RawBattVoltage HA Discovery sensors exposing the raw battery ADC reading and computed voltage as native HA entities; RawBattAdc has no unit of measurement, matching SoilRaw (both are raw ADC counts)

// mqtt constants
WiFiClient wifiClient;

// Date calculator
unsigned long epochTime;
String battChargeEpoc;
unsigned long epochChargeTime;
float battChargeDateDivider = 86400;
float daysOnBattery;

// Reboot counters
RTC_DATA_ATTR int bootCount = 0;

// Sensor bools
bool bme_found = false;

// json construct setup
struct Config
{
  // String date;
  // String time;
  String updated;
  int bootno;
  float lux;
  float temp;
  float humid;
  float soil;
  String soilRaw;
  String soilCalibration;
  float salt;
  String saltadvice;
  float bat;
  String batcharge;
  String batchargeDate;
  float daysOnBattery;
  float batvolt;
  float batvoltage;
  float pressure;
  String rel;
};
Config config;

const int led = 13;

#define I2C_SDA 25
#define I2C_SCL 26
#define DHT_PIN 16
#define BAT_ADC 33
#define SALT_PIN 34
#define SOIL_PIN 32
#define BOOT_PIN 0
#define POWER_CTRL 4
#define USER_BUTTON 35
#define DS18B20_PIN 21

BH1750 lightMeter(0x23); // 0x23
Adafruit_BME280 bmp;     // 0x77

DHT dht(DHT_PIN, DHT_TYPE);
DS18B20 temp18B20(DS18B20_PIN);

WiFiUDP ntpUDP;
NTPClient* timeClient = nullptr;
String dayStamp;
bool ntpOk = false;

// Sleep until the next clock-grid point (full hour for 3600 s, half hour for
// 1800 s, every second full hour for 7200 s, ...). Works for any interval
// because the Unix epoch is aligned to midnight UTC. Falls back to the raw
// interval when no valid NTP time is available.
uint32_t alignedSleepSeconds(uint32_t intervalSeconds, bool timeValid)
{
  if (!timeValid || intervalSeconds == 0)
  {
    return intervalSeconds;
  }
  const uint32_t MIN_SLEEP_S = 120;
  unsigned long now = timeClient->getEpochTime();
  uint32_t remaining = intervalSeconds - (uint32_t)(now % intervalSeconds);
  if (remaining < MIN_SLEEP_S)
  {
    // Too close to the next grid point - skip to the one after it so the
    // device does not wake up again almost immediately.
    remaining += intervalSeconds;
  }
  return remaining;
}

// Start Subroutines
#include <file-management.h>
#include <go-to-deep-sleep.h>
#include <get-string-value.h>
#include <read-sensors.h>
#include <save-configuration.h>
#include <connect-to-network.h>
#include <ota-update.h>
#include <read-batt-info.h>
#include <floatConv.h>
#include <home-assistant-discovery.h>    // NEW v5.0.0: Home Assistant Auto Discovery functions

void setup()
{
  //! Sensor power control pin, use deteced must set high
  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, 1);
  delay(1000);

  Serial.begin(115200);
  Serial.println("Starting setup...");
  Serial.println();
  
  float bat = readBattery();
  
  // Initialize SPIFFS and manage saved parameters from user variables
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    if (logging)
    {
      writeFile(SPIFFS, "/error.log", "An Error has occurred while mounting SPIFFS \n");
    }
    return;
  }
  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Void Setup \n");
  }

  listDir(SPIFFS, "/", 0);

  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "After listDir \n");
  }

  if (readLogfile)
  {
    // Now we start reading the files..
    readFile(SPIFFS, "/error.log");
    Serial.println("Here STARTS the logging info:");
    Serial.println(readString);
    Serial.println("Here ENDS the logging info:");
  }

  if (deleteLogfile)
  {
    SPIFFS.remove("/error.log");
  }

  // Calibrate soil figures save to file, if calibrate_soil == true
  if (calibrate_soil)
  {
    SPIFFS.remove("/soil.conf");
    String soil_write_str = String(soil_min) + ":" + String(soil_max);
    config.soilCalibration = soil_write_str;
    const char *soil_write = soil_write_str.c_str();
    writeFile(SPIFFS, "/soil.conf", soil_write);
  }
  else
  {
    Serial.println("Reading /soil.conf");
    readFile(SPIFFS, "/soil.conf");
    Serial.print("   Persisted calibration info: ");
    Serial.println(readString);
    String xval = getValue(readString, ':', 0);
    String yval = getValue(readString, ':', 1);

    soil_min = xval.toInt();
    soil_max = yval.toInt();
    config.soilCalibration = xval + ":" + yval;
    readString = "";
  }

  if (update_plant_name)
  {
    SPIFFS.remove("/name.conf");
    String name_write_str = plant_name;
    const char *name_write = name_write_str.c_str();
    writeFile(SPIFFS, "/name.conf", name_write);
  }
  else
  {
    Serial.println("Reading /name.conf");
    readFile(SPIFFS, "/name.conf");
    plant_name = readString;
    readString = "";
  }

  Serial.print("   Persisted plant name: ");
  Serial.println(plant_name);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Before Start WIFI \n");
  }

  read_batt_info();

  // Measure soil and salt BEFORE WiFi starts: the analog sensor chain is
  // supplied from the battery rail, and on a weak battery the WiFi TX bursts
  // sag that rail. The ADC reference is fixed, so a sagging rail shrinks the
  // raw readings and soil falsely drifts towards 100%.
  int16_t soil = readSoil();
  config.soil = soil;
  config.soilRaw = soilRead;

  uint32_t salt = readSalt();
  config.salt = salt;
  Serial.print("Salt (raw): ");
  Serial.println(salt);

  String advice;
  if (salt < 201)
  {
    advice = "needed";
  }
  else if (salt < 251)
  {
    advice = "low";
  }
  else if (salt < 351)
  {
    advice = "optimal";
  }
  else if (salt > 350)
  {
    advice = "too high";
  }
  Serial.print("Salt Advice: ");
  Serial.println(advice);
  config.saltadvice = advice;

  // Start WiFi and update time
  connectToNetwork();
  Serial.println(" ");
  Serial.println("  Connected to network!");
  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Connected to network \n");
  }

  Serial.print("  MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("  IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("  DNS: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("  Hostname: ");
  Serial.println(WiFi.getHostname());

  // OTA check before NTP/measurement/MQTT: on update the device flashes and
  // reboots immediately, the new firmware then does measurement and publish.
  checkForOtaUpdate(rel);

  IPAddress ntpServerIPAddress;
  bool hasNtpServerIp = strlen(ntpServerIp) > 0;
  if (hasNtpServerIp && ntpServerIPAddress.fromString(ntpServerIp))
  {
    timeClient = new NTPClient(ntpUDP, ntpServerIPAddress);
    Serial.print(F("  Using local NTP IP: "));
    Serial.println(ntpServerIp);
  }
  else
  {
    if (hasNtpServerIp)
    {
      Serial.print(F("Invalid ntpServerIp, falling back to hostname: "));
      Serial.println(ntpServerIp);
    }
    timeClient = new NTPClient(ntpUDP, ntpServer);
    Serial.print(F("  Using NTP server: "));
    Serial.println(ntpServer);
  }

  // DEBUG: Start NTP synchronization
  Serial.println(F("\n  [NTP] Starting time synchronization..."));
  unsigned long ntpStart = millis();
  ntpOk = false;
  while (millis() - ntpStart < 20000UL)
  {
    if (timeClient->update())
    {
      ntpOk = true;
      unsigned long ntpDuration = millis() - ntpStart;
      Serial.print(F("  [NTP] SUCCESS after "));
      Serial.print(ntpDuration);
      Serial.println(F("ms"));
      break;
    }
    timeClient->forceUpdate();
  }
  if (!ntpOk)
  {
    unsigned long ntpDuration = millis() - ntpStart;
    Serial.print(F("  [NTP] TIMEOUT after "));
    Serial.print(ntpDuration);
    Serial.println(F("ms - continuing without time sync"));
  }

  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  String formattedDate = timeClient->getFormattedDate();
  config.updated = formattedDate;

  // We need to extract date and time
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  dayStamp = dayStamp.substring(5);

  //#include <battChargeDays.h>
  if (dht_found)
  {
    dht.begin();
  }
  else
  {
    Serial.println(F("Could not find a valid DHT sensor, check if there is one present on board!"));
  }

  bool wireOk = Wire.begin(I2C_SDA, I2C_SCL); // wire can not be initialized at beginng, the bus is busy
  if (wireOk)
  {
    Serial.println(F("Wire ok"));
    if (logging)
    {
      writeFile(SPIFFS, "/error.log", "Wire Begin OK! \n");
    }
  }
  else
  {
    Serial.println(F("Wire NOK"));
  }

  if (bme_found && !bmp.begin())
  {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    bme_found = false;
  }

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else
  {
    Serial.println(F("Error initialising BH1750"));
  }

  float luxRead = lightMeter.readLightLevel(); // 1st read seems to return 0 always
  // Serial.print("  lux first read is 0: ");
  // Serial.println(luxRead);
  
  // // wozu das delay?? weiß ich nimmer! lieber mal da lassen.
  // Serial.println("wait 2 seconds...");
  // delay(2000);

  if (dht_found)
  {
    float t12 = dht.readTemperature(); // Read temperature as Fahrenheit then dht.readTemperature(true)
    config.temp = t12;
    delay(2000);
    float h12 = dht.readHumidity();
    config.humid = h12;
  }

  if (bme_found)
  {
    float bme_temp = bmp.readTemperature();
    config.temp = bme_temp;

    float bme_humid = bmp.readHumidity();
    config.humid = bme_humid;

    float bme_pressure = (bmp.readPressure() / 100.0F);
    config.pressure = bme_pressure;
  }

  // Battery status, and charging status and days.
  config.bat = bat;
  config.batcharge = "";
  if (bat > 120)
  {
    config.batcharge = "charging";
    SPIFFS.remove("/batinfo.conf");
    epochChargeTime = timeClient->getEpochTime();
    battChargeEpoc = String(epochChargeTime) + ":" + String(dayStamp);
    const char *batinfo_write = battChargeEpoc.c_str();
    writeFile(SPIFFS, "/batinfo.conf", batinfo_write);
    Serial.print("charging dayStamp: ");
    Serial.println(dayStamp);
    config.batchargeDate = dayStamp;
  }

  Serial.print("Charge Epoc: ");
  Serial.println(battChargeEpoc);
  unsigned long epochTime = timeClient->getEpochTime();
  Serial.print("  Test Epoc: ");
  Serial.println(epochTime);
  epochChargeTime = battChargeEpoc.toInt();
  Serial.print("  First time calculation: ");
  Serial.println(epochTime - epochChargeTime);
  float epochTimeFl = float(epochTime);
  float epochChargeTimeFl = float(epochChargeTime);
    
  daysOnBattery = (epochTimeFl - epochChargeTimeFl) / battChargeDateDivider;
  daysOnBattery = truncate(daysOnBattery, 1);
  config.daysOnBattery = daysOnBattery;

  if (bat > 100)
  {
    config.bat = 100;
  }

  config.bootno = bootCount;

  luxRead = lightMeter.readLightLevel();
  Serial.print("lux: ");
  Serial.println(luxRead);
  config.lux = luxRead;
  config.rel = rel;
  
  // *** NEW in v5.0.0: Native Home Assistant Auto Discovery Integration ***
  // Setup Home Assistant Auto Discovery using ArduinoHA library
  // This replaces the need for external Python script (TTGO-T-HiGrow-aut.py)
  // All sensors are automatically discovered and configured in Home Assistant
  sendDiscoveryTopic();
  
  // Update HA sensor values with current readings
  // Sends all sensor data directly to Home Assistant via MQTT Discovery
  updateHASensors(config);
  
  // Send traditional MQTT message (maintained for backward compatibility)
  // This preserves existing MQTT subscriptions and external integrations
  // saveConfiguration(config);

  // Go to sleep
  // Increment boot number and print it every reboot
  ++bootCount;
  Serial.println();
  Serial.print(plant_name);
  Serial.println(" Boot number: " + String(bootCount));

  // Go to sleep now
  delay(200);
  uint32_t sleepTime = (config.batcharge == "charging")
                         ? 300
                         : alignedSleepSeconds(TIME_TO_SLEEP, ntpOk);
  goToDeepSleep(sleepTime);
}

void loop()
{
}
