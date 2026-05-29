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
extern int plantValveNo;

// Utility function for sensor value formatting
String formatSensorValue(float value, bool isInteger = false) {
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
       globalSoilRawId, globalSoilTempId, globalSaltId, globalSaltAdvId, globalBatId, 
       globalBatChargeId, globalBatChargeDateId, globalDaysId, globalPressId, globalWifiId, 
       globalValveId, globalRelId, globalUpdId, globalSlp5Id, globalBootId;

HASensor* sensorLux;
HASensor* sensorTemp;
HASensor* sensorHumid;
HASensor* sensorSoil;
HASensor* sensorSoilRaw;
HASensor* sensorSoilTemp;
HASensor* sensorSalt;
HASensor* sensorSaltAdvice;
HASensor* sensorBat;
HASensor* sensorBatCharge;
HASensor* sensorBatChargeDate;
HASensor* sensorDaysOnBattery;
HASensor* sensorPressure;
HASensor* sensorName;
HASensor* sensorMacId;
HASensor* sensorUpdated;
HASensor* sensorSleep5Count;
HASensor* sensorBootCount;
HASensor* sensorWifiSSID;
HASensor* sensorPlantValveNo;
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
  
  globalSlp5Id = "TTGO_" + chipId + "_sleep5Count";
  sensorSleep5Count = new HASensor(globalSlp5Id.c_str());
  sensorSleep5Count->setName("Sleep5count");
  sensorSleep5Count->setUnitOfMeasurement("count");
  sensorSleep5Count->setIcon("mdi:counter");
  sensorSleep5Count->setStateClass("total_increasing");

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

  globalSoilTempId = "TTGO_" + chipId + "_soilTemp";
  sensorSoilTemp = new HASensor(globalSoilTempId.c_str());
  sensorSoilTemp->setName("SoilTemp");
  sensorSoilTemp->setUnitOfMeasurement("°C");
  sensorSoilTemp->setDeviceClass("temperature");
  sensorSoilTemp->setIcon("mdi:thermometer");
  sensorSoilTemp->setStateClass("measurement");
  sensorSoilTemp->setForceUpdate(true);

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
  sensorSaltAdvice->setForceUpdate(true);
  
  globalBatId = "TTGO_" + chipId + "_bat";
  sensorBat = new HASensor(globalBatId.c_str());
  sensorBat->setName("Battery");
  sensorBat->setUnitOfMeasurement("%");
  sensorBat->setDeviceClass("battery");
  sensorBat->setIcon("mdi:battery");
  sensorBat->setStateClass("measurement");
  sensorBat->setForceUpdate(true);

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
  
  globalValveId = "TTGO_" + chipId + "_plantValveNo";
  sensorPlantValveNo = new HASensor(globalValveId.c_str());
  sensorPlantValveNo->setName("plantValveNo");
  
  globalRelId = "TTGO_" + chipId + "_rel";
  sensorRelease = new HASensor(globalRelId.c_str());
  sensorRelease->setName("Release");
  sensorRelease->setIcon("mdi:counter");
  sensorRelease->setForceUpdate(true);
  
  Serial.println(F("Home Assistant Auto Discovery setup complete"));
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
  
  sensorSleep5Count->setValue(String(config.sleep5no).c_str());
  sensorBootCount->setValue(String(config.bootno).c_str());
  sensorLux->setValue(formatSensorValue(config.lux).c_str());  // 1 decimal place
  
  sensorTemp->setValue(formatSensorValue(config.temp).c_str());  // 1 decimal place
  
  sensorHumid->setValue(formatSensorValue(config.humid, true).c_str());  // integer
  
  sensorSoil->setValue(formatSensorValue(config.soil, true).c_str());  // integer
  
  sensorSoilRaw->setValue(config.soilRaw.c_str());
  sensorSoilTemp->setValue(formatSensorValue(config.soilTemp).c_str());  // 1 decimal place
  sensorSalt->setValue(formatSensorValue(config.salt, true).c_str());  // integer
  sensorSaltAdvice->setValue(config.saltadvice.c_str());
  
  sensorBat->setValue(formatSensorValue(config.bat, true).c_str());  // integer
  
  sensorBatCharge->setValue(config.batcharge.c_str());
  sensorBatChargeDate->setValue(config.batchargeDate.c_str());
  sensorDaysOnBattery->setValue(formatSensorValue(config.daysOnBattery).c_str());  // 1 decimal place
  sensorPressure->setValue(formatSensorValue(config.pressure, true).c_str());  // integer
  sensorWifiSSID->setValue(WiFi.SSID().c_str());
  sensorPlantValveNo->setValue(String(plantValveNo).c_str());
  sensorRelease->setValue(config.rel.c_str());
  
  // Process MQTT updates
  mqtt.loop();
}
