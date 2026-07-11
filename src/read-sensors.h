// READ Sensors
uint16_t soilRead;

// Trimmed mean over many ADC samples: the ESP32 ADC is noisy (+-20..50 counts),
// so sort the samples, drop the extremes and average the rest.
uint16_t readAnalogTrimmedMean(uint8_t pin)
{
  const uint8_t samples = 120;
  uint16_t array[120];
  uint32_t sum = 0;

  for (int i = 0; i < samples; i++)
  {
    array[i] = analogRead(pin);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 1; i < samples - 1; i++)
  {
    sum += array[i];
  }
  return sum / (samples - 2);
}

// READ Salt
// I am not quite sure how to read and use this number. I know that when put in water wich a DH value of 26, it gives a high number, but what it is and how to use ??????
uint32_t readSalt()
{
  return readAnalogTrimmedMean(SALT_PIN);
}

// READ Soil
int16_t readSoil()
{

  int loop = 1;
  int result = -1;
  if (calibrate_soil)
  {
    loop = 30;
    Serial.println("Calibrating soil sensor...");
  }

  Serial.print(" Soil max (air): ");
  Serial.println(soil_max);
  Serial.print(" Soil min (water): ");
  Serial.println(soil_min);

  for (int i = 0; i < loop; i++)
  {
    soilRead = readAnalogTrimmedMean(SOIL_PIN);
    if (calibrate_soil)
      Serial.print(i+1);
    Serial.print(" CALIBRATE ===================> Current soil reading: ");
    Serial.print(soilRead);
    if (soilRead == 0) // ERROR
    {
      Serial.println(" - ERROR!");
      return -1;
    }
    result = map(soilRead, soil_min, soil_max, 100, 0);
    Serial.print(" - percent calculated: ");
    Serial.println(result);
    if (calibrate_soil)
      delay(2000);
  }

  if (result > 100)
    return 100;
  if (result < 0)
    return 0;
  return result;
}


// READ Battery
float readBattery()
{
  Serial.println("Reading battery:");
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);
  Serial.print("  Volt direct: ");
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

  // if (bat < 0)
  //   return 0;
  return bat;
}
