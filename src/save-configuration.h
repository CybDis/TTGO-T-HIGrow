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

struct HASensor {
  const char *key;
  const char *name;
  const char *unit;
  const char *device_class;
  const char *state_class;
  const char *icon;
};

void publishDiscovery(const String &chipId) {
  StaticJsonDocument<256> device;
  device["identifiers"][0] = chipId;
  device["manufacturer"] = "LILYGO, programmed by github.com/cybdis";
  device["model"] = "TTGO T-Higrow";
  device["name"] = plant_name;
  device["sw_version"] = rel;

  const HASensor sensors[] = {
      {"sensorname", "Name", nullptr, nullptr, nullptr, nullptr},
      {"Tgrow_HIGrow", "Mac-ID", nullptr, nullptr, nullptr, nullptr},
      {"updated", "Updated", nullptr, "timestamp", nullptr, "mdi:update"},
      {"sleep5Count", "Sleep5count", nullptr, nullptr, nullptr, "mdi:counter"},
      {"bootCount", "Bootcount", nullptr, nullptr, nullptr, "mdi:counter"},
      {"lux", "Lux", "lx", "illuminance", "measurement", "mdi:weather-sunny"},
      {"temp", "Temperature", "°C", "temperature", "measurement", "mdi:thermometer"},
      {"humid", "Humidity", "%", "humidity", "measurement", "mdi:water-percent"},
      {"soil", "Soil", "%", "moisture", "measurement", "mdi:raw"},
      {"soilTemp", "SoilTemp", "°C", "temperature", "measurement", "mdi:thermometer"},
      {"water", "Water", "%", nullptr, "measurement", "mdi:waves-arrow-up"},
      {"salt", "Fertilizer", "%", nullptr, "measurement", "mdi:bottle-tonic"},
      {"saltadvice", "Fertilize state", nullptr, nullptr, nullptr, "mdi:alpha-i-circle-outline"},
      {"bat", "Battery", "%", "battery", "measurement", "mdi:battery"},
      {"batcharge", "Charging", nullptr, nullptr, nullptr, "mdi:battery"},
      {"batchargeDate", "batchargeDate", nullptr, nullptr, nullptr, "mdi:calendar"},
      {"daysOnBattery", "daysOnBattery", "days", nullptr, nullptr, "mdi:calendar"},
      {"wifissid", "WIFI", nullptr, nullptr, nullptr, "mdi:wifi"},
      {"pressure", "Pressure", "Hpa", nullptr, "measurement", "mdi:gauge"},
      {"plantValveNo", "plantValveNo", nullptr, nullptr, nullptr, nullptr},
      {"rel", "Release", nullptr, nullptr, nullptr, "mdi:counter"}
  };

  for (const auto &s : sensors) {
    StaticJsonDocument<512> doc;
    doc["name"] = s.name;
    if (s.unit) doc["unit_of_meas"] = s.unit;
    if (s.device_class) doc["device_class"] = s.device_class;
    if (s.state_class) doc["state_class"] = s.state_class;
    if (s.icon) doc["icon"] = s.icon;
    doc["val_tpl"] = String("{{ value_json.plant.") + s.key + " }}";
    doc["uniq_id"] = String("TTGO_") + chipId + "_" + s.key;
    doc["stat_t"] = device_name + "/" + chipId;
    doc["dev"] = device;
    String topic = String("homeassistant/sensor/Tgrow_HIGrow_") + chipId + "/" + s.key + "/config";
    char buffer[512];
    serializeJson(doc, buffer);
    mqttClient.publish(topic.c_str(), buffer, true);
  }
}

// Allocate a  JsonDocument
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
  plant["sleep5Count"] = config.sleep5no;
  plant["bootCount"] = config.bootno;
  plant["lux"] = floatToString(config.lux);
  plant["temp"] = floatToString(config.temp);
  plant["humid"] = floatToString(config.humid);
  plant["soil"] = floatToString(config.soil);
  plant["soilTemp"] = floatToString(config.soilTemp);
  plant["water"] = floatToString(config.water);
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

  // Send to mqtt
  char buffer[1536];
  serializeJson(doc, buffer);


  Serial.print("  Sending message to topic: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Sending message to topic: \n");
  }

  Serial.println(buffer);

  // Connect to mqtt broker
  Serial.print("Attempting to connect to the MQTT broker: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Attempting to connect to the MQTT broker! \n");
  }

  Serial.println(broker);
  mqttClient.setServer(broker, port);

  if (!mqttClient.connect(broker, mqttuser, mqttpass)) {
    if (logging) {
      writeFile(SPIFFS, "/error.log", "MQTT connection failed! \n");
    }

    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());
    goToDeepSleepFiveMinutes();
  }

  if (logging) {
    writeFile(SPIFFS, "/error.log", "You're connected to the MQTT broker! \n");
  }

  Serial.println("You're connected to the MQTT broker! Publishing...");

  publishDiscovery(chipId);

  bool retained = true;

  if (mqttClient.publish(topic, buffer, retained)) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Error in Message, not published");
    goToDeepSleepFiveMinutes();
  }
  Serial.println();
}

