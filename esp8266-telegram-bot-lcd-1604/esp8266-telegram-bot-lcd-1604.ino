/*******************************************************************
* An example of bot that receives commands and turns on and off *
* https://github.com/Alex2269/esp8266
*******************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <LiquidCrystal_I2C.h>

#include "DHT.h"
#include <HCSR04.h>

// A library for I2C LCD displays.
// The library allows to control I2C displays with functions extremely similar to LiquidCrystal library
// https://github.com/marcoschwartz/LiquidCrystal_I2C
// display connect: SCL-esp8266_pin d1(GPIO5) SDA-esp8266_pin d2(GPIO4)
#define i2c_addres 0x3f
LiquidCrystal_I2C lcd(i2c_addres,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// UltraSonic connect:
#define trigger_pin 12 // esp8266_pin d6
#define echo_pin    14 // esp8266_pin d5

UltraSonicDistanceSensor distanceSensor(trigger_pin, echo_pin); // Инициализируйте датчик, который использует цифровые выводы 12 и 14.

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

void sender_message(void)
{
  String chat_id = String(bot.messages[0].chat_id);
  String text = bot.messages[0].text;
  // String from_name = bot.messages[0].from_name;

  uint16_t distance_min = 25; // минимальная дистанция для отправки сообщения
  uint16_t distance = (uint16_t)distanceSensor.measureDistanceCm();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("distance:");
  lcd.print(distance);
  lcd.setCursor(0,1);
  lcd.print(text);

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
      bot.sendMessage(chat_id, (String)distanceSensor.measureDistanceCm(), "");
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
  delay(500);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("wait connection");
    delay(500);
  }
  delay(500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi connected");
  lcd.setCursor(0,1);
  lcd.print("IP address:");
  delay(500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(WiFi.localIP());
  lcd.setCursor(0,1);
  lcd.print("distance:");
  lcd.print(distanceSensor.measureDistanceCm());
  delay(500);
}

void setupPins()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  delay(10);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
}

void setup()
{
  lcd.init();
  lcd.backlight();

  setupWifi();
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
  yield();
  delay(10);
}

