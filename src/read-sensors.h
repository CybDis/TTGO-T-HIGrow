// READ Sensors

// READ Salt
// I am not quite sure how to read and use this number. I know that when put in water wich a DH value of 26, it gives a high number, but what it is and how to use ??????
uint32_t readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++)
  {
    array[i] = analogRead(SALT_PIN);
  //  Serial.print("Read salt pin : ");

  //  Serial.println(array[i]);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++)
  {
    if (i == 0 || i == samples - 1)
      continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return humi;
}

// READ Soil
int16_t readSoil()
{
  uint16_t soil = analogRead(SOIL_PIN);
  Serial.print("CALIBRATE ===================> Current soil reading: ");
  Serial.print(soil);
  int result = map(soil, soil_min, soil_max, 100, 0);
  Serial.print(" - percent calculated: ");
  Serial.println(result);

  Serial.print(" Soil max (air): ");
  Serial.println(soil_max);
  Serial.print(" Sil min (water): ");
  Serial.println(soil_min);

  if (soil == 0) // ERROR
    return -1;
  if (result > 100)
    return 100;
  if (result < 0)
    return 0;
  return result;
}

float readSoilTemp()
{
  float temp;
  // READ Soil Temperature
  if (USE_18B20_TEMP_SENSOR)
  {
    //Single data stream upload
    temp = temp18B20.temp();
  }
  else
  {
    temp = 0.00;
  }
  return temp;
}

// READ Battery
float readBattery()
{
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);
  Serial.print("Volt direct: ");
  Serial.println(volt);
  config.batvolt = volt;
  float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
  config.batvoltage = battery_voltage;
  Serial.print("  Battery Voltage: ");
  Serial.println(battery_voltage);
  
  battery_voltage = battery_voltage * 100;
  float bat =  map(battery_voltage, 416, 290, 100, 0);
  Serial.print("  Battery level: ");
  Serial.println(bat);

  if (bat < 0)
    return 0;
  return bat;
}


#define WATERPOWER_PIN 17
#define WATERSIGNAL_PIN 36

// READ Water Level
uint16_t readWaterLevel()
{
  pinMode(WATERPOWER_PIN, OUTPUT);
  digitalWrite(WATERPOWER_PIN, HIGH);  // turn the sensor ON
  delay(10);
  uint16_t value= analogRead(WATERSIGNAL_PIN); // read the analog value from sensor
  uint16_t percent = map(value, water_min, water_max, 0, 100);
  Serial.print("CALIBRATE ===================> Water level sensor value: ");
  Serial.print(value);
  Serial.print(" - percent after map: ");
  Serial.println(percent);
  digitalWrite(WATERPOWER_PIN, LOW);  // turn the sensor off
  if (percent > 100)
    return 100;
  return percent;
}