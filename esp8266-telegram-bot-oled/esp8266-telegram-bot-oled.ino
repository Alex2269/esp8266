/*******************************************************************
* An example of bot that receives commands and turns on and off *
* https://github.com/Alex2269/esp8266
*******************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <strings.h>

#include <Wire.h>
#include <ACROBOTIC_SSD1306.h>
#define SDA D1 // define i2c pins (Wire.h)
#define SCL D2 // define i2c pins (Wire.h)

#include <NTPClient.h>
#include <WiFiUdp.h>

#include "DHT.h"
#include <HCSR04.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 10800, 60000);

// UltraSonic connect:
#define trigger_pin D6 // 12 esp8266_pin d6
#define echo_pin    D5 // 14 esp8266_pin d5

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

  timeClient.update();
  String display_msg_distance = "distance: " + (String)distance + "\n";
  String display_msg_chat_id = "chat_id: " + chat_id + "\n";
  String display_msg_command = "command: " + text + "\n";
  String display_msg_time = "time:" + timeClient.getFormattedTime() + "\n";

  //oled.clearDisplay();
  oled.setTextXY(0,10);
  for(uint8_t i=10;i<16;i++)
  {
    oled.putString(" "); // clear space for value distance
  }
  oled.setTextXY(0,0);
  oled.putString(display_msg_distance);
  oled.setTextXY(1,0);
  oled.putString(display_msg_chat_id);
  oled.setTextXY(2,0);
  oled.putString(display_msg_command);  
  oled.setTextXY(3,0);
  oled.putString(display_msg_time);

  oled.setTextXY(5,0);
  oled.putString("IP address:");
  oled.setTextXY(6,0);
  char buf[16];
  IPAddress ip = WiFi.localIP();
  sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  //sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  oled.putString(buf);


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
  // oled.putStringln("handleNewMessages");
  // oled.putStringln(String(numNewMessages));
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

char* snd_str(String msg_ip)
{
  char ip[16]={0};
  uint8_t i = 0;
  while(msg_ip)
  {
    ip[i] = msg_ip[i];
    i++;
  }
  return ip;
}

void setupWifi()
{
  // attempt to connect to Wifi network:
  oled.clearDisplay();
  oled.setTextXY(0,0);
  oled.putString("Connecting Wifi");
  oled.putString(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    oled.setTextXY(1,0);
    oled.putString("wait connection");
    delay(1500);
  }

  oled.setTextXY(2,0);
  oled.putString("WiFi connected");

  oled.setTextXY(3,0);
  oled.putString("WIFI_SSID:");

  oled.setTextXY(4,0);
  oled.putString(WIFI_SSID);

  oled.setTextXY(5,0);
  oled.putString("IP address:");
  oled.setTextXY(6,0);
  char buf[16];
  sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  oled.putString(buf);

  delay(3000);
  oled.clearDisplay();
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
  // Initialize display
  Wire.begin(SDA,SCL); // set i2c pins (Wire.h)
  oled.init();                      // Initialze SSD1306 OLED display
  oled.clearDisplay();              // Clear screen

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
    // oled.putString("Checking messages - ");
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    // oled.putStringln(numNewMessages);
    while(numNewMessages)
    {
      // oled.putStringln("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeScan = millis();
  }
  // Передает управление к другим задачам, когда вызвано. должен использоваться в функциях, которым нужно некоторое время, чтобы завершиться.
  yield();
  delay(100);
}
