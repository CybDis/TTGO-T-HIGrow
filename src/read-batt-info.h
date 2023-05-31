#include <Arduino.h>
// Read battery charging info
void read_batt_info()
{
    readFile(SPIFFS, "/batinfo.conf");
    Serial.print("Here comes the charge date info info: ");
    Serial.println(readString);

    battChargeEpoc = getValue(readString, ':', 0);
    config.batchargeDate = getValue(readString, ':', 1);

    readString = "";
}