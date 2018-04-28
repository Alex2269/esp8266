#include <ESP8266WiFi.h>

#include <HCSR04.h>
#define trigger_pin 12
#define echo_pin 14
UltraSonicDistanceSensor distanceSensor(trigger_pin, echo_pin);

#define ENABLE_DHT11 1
#define ENABLE_DS18B20 1
#define ENABLE_INTERACTIVE 1
#define ENABLE_USERPERMITTED 0
uint32_t users=2;

String nick_users[] =
{
  "PDAUSER|49192", "PDAUSER|xxxx"
};

#if ENABLE_DHT11==1
  #include "DHT.h"
  #define DHTPIN 14
  #define DHTTYPE DHT11
  DHT dht(DHTPIN, DHTTYPE);
#endif

#if ENABLE_DS18B20==1
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define ONE_WIRE_BUS 2
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
#endif

bool first_bit=0;
const char* ssid = "you-ssid";
const char* password = "you-password";

const char* host = "irc.freenode.net";
uint32_t portirc = 6667;
String nickname = "esp8266_bot";
String channel = "#channelxxxx";

String Msg;
String Nick;
String Mac;
String NickComplete;

uint32_t counterInteractions=0;
uint32_t reeboot_esp=0;
uint32_t reeboot_hr = 8;

#define IRC_BUFSIZE 32

char from[IRC_BUFSIZE];
char type[IRC_BUFSIZE];
char to[IRC_BUFSIZE];
WiFiClient client;

void setup()
{
  pinMode(12, OUTPUT);
  Serial.begin(115200);

  #if ENABLE_DHT11==1
    dht.begin();
  #endif

  #if ENABLE_DS18B20==1
    sensors.begin();
  #endif

  delay(10);
  reeboot_esp=0;
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Mac = String(WiFi.macAddress());
  Mac.remove(0, 9);
  Mac.replace(":", "");
  NickComplete = nickname + Mac;
  Serial.println("nickname esp8266 = "+NickComplete);
}

void loop()
{
  if (!client.connected())
  {
    Serial.println("connecting ...");
    if (client.connect(host, portirc))
    {
      Serial.println("connected");
      delay(500);
      client.println("USER uk 8 * : Uwe user\r");
      delay(500);
      client.println("NICK "+nickname+Mac+"\r");
      delay(250);
      Serial.println("Ingresando a Canal ...");
      delay(250);
      Serial.println(channel);
      client.println("JOIN "+channel+"\r");
      handle_irc_connection();
    }
    else
    {
      Serial.println("connection failed...........");
      delay(1000);
    }
  }
}

void handle_irc_connection()
{
  char c;
  while(true)
  {
    if (!client.connected())
    {
      return;
    }

    if((uint16_t)distanceSensor.measureDistanceCm()<40)
    {
      //IRCcommand(Msg,"","distance Now = "+(String)distanceSensor.measureDistanceCm()+"Cm");
      IRCsendMsg(channel, "distance Now = "+(String)distanceSensor.measureDistanceCm()+"Cm");
      delay(1500);
    }

    if(client.available())
    {
      c = client.read();
    }
    else
    {
      continue;
    }

    if(c == ':')
    {
      memset(from, 0, sizeof(from));
      memset(type, 0, sizeof(type));
      memset(to, 0, sizeof(to));
      read_until(' ', from);
      read_until(' ', type);
      read_until(' ', to);
      if(strcmp(type, "PRIVMSG") == 0)
      {
        Nick = print_nick(from);
        ignore_until(':');
        Msg = print_until('\r');
        if(first_bit==0)
        {
          IRCsendMsg(channel,"Hello everyone :), I am bot "+ NickComplete);
          first_bit=1;
        }
        Serial.println("<"+Nick+"> :"+String(Msg));
        if(userPermitted ()==1)
        {
          IRCcommand(Msg,"HIGH GPIO","Turn ON");
          IRCcommand(Msg,"LOW GPIO","Turn OFF");
          IRCcommand(Msg,"READ ADC0","ADC0 esp8266 = "+String(A0));
          IRCcommand(Msg,"ESP SIGNAL?","Signal RSSI ESP8266 = "+String(WiFi.RSSI( ))+"dB");
          IRCcommand(Msg,"ESP dist?","distance Now = "+(String)distanceSensor.measureDistanceCm()+"Cm");

          #if ENABLE_DHT11==1
            float humedad = dht.readHumidity();
            float temperatura = dht.readTemperature();
            IRCcommand(Msg,"ESP Temp?","DHT11 Temperature Now = "+String(temperatura)+"°C");
            IRCcommand(Msg,"ESP HR?","DHT11 Humidity Relative Now = "+String(humedad)+"%" );
          #endif

          #if ENABLE_DS18B20==1
            sensors.requestTemperatures();
            float celsius = sensors.getTempCByIndex(0);
            IRCcommand(Msg,"ESP Temp2?","DS18B20 Onewire Temperature Now = "+String(celsius)+"°C");
          #endif

          if(IRCcommand(Msg,NickComplete+" help","*------ Comandos Disponibles / Available Commands -------*" ))
          {
            IRCsendMsg(channel," HIGH GPIO#");
            IRCsendMsg(channel," LOW GPIO#");
            IRCsendMsg(channel," READ ADC0");
            IRCsendMsg(channel," ESP dist?");
            IRCsendMsg(channel," ESP Temp?");
            IRCsendMsg(channel," ESP Temp2?");
            IRCsendMsg(channel," ESP HR?");
            IRCsendMsg(channel," ESP SIGNAL?");
            IRCsendMsg(channel,"*--------------------------------------------------------*");
          }
        }
        IRCcommand(Msg,":(","Why are you sad? "+Nick );
        IRCcommand(Msg,":(","Por que estas triste? "+Nick );
        IRCcommand(Msg,"ESP?????","Hi, I'm here. I'm " + NickComplete +" :P space on the chip is "+String(ESP.getFreeHeap())+" and how are you?? "+Nick);
      }
      else
      {
        ignore_until('\r');
      }
    }
    else if (c == 'P')
    {
      char buf[5];
      memset(buf, 0, sizeof(buf));
      buf[0] = c;
      for(int i = 1; i < 4; i++)
      {
        c = client.read();
        buf[i] = c;
      }
      ignore_until('\r');
      if(strcmp(buf, "PING") == 0)
      {
        client.println("PONG\r");

        #if ENABLE_INTERACTIVE==1
          counterInteractions = counterInteractions +1;
        #endif

        if(counterInteractions>= 15)
        {
          Serial.println("interaction");
          IRCsendMsg(channel,"More info Project PDAControl: pdacontrolen.com/irc ;)");
          delay(100);
          IRCsendMsg(channel,"Mas info Proyecto PDAControl: pdacontroles.com/irc ;)");
          counterInteractions=0;
        }
        reeboot_esp= reeboot_esp +1;
        if (reeboot_esp>= (30 * reeboot_hr ))
        {
          IRCsendMsg(channel,"Cleaning .. I'll be back in a moment ... :) ");
          ESP.restart();
          reeboot_esp=0;
        }
      }
    }
  }
}
