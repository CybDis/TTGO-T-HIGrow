void connectToNetwork()
{
  Serial.print("Size of SSID array ");
  Serial.println(ssidArrNo);
  const char *Hostname = plant_name.c_str();
  WiFi.mode(WIFI_AP); // Workaround: Setting Hostname works only in AP mode
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // call is only a workaround for bug in WiFi class
  WiFi.setHostname(Hostname);

  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setAutoConnect(true);
  
  bool breakLoop = false;
  if (logging)
  {
    writeFile(SPIFFS, "/error.log", "Connecting to Network: \n");
  }
  for (int i = 0; i < ssidArrNo; i++)
  {
    ssid = ssidArr[i].c_str();
    Serial.print("SSID name: ");
    Serial.println(ssidArr[i]);
    Serial.print("Connecting");

    while (WiFi.status() != WL_CONNECTED)
    {
      // wifi down, reconnect here
      WiFi.begin(ssid, password);
      int WLcount = 0;
      int UpCount = 0;
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        Serial.printf(".");
        if (UpCount >= 60) // just keep terminal from scrolling sideways
        {
          UpCount = 0;
          Serial.printf("\n");
          Serial.printf("Disconnecting & Reconnecting");
          WiFi.disconnect(true, true);
          delay(1000);
          WiFi.begin(ssid, password);
        }
        ++UpCount;
        ++WLcount;
        if (WLcount > 200)
        {
          Serial.println("we should break");
          breakLoop = true;
          break;
        }
      }
      if (breakLoop)
      {
        breakLoop = false;
        break;
      }
    }
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    goToDeepSleepFiveMinutes();
  }
}