#include <Arduino.h>
// Read battery charging info
void read_batt_info()
{
    Serial.println("Reading /batinfo.conf");
    readFile(SPIFFS, "/batinfo.conf");
    Serial.print("   Charge date info: ");
    Serial.println(readString);

    battChargeEpoc = getValue(readString, ':', 0);
    config.batchargeDate = getValue(readString, ':', 1);

    readString = "";
}