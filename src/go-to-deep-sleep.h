void goToDeepSleep()
{
  Serial.print("Going to sleep: ");
  Serial.print(TIME_TO_SLEEP);
  Serial.print(" seconds = ");
  Serial.print(TIME_TO_SLEEP / 60);
  Serial.println(" minutes.");
  
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Going to sleep for configured time \n");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);

  // Testpurposes
  //esp_sleep_enable_timer_wakeup(10 * uS_TO_S_FACTOR);

  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Going to deep sleep \n \n \n");
  }

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}