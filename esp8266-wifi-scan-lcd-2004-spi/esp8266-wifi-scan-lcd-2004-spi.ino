/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 */
#include "ESP8266WiFi.h"
#include <LiquidCrystal_I2C.h>

//#define SDA D1
//#define SCL D2
#define i2c_addres 0x3f
LiquidCrystal_I2C lcd(i2c_addres,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
  lcd.init();
  lcd.backlight();
  //Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  lcd.println("Setup done");
}

void loop() {
  lcd.print("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  lcd.print("scan done");
  if (n == 0)
    lcd.print("no networks found");
  else
  {
    lcd.print(n);
    lcd.print(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      lcd.setCursor(0,0);
      lcd.print(i + 1);
      lcd.print(": ");
      lcd.print(WiFi.SSID(i));
      lcd.print(" (");
      lcd.print(WiFi.RSSI(i));
      lcd.print(")");
      lcd.print((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(1500);
      lcd.clear();
    }
  }
  lcd.print("\n");

  // Wait a bit before scanning again
  delay(5000);
}
