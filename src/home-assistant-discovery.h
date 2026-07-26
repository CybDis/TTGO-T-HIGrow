/*
 * Home Assistant Auto Discovery Integration v5.0.0
 * 
 * This file implements native Home Assistant Auto Discovery using the ArduinoHA library.
 * It replaces the external Python script (TTGO-T-HiGrow-aut.py) dependency.
 * 
 * Features:
 * - Automatic device and sensor discovery in Home Assistant
 * - All sensor entities with proper device classes and icons
 * - Maintains compatibility with existing MQTT structure
 * - Runs directly on ESP32 without external dependencies
 * 
 * Created: September 2025
 * Author: GitHub Copilot Integration
 */

#include <ArduinoHA.h>
#include <WiFi.h>

// External references
extern String plant_name;

// Utility function for sensor value formatting
// Returns an empty String as a sentinel when the reading is not valid (e.g. a
// failed sensor transaction produced NAN), so callers can skip publishing
// this cycle instead of sending the literal "nan" over MQTT.
String formatSensorValue(float value, bool isInteger = false) {
  if (isnan(value)) {
    return String();
  }
  if (isInteger) {
    return String((int)value);
  } else {
    return String(value, 1);  // 1 decimal place
  }
}

// Home Assistant Auto Discovery
HADevice device;
HAMqtt mqtt(wifiClient, device);

// HA Sensors - Store IDs as global variables to prevent memory issues
String globalNameId, globalMacId, globalLuxId, globalTempId, globalHumId, globalSoilId,
       globalSoilRawId, globalSoilCalId, globalSaltId, globalSaltAdvId, globalBatId,
       globalRawBattAdcId, globalRawBattVoltageId, globalBatChargeId, globalBatChargeDateId,
       globalDaysId, globalPressId, globalWifiId, globalRelId, globalUpdId, globalBootId;

HASensor* sensorLux;
HASensor* sensorTemp;
HASensor* sensorHumid;
HASensor* sensorSoil;
HASensor* sensorSoilRaw;
HASensor* sensorSoilCalibration;
HASensor* sensorSalt;
HASensor* sensorSaltAdvice;
HASensor* sensorBat;
HASensor* sensorRawBattAdc;
HASensor* sensorRawBattVoltage;
HASensor* sensorBatCharge;
HASensor* sensorBatChargeDate;
HASensor* sensorDaysOnBattery;
HASensor* sensorPressure;
HASensor* sensorName;
HASensor* sensorMacId;
HASensor* sensorUpdated;
HASensor* sensorBootCount;
HASensor* sensorWifiSSID;
HASensor* sensorRelease;

void sendDiscoveryTopic() {
  Serial.println(F("Setting up Home Assistant Auto Discovery..."));
  
  // Get MAC address for unique device identifier
  uint8_t macBytes[6];
  WiFi.macAddress(macBytes);
  String chipId = "";
  String HEXcheck = "";
  for (int i = 0; i <= 5; i++) {
    HEXcheck = String(macBytes[i], HEX);
    if (HEXcheck.length() == 1) {
      chipId = chipId + "0" + String(macBytes[i], HEX);
    } else {
      chipId = chipId + String(macBytes[i], HEX);
    }
  }
  
  // Setup device with Tgrow_HIGrow prefix for compatibility
  String deviceId = "Tgrow_HIGrow_" + chipId;
  device.setUniqueId(macBytes, sizeof(macBytes));
  device.setName(plant_name.c_str());
  device.setManufacturer("LILYGO, programmed by github.com/cybdis");
  device.setModel("TTGO T-Higrow");
  device.setSoftwareVersion(rel.c_str());
  
  // Setup MQTT with custom topic prefix for compatibility
  mqtt.begin(broker, mqttuser, mqttpass);
  // Explicit buffer size: the legacy combined-JSON topic (save-configuration.h)
  // and the discovery payloads are streamed straight to the socket, but the
  // fixed header/topic buffer should still comfortably fit the largest topic
  // + header. Made explicit rather than relying on PubSubClient's default.
  mqtt.setBufferSize(2048);
  
  // Create all sensors with TTGO prefix and chipId for compatibility - FIXED MEMORY ISSUE
  globalNameId = "TTGO_" + chipId + "_sensorname";
  sensorName = new HASensor(globalNameId.c_str());
  sensorName->setName("Name");
  sensorName->setIcon("mdi:label");
  
  globalMacId = "TTGO_" + chipId + "_Tgrow_HIGrow";
  sensorMacId = new HASensor(globalMacId.c_str());
  sensorMacId->setName("Mac-ID");
  sensorMacId->setIcon("mdi:identifier");
  
  globalUpdId = "TTGO_" + chipId + "_updated";
  sensorUpdated = new HASensor(globalUpdId.c_str());
  sensorUpdated->setName("Updated");
  sensorUpdated->setDeviceClass("timestamp");
  sensorUpdated->setIcon("mdi:update");
  sensorUpdated->setForceUpdate(true);
  
  globalBootId = "TTGO_" + chipId + "_bootCount";
  sensorBootCount = new HASensor(globalBootId.c_str());
  sensorBootCount->setName("Bootcount");
  sensorBootCount->setUnitOfMeasurement("count");
  sensorBootCount->setIcon("mdi:counter");
  sensorBootCount->setStateClass("total_increasing");
  sensorBootCount->setForceUpdate(true);
  
  globalLuxId = "TTGO_" + chipId + "_lux";
  sensorLux = new HASensor(globalLuxId.c_str());
  sensorLux->setName("Lux");
  sensorLux->setUnitOfMeasurement("lx");
  sensorLux->setDeviceClass("illuminance");
  sensorLux->setIcon("mdi:weather-sunny");
  sensorLux->setStateClass("measurement");
  sensorLux->setForceUpdate(true);

  globalTempId = "TTGO_" + chipId + "_temp";
  sensorTemp = new HASensor(globalTempId.c_str());
  sensorTemp->setName("Temperature");
  sensorTemp->setUnitOfMeasurement("°C");
  sensorTemp->setDeviceClass("temperature");
  sensorTemp->setIcon("mdi:thermometer");
  sensorTemp->setStateClass("measurement");
  sensorTemp->setForceUpdate(true);

  globalHumId = "TTGO_" + chipId + "_humid";
  sensorHumid = new HASensor(globalHumId.c_str());
  sensorHumid->setName("Humidity");
  sensorHumid->setUnitOfMeasurement("%");
  sensorHumid->setDeviceClass("humidity");
  sensorHumid->setIcon("mdi:water-percent");
  sensorHumid->setStateClass("measurement");
  sensorHumid->setForceUpdate(true);

  globalSoilId = "TTGO_" + chipId + "_soil";
  sensorSoil = new HASensor(globalSoilId.c_str());
  sensorSoil->setName("Soil");
  sensorSoil->setUnitOfMeasurement("%");
  sensorSoil->setDeviceClass("moisture");
  sensorSoil->setIcon("mdi:water-percent");
  sensorSoil->setStateClass("measurement");
  sensorSoil->setForceUpdate(true);

  globalSoilRawId = "TTGO_" + chipId + "_soilRaw";
  sensorSoilRaw = new HASensor(globalSoilRawId.c_str());
  sensorSoilRaw->setName("SoilRaw");
  sensorSoilRaw->setIcon("mdi:raw");
  sensorSoilRaw->setStateClass("measurement");
  sensorSoilRaw->setForceUpdate(true);

  globalSoilCalId = "TTGO_" + chipId + "_soilCalibration";
  sensorSoilCalibration = new HASensor(globalSoilCalId.c_str());
  sensorSoilCalibration->setName("SoilCalibration");
  sensorSoilCalibration->setIcon("mdi:tune");

  globalSaltId = "TTGO_" + chipId + "_salt";
  sensorSalt = new HASensor(globalSaltId.c_str());
  sensorSalt->setName("Fertilizer");
  sensorSalt->setUnitOfMeasurement("%");
  sensorSalt->setIcon("mdi:bottle-tonic");
  sensorSalt->setStateClass("measurement");
  sensorSalt->setForceUpdate(true);

  globalSaltAdvId = "TTGO_" + chipId + "_saltadvice";
  sensorSaltAdvice = new HASensor(globalSaltAdvId.c_str());
  sensorSaltAdvice->setName("Fertilize state");
  sensorSaltAdvice->setIcon("mdi:alpha-i-circle-outline");

  globalBatId = "TTGO_" + chipId + "_bat";
  sensorBat = new HASensor(globalBatId.c_str());
  sensorBat->setName("Battery");
  sensorBat->setUnitOfMeasurement("%");
  sensorBat->setDeviceClass("battery");
  sensorBat->setIcon("mdi:battery");
  sensorBat->setStateClass("measurement");
  sensorBat->setForceUpdate(true);

  globalRawBattAdcId = "TTGO_" + chipId + "_RawBattAdc";
  sensorRawBattAdc = new HASensor(globalRawBattAdcId.c_str());
  sensorRawBattAdc->setName("RawBattAdc");
  sensorRawBattAdc->setIcon("mdi:gauge");
  sensorRawBattAdc->setStateClass("measurement");
  sensorRawBattAdc->setForceUpdate(true);

  globalRawBattVoltageId = "TTGO_" + chipId + "_RawBattVoltage";
  sensorRawBattVoltage = new HASensor(globalRawBattVoltageId.c_str());
  sensorRawBattVoltage->setName("RawBattVoltage");
  sensorRawBattVoltage->setUnitOfMeasurement("V");
  sensorRawBattVoltage->setDeviceClass("voltage");
  sensorRawBattVoltage->setIcon("mdi:flash");
  sensorRawBattVoltage->setStateClass("measurement");
  sensorRawBattVoltage->setForceUpdate(true);

  globalBatChargeId = "TTGO_" + chipId + "_batcharge";
  sensorBatCharge = new HASensor(globalBatChargeId.c_str());
  sensorBatCharge->setName("Charging");
  sensorBatCharge->setIcon("mdi:battery");
  
  globalBatChargeDateId = "TTGO_" + chipId + "_batchargeDate";
  sensorBatChargeDate = new HASensor(globalBatChargeDateId.c_str());
  sensorBatChargeDate->setName("batchargeDate");
  sensorBatChargeDate->setIcon("mdi:calendar");
  sensorBatChargeDate->setForceUpdate(true);
  
  globalDaysId = "TTGO_" + chipId + "_daysOnBattery";
  sensorDaysOnBattery = new HASensor(globalDaysId.c_str());
  sensorDaysOnBattery->setName("daysOnBattery");
  sensorDaysOnBattery->setUnitOfMeasurement("days");
  sensorDaysOnBattery->setIcon("mdi:calendar");
  sensorDaysOnBattery->setStateClass("total_increasing");
  sensorDaysOnBattery->setForceUpdate(true);

  globalPressId = "TTGO_" + chipId + "_pressure";
  sensorPressure = new HASensor(globalPressId.c_str());
  sensorPressure->setName("Pressure");
  sensorPressure->setUnitOfMeasurement("hPa");
  sensorPressure->setDeviceClass("pressure");
  sensorPressure->setIcon("mdi:gauge");
  sensorPressure->setStateClass("measurement");
  sensorPressure->setForceUpdate(true);
  
  globalWifiId = "TTGO_" + chipId + "_wifissid";
  sensorWifiSSID = new HASensor(globalWifiId.c_str());
  sensorWifiSSID->setName("WIFI");
  sensorWifiSSID->setIcon("mdi:wifi");
  
  globalRelId = "TTGO_" + chipId + "_rel";
  sensorRelease = new HASensor(globalRelId.c_str());
  sensorRelease->setName("Release");
  sensorRelease->setIcon("mdi:counter");
  sensorRelease->setForceUpdate(true);
  
  Serial.println(F("   Home Assistant Auto Discovery setup complete"));
}

void updateHASensors(const Config& config) {
  // Ensure MQTT connection is active
  if (!mqtt.isConnected()) {
    mqtt.loop();
    if (!mqtt.isConnected()) {
      return;
    }
  }
  
  // Update all sensor values
  sensorName->setValue(plant_name.c_str());
  
  uint8_t macBytes[6];
  WiFi.macAddress(macBytes);
  String chipId = "";
  String HEXcheck = "";
  for (int i = 0; i <= 5; i++) {
    HEXcheck = String(macBytes[i], HEX);
    if (HEXcheck.length() == 1) {
      chipId = chipId + "0" + String(macBytes[i], HEX);
    } else {
      chipId = chipId + String(macBytes[i], HEX);
    }
  }
  sensorMacId->setValue(chipId.c_str());
  
  sensorUpdated->setValue(config.updated.c_str());
  
  sensorBootCount->setValue(String(config.bootno).c_str());

  // Format each float-backed sensor once; formatSensorValue() returns an
  // empty String when the reading is invalid (NAN), which both setValue()
  // and mqttLog() below use as the signal to skip this sensor for the
  // cycle, leaving its previously retained MQTT value in place.
  String luxVal          = formatSensorValue(config.lux);            // 1 decimal place
  String tempVal         = formatSensorValue(config.temp);           // 1 decimal place
  String humidVal        = formatSensorValue(config.humid, true);    // integer
  String soilVal         = formatSensorValue(config.soil, true);     // integer
  String saltVal         = formatSensorValue(config.salt, true);     // integer
  String batVal          = formatSensorValue(config.bat, true);      // integer
  String battAdcVal      = formatSensorValue(config.batvolt, true);
  String battVoltageVal  = formatSensorValue(config.batvoltage);
  String daysVal         = formatSensorValue(config.daysOnBattery);  // 1 decimal place
  String pressureVal     = formatSensorValue(config.pressure, true); // integer

  if (luxVal.length() > 0)         sensorLux->setValue(luxVal.c_str());
  if (tempVal.length() > 0)        sensorTemp->setValue(tempVal.c_str());
  if (humidVal.length() > 0)       sensorHumid->setValue(humidVal.c_str());
  if (soilVal.length() > 0)        sensorSoil->setValue(soilVal.c_str());

  sensorSoilRaw->setValue(config.soilRaw.c_str());
  sensorSoilCalibration->setValue(config.soilCalibration.c_str());
  if (saltVal.length() > 0)        sensorSalt->setValue(saltVal.c_str());
  sensorSaltAdvice->setValue(config.saltadvice.c_str());

  if (batVal.length() > 0)         sensorBat->setValue(batVal.c_str());
  if (battAdcVal.length() > 0)     sensorRawBattAdc->setValue(battAdcVal.c_str());
  if (battVoltageVal.length() > 0) sensorRawBattVoltage->setValue(battVoltageVal.c_str());

  sensorBatCharge->setValue(config.batcharge.c_str());
  sensorBatChargeDate->setValue(config.batchargeDate.c_str());
  if (daysVal.length() > 0)        sensorDaysOnBattery->setValue(daysVal.c_str());
  if (pressureVal.length() > 0)    sensorPressure->setValue(pressureVal.c_str());
  sensorWifiSSID->setValue(WiFi.SSID().c_str());
  sensorRelease->setValue(config.rel.c_str());

  // Print MQTT payload summary to Serial (labels from discovery getName())
  auto mqttLog = [](const HASensor* s, const String& val) {
    Serial.print(F("  ")); Serial.print(s->getName()); Serial.print(F(": ")); Serial.println(val);
  };
  // Same as mqttLog(), but for a float-backed sensor whose formatted value
  // may be an empty sentinel (invalid/NAN reading) - logs the skip instead.
  auto mqttLogOrSkip = [&mqttLog](const HASensor* s, const String& val) {
    if (val.length() > 0) {
      mqttLog(s, val);
    } else {
      Serial.print(F("  ")); Serial.print(s->getName());
      Serial.println(F(": skipped (invalid/NAN reading, retaining previous value)"));
    }
  };
  Serial.println(F("\n--- MQTT payload ---"));
  mqttLog(sensorName,          plant_name);
  mqttLog(sensorMacId,         chipId);
  mqttLog(sensorUpdated,       config.updated);
  mqttLog(sensorBootCount,     String(config.bootno));
  mqttLogOrSkip(sensorLux,     luxVal);
  mqttLogOrSkip(sensorTemp,    tempVal);
  mqttLogOrSkip(sensorHumid,   humidVal);
  mqttLogOrSkip(sensorSoil,    soilVal);
  mqttLog(sensorSoilRaw,          config.soilRaw);
  mqttLog(sensorSoilCalibration,  config.soilCalibration);
  mqttLogOrSkip(sensorSalt,    saltVal);
  mqttLog(sensorSaltAdvice,    config.saltadvice);
  mqttLogOrSkip(sensorBat,            batVal);
  mqttLogOrSkip(sensorRawBattAdc,     battAdcVal);
  mqttLogOrSkip(sensorRawBattVoltage, battVoltageVal);
  mqttLog(sensorBatCharge,     config.batcharge);
  mqttLog(sensorBatChargeDate, config.batchargeDate);
  mqttLogOrSkip(sensorDaysOnBattery, daysVal);
  mqttLogOrSkip(sensorPressure,      pressureVal);
  mqttLog(sensorWifiSSID,      WiFi.SSID());
  mqttLog(sensorRelease,       config.rel);
  Serial.println(F("--------------------"));

  // Process MQTT updates
  mqtt.loop();

  Serial.println(F("MQTT update sent."));
}
