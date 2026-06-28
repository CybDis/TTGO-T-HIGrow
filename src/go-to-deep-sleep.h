void goToDeepSleep(uint32_t sleepSeconds = TIME_TO_SLEEP)
{
  Serial.print("Going to sleep: ");
  Serial.print(sleepSeconds);
  Serial.print(" seconds = ");
  Serial.print(sleepSeconds / 60);
  Serial.println(" minutes.");

  if (logging) {
    writeFile(SPIFFS, "/error.log", "Going to sleep for configured time \n");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  Serial.println("Zzzzz...");
  Serial.flush(true);

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup((uint64_t)sleepSeconds * uS_TO_S_FACTOR);
  
  // HiGrow3 ist defekt, so no external wakeup - we do not use it anyways
  //esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);

  // Testpurposes
  //esp_sleep_enable_timer_wakeup(10 * uS_TO_S_FACTOR);

  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Going to deep sleep \n \n \n");
  }

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}