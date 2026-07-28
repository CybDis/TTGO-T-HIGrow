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

// Trimmed mean over many calibrated millivolt samples: same ESP32 ADC noise as
// readAnalogTrimmedMean(), but on analogReadMilliVolts()'s already-calibrated
// output (which does its own read internally, so it can't reuse the raw-count helper above).
uint32_t readAnalogMilliVoltsTrimmedMean(uint8_t pin)
{
  const uint8_t samples = 120;
  uint32_t array[120];
  uint32_t sum = 0;

  for (int i = 0; i < samples; i++)
  {
    array[i] = analogReadMilliVolts(pin);
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
//
// Battery ADC calibration: RawBattAdc -> voltage via analogReadMilliVolts(),
// which uses the ESP32's factory eFuse Vref/Two-Point calibration internally.
// Found to be accurate in practice (see doc/2026-07-25-battery-voltage-calibration-issue.md);
// an earlier attempt at an additional per-device empirical linear fit was
// removed again since it turned out to be unnecessary.
//
// Both RawBattAdc and the calibrated voltage are read via the trimmed-mean
// helpers (same as Soil/Salt) instead of a single sample: the Battery% mapping
// below only spans 0.9V end-to-end (~1.1%/mV), so the ESP32 ADC's usual
// +-20..50 count noise otherwise shows up as multi-percent flicker in Home
// Assistant (see doc/2026-07-28-battery-percentage-flicker-analysis.md).
//
// Battery% endpoints: 420 = 4.2V, standard 1S LiPo full-charge voltage;
// 330 = 3.3V, the lowest bench voltage at which HiGrow12 still boots
// (3.2V fails to boot - LDO dropout), so 0% lines up with "about to die"
// rather than an arbitrary lower voltage the board would never report from
// anyway.
//
// The upper bound is intentionally NOT clamped here: while charging, the
// measured voltage exceeds the 4.2V full-charge point, pushing the mapped
// value above 100. main.cpp uses that ">100" overshoot (checked against a
// margin, see the "charging" threshold there) to detect charging before
// clamping config.bat down to 100 for display. Clamping it here would
// silently break that detection.
float readBattery()
{
  Serial.println("Reading battery:");

  uint16_t volt = readAnalogTrimmedMean(BAT_ADC);
  Serial.print("  Volt direct (trimmed mean): ");
  Serial.println(volt);
  config.batvolt = volt;

  float battery_voltage = ((float)readAnalogMilliVoltsTrimmedMean(BAT_ADC) / 1000.0) * 2.0; // 2.0 = board's BAT_ADC divider ratio
  config.batvoltage = battery_voltage;
  Serial.print("  Battery Voltage: ");
  Serial.println(battery_voltage);

  battery_voltage = battery_voltage * 100;
  float bat = map((long)roundf(battery_voltage), 420, 330, 100, 0); // round, not truncate, to the nearest 0.01V before map()'s integer math
  Serial.print("  Battery level: ");
  Serial.println(bat);

  if (bat < 0)
    return 0;
  return bat;
}
