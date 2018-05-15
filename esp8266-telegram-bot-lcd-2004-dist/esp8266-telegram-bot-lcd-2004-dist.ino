/*******************************************************************
* An example of bot that receives commands and turns on and off *
* https://github.com/Alex2269/esp8266
*******************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "DHT.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 10800, 60000);

// A library for I2C LCD displays.
// The library allows to control I2C displays with functions extremely similar to LiquidCrystal library
// https://github.com/marcoschwartz/LiquidCrystal_I2C
// display connect: SCL-esp8266_pin d1(GPIO5) SDA-esp8266_pin d2(GPIO4)
//#include <Wire.h>
//#define SDA D1
//#define SCL D2
#define i2c_addres 0x3f
LiquidCrystal_I2C lcd(i2c_addres,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// UltraSonic connect D6 D5:
uint8_t triggerPin = D6;
uint8_t echoPin = D5;

// Initialize Wifi connection to the router
// bot name:       esp8266x002
// bot user_name:  @esp8266x002_bot
// https://web.telegram.org/#/im?p=@esp8266x002_bot

#define WIFI_SSID "you-ssid"
#define WIFI_PASSWORD "you-password"
#define BOTtoken "you-token:xxxxxxxxxxxxxxxxxxxxxxxxxxxxx" // give key: https://core.telegram.org/bots#6-botfather

#define LED_PIN 2
#define RELAY_PIN D8
#define DHT_PIN D7
#define DHTTYPE DHT11
#define BOT_SCAN_MESSAGE_INTERVAL 1000 // Интервал, для получения новых сообщений

long lastTimeScan; // время, после последнего сообщения
bool ledStatus = false; // статус светодиода
bool relayStatus = false; // статус реле
WiFiClientSecure client;

UniversalTelegramBot bot(BOTtoken, client);
DHT dht(DHT_PIN, DHTTYPE);

void init_HCSR04(uint8_t triggerPin, uint8_t echoPin)
{
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

double measure_HCSR04(void)
{
  // Make sure that trigger pin is LOW.
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  // Hold trigger for 10 microseconds, which is signal for sensor to measure distance.
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  // Measure the length of echo signal, which is equal to the time needed for sound to go there and back.
  unsigned long durationMicroSec = pulseIn(echoPin, HIGH);
  double distanceCm = durationMicroSec / 2.0 * 0.0343;
  if (distanceCm == 0 || distanceCm > 400) 
  {
    return -1.0 ;
  }
  else 
  {
    return distanceCm;
  }
}

void sender_message(void)
{
  String chat_id = String(bot.messages[0].chat_id);
  String text = bot.messages[0].text;
  // String from_name = bot.messages[0].from_name;

  uint16_t distance_min = 25; // минимальная дистанция для отправки сообщения
  uint16_t distance = (uint16_t)measure_HCSR04();

  timeClient.update();
  String lcd_msg_distance = " distance: " + (String)distance + " cm";
  String lcd_msg_chat_id = " chat_id: " + chat_id;
  String lcd_msg_command = " command: " + text;
  String lcd_msg_time = " time: " + timeClient.getFormattedTime();

  lcd.clear();
  lcd.setCursor(0,0); lcd.print(lcd_msg_distance);
  lcd.setCursor(0,1); lcd.print(lcd_msg_chat_id);  
  lcd.setCursor(0,2); lcd.print(lcd_msg_command);
  lcd.setCursor(0,3); lcd.print(lcd_msg_time);

  if(text == "/mute") return;
  if(distance > distance_min) return;

  bot.messages[0].text = "/mute"; // блокируем повторные отправки
  String msg_distance = " distance: " + (String)distance + " centimeter " + " chat_id: " + chat_id + "\n";
  bot.sendMessage(chat_id, msg_distance, ""); // give chat_id - идентификатор чата которому пойдет сообщение
  //bot.sendMessage("you-chat_id_l", msg_distance, ""); // отправка в канал 1.
  //bot.sendMessage("you-chat_id_2", msg_distance, ""); // отправка в канал 2.
}

// Это рассматривает новые сообщения, которые прибыли
void handleNewMessages(int numNewMessages)
{
  // lcd.println("handleNewMessages");
  // lcd.println(String(numNewMessages));
  for (int i=0; i<numNewMessages; i++)
  {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if(from_name == "") from_name = "вводите команду";
    // идентификация полученных команд

    if(text == "/distance")
    {
      bot.sendMessage(chat_id, (String)measure_HCSR04(), "");
    }

    if(text == "/ledon")
    {
      digitalWrite(LED_PIN, HIGH); // turn the LED on (HIGH is the voltage level)
      ledStatus = true;
      bot.sendMessage(chat_id, "светодиод включен", "");
    }
    if(text == "/ledoff")
    {
      ledStatus = false;
      digitalWrite(LED_PIN, LOW); // turn the LED off (LOW is the voltage level)
      bot.sendMessage(chat_id, "светодиод выключен", "");
    }

    if(text == "/relayon")
    {
      digitalWrite(RELAY_PIN, HIGH);
      relayStatus = true;
      bot.sendMessage(chat_id, "реле включено", "");
    }
    if(text == "/relayoff")
    {
      relayStatus = false;
      digitalWrite(RELAY_PIN, LOW);
      bot.sendMessage(chat_id, "реле выключено", "");
    }

    if(text == "/status")
    {
      String message = "светодиод ";
      if(ledStatus)
      {
        message += "включен";
      }
      else
      {
        message += "отключен";
      }
      message += ". \n";
      message += "реле ";
      if(relayStatus)
      {
        message += "включено";
      }
      else
      {
        message += "отключено";
      }
      message += ". \n";
      bot.sendMessage(chat_id, message, "Markdown");
    }

    if( text == "/env")
    {
      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();
      String message = "Температура имеет " + String(temperature, 2) + " градус цельсия.\n";
      message += "Относительная влажность воздуха имеет " + String(humidity, 2)+ "%.\n\n";
      bot.sendMessage(chat_id, message, "Markdown");
    }
    // Это создает клавиатуру с вариантами команды
    if(text == "/options")
    {
      String keyboardJson = "[[\"/distance\", \"/mute\"],[\"/ledon\", \"/ledoff\"],[\"/relayon\", \"/relayoff\"],[\"/env\",\"/status\"],[\"/options\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Выберите один из вариантов", "", keyboardJson, true);
    }

    // Группа команд обращений к устройству
    if(text == "/start")
    {
      String welcome = from_name + ", посылаем сообщение боту от esp .\n";
      welcome += "чтобы взаимодействовать с домом, используйте одну из следующих команд.\n\n";
      welcome += "/distance : показать расстояние до объекта \n";
      welcome += "/mute : запрещает автоматические сообщения, любая последующая команда отменяет ее. \n";
      welcome += "/ledon : включаем светодиод \n";
      welcome += "/ledoff : выключаем светодиод \n";
      welcome += "/relayon : включаем реле \n";
      welcome += "/relayoff : выключаем реле \n";
      welcome += "/env : показать температуру и влажность окружающей среды \n";
      welcome += "/status : показать статус датчиков и прочее \n";
      welcome += "/options : управление с помощью виртуальной клавиатуры команд \n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void setupWifi()
{
  // attempt to connect to Wifi network:
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting Wifi");
  lcd.setCursor(0,1);
  lcd.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    lcd.setCursor(0,2);
    lcd.print("wait connection");
    delay(1500);
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi connected");
  lcd.setCursor(0,1);
  lcd.print("IP address:");
  lcd.setCursor(0,2);
  lcd.print(WiFi.localIP());
  //lcd.setCursor(0,3);
  //lcd.print("distance:");
  //lcd.print((uint16_t)measure_HCSR04());
  delay(1500);
}

void setupPins()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  delay(10);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
}

void setup()
{
  // Initialize display:
  //Wire.begin(SDA,SCL); // set i2c pins (Wire.h)

  init_HCSR04(triggerPin, echoPin);

  lcd.init();
  lcd.backlight();

  setupWifi();
  dht.begin();
  setupPins();
  lastTimeScan = millis();
}

void loop()
{
  if(millis() > lastTimeScan + BOT_SCAN_MESSAGE_INTERVAL)
  {
    sender_message();
    // lcd.print("Checking messages - ");
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    // lcd.println(numNewMessages);
    while(numNewMessages)
    {
      // lcd.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeScan = millis();
  }
  // Передает управление к другим задачам, когда вызвано. должен использоваться в функциях, которым нужно некоторое время, чтобы завершиться.
  yield();
  delay(10);
}

