#include <ArduinoHA.h>

String floatToString(float value) {
  // Round the value to one decimal place
  float roundedValue = roundf(value * 10.0) / 10.0;

  // Check if the rounded value has a decimal place not equal to zero
  if (fabs(roundedValue - (int)roundedValue) >= 0.1) {
    // Convert the rounded value with decimal place into a string
    char buffer[10];
    sprintf(buffer, "%.1f", roundedValue);
    return String(buffer);
  } else {
    // Convert the value without decimal place into a string
    return String((int)value);
  }
}

// External reference to the shared ArduinoHA MQTT client (v5.0.0)
extern HAMqtt mqtt;

// Allocate a JsonDocument
void saveConfiguration(const Config & config) {

  byte mac[6];
  WiFi.macAddress(mac);

  //  String chipId = String(mac[0], HEX) + String(mac[1], HEX) + String(mac[2], HEX) + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  String chipId = "";
  String HEXcheck = "";
  for (int i = 0; i <= 5; i++) {
    HEXcheck = String(mac[i], HEX);
    if (HEXcheck.length() == 1) {
      chipId = chipId + "0" + String(mac[i], HEX);
    } else {
      chipId = chipId + String(mac[i], HEX);
    }
  }
  
  Serial.println("  chipId: " + chipId);
  const String topicStr = device_name + "/" + chipId;
  const char* topic = topicStr.c_str();
  Serial.print("  topic: ");
  Serial.println(topic);

  StaticJsonDocument<1536> doc;
  // Set the values in the document
  // Device changes according to device placement
  JsonObject root = doc.to<JsonObject>();

  JsonObject plant = root.createNestedObject("plant");
  plant[device_name] = chipId;
  plant["sensorname"] = plant_name;
  plant["updated"] = config.updated;
  // plant["date"] = config.date;
  // plant["time"] = config.time;
  plant["bootCount"] = config.bootno;
  plant["lux"] = floatToString(config.lux);
  plant["temp"] = floatToString(config.temp);
  plant["humid"] = floatToString(config.humid);
  plant["soil"] = floatToString(config.soil);
  plant["soilRaw"] = config.soilRaw;
  plant["soilCalibration"] = config.soilCalibration;
  plant["salt"] = floatToString(config.salt);
  plant["saltadvice"] = config.saltadvice;
  plant["bat"] = floatToString(config.bat);
  plant["batcharge"] = config.batcharge;
  plant["batchargeDate"] = config.batchargeDate;
  plant["daysOnBattery"] = floatToString(config.daysOnBattery);
  plant["battvolt"] = floatToString(config.batvolt);
  plant["battvoltage"] = floatToString(config.batvoltage);
  plant["pressure"] = floatToString(config.pressure);
  plant["plantValveNo"] = plantValveNo;
  plant["wifissid"] = WiFi.SSID();
  plant["rel"] = config.rel;

  // Send to mqtt using ArduinoHA mqtt client (v5.0.0 - unified MQTT handling)
  char buffer[1536];
  serializeJson(doc, buffer);

  Serial.print("  Sending message to topic: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Sending message to topic: \n");
  }

  Serial.println(buffer);

  // Publish via the same ArduinoHA mqtt client used for discovery, so there is
  // only ever one MQTT client driving the shared wifiClient connection.
  Serial.println("  Publishing via ArduinoHA MQTT client...");

  bool retained = true;

  if (mqtt.publish(topic, buffer, retained)) {
    Serial.println("  Message published successfully via ArduinoHA");
  } else {
    Serial.println("  Error in Message, not published via ArduinoHA");
    goToDeepSleep();
  }
  Serial.println();
}

