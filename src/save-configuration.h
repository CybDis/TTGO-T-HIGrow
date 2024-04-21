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
  
  bool retained = true;

  if (mqttClient.publish(topic, buffer, retained)) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Error in Message, not published");
    goToDeepSleepFiveMinutes();
  }
  Serial.println();
}

