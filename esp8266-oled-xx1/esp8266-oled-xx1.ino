/*******************************************************************
* An example of bot that receives commands and turns on and off *
* https://github.com/Alex2269/esp8266
*******************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <strings.h>

#include <Wire.h>
#include "OLED.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

#include "DHT.h"
#include <HCSR04.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 10800, 60000);

#define SDA D1
#define SCL D2
OLED display(SDA,SCL);

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
  char buf[32];
  uint8_t i = 0;

  String chat_id = String(bot.messages[0].chat_id);
  String text = bot.messages[0].text;
  // String from_name = bot.messages[0].from_name;

  uint16_t distance_min = 25; // минимальная дистанция для отправки сообщения
  uint16_t distance = (uint16_t)distanceSensor.measureDistanceCm();

  timeClient.update();
  String display_msg_chat_id = "chatID:" + chat_id;
  String display_msg_command = "cmd: " + text;
  String display_msg_time = "time:" + timeClient.getFormattedTime() + "\n";

  display.clear();
  sprintf(buf, "distance: %d", distance); display.print(buf,0,0);

  i=0;
  while(display_msg_chat_id){
    buf[i]=display_msg_chat_id[i];
    i++;
  } display.print(buf,3,0);

  //display.print(display_msg_command,1,0);

  //display.print(display_msg_distance,0,0);
  //display.print(display_msg_chat_id,1,0);
  //display.print(display_msg_command,2,0);  
  //display.print(display_msg_time,3,0);

  display.print("IP address:",5,0);
  IPAddress ip = WiFi.localIP();
  sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  //sprintf(buf, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  display.print(buf,6,0);

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
  // display.println("handleNewMessages");
  // display.println(String(numNewMessages));
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
  display.clear();
  display.print("Connecting Wifi",0,0);
  display.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    display.print("wait connection",1,0);
    delay(1500);
  }
  display.print("WiFi connected",2,0);
  display.print("WIFI_SSID:",3,0);
  display.print(WIFI_SSID,4,0);
  display.print("IP address",5,0);

  char buf[16];
  IPAddress ip = WiFi.localIP();
  sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  display.print(buf,6,0);

  delay(2000);
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
  display.begin();

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
    // display.print("Checking messages - ");
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    // display.println(numNewMessages);
    while(numNewMessages)
    {
      // display.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeScan = millis();
  }
  // Передает управление к другим задачам, когда вызвано. должен использоваться в функциях, которым нужно некоторое время, чтобы завершиться.
  yield();
  delay(100);
}
