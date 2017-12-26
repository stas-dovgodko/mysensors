#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <Ticker.h>

#include <stdio.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


// Enable debug prints to serial monitor
#define MY_DEBUG

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600



#define MY_ESP8266_SSID "iot"
#define MY_ESP8266_PASSWORD ""

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2


#define MY_GATEWAY_MQTT_CLIENT
#define MY_GATEWAY_ESP8266
#define MY_REPEATER_FEATURE
#define MY_GATEWAY_CLIENT_MODE
#define MY_RAM_ROUTING_TABLE_FEATURE

// Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "in"

#define MY_TRANSPORT_DISCOVERY_INTERVAL_MS (10*60*1000ul)
#define MY_ROUTING_TABLE_SAVE_INTERVAL_MS (10*60*1000ul)

#define MY_SIGNING_REQUEST_SIGNATURES

#define TRIGGER_PIN 3
#define STATUS_LED BUILTIN_LED

#define MY_INCLUSION_MODE_FEATURE
//#define MY_INCLUSION_BUTTON_FEATURE
//#define MY_INCLUSION_MODE_BUTTON_PIN 3
#define MY_INCLUSION_MODE_DURATION 60

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Set MQTT client id
#define MY_MQTT_CLIENT_ID "mysensors-1"

#include <ESP8266WiFi.h>


#include "options.h"
#include "lib/MySensors.h"



#define MAX_SRV_CLIENTS 1

Ticker ticker;
WiFiManager wifiManager;


WiFiClient wclient;

PubSubClient MQTTClient = PubSubClient(wclient);//, mqtt_server, mqtt_port);

int tm = 300;
bool shouldSaveConfig = false;
bool firstConfig = true;
bool error = false;


void configModeCallback (WiFiManager *myWiFiManager) {

  MY_SERIALDEVICE.print("Entered config mode - ");
  MY_SERIALDEVICE.print(WiFi.softAPIP());
  MY_SERIALDEVICE.println(myWiFiManager->getConfigPortalSSID());

}

void saveConfigCallback () {

  shouldSaveConfig = true;
}

char ssid[40] = ""; // Имя вайфай точки доступа
char pass[255] = ""; // Пароль от точки доступа

char mqtt_server[255] = "m12.cloudmqtt.com"; // Имя сервера MQTT
char mqtt_port[6] = "14775"; // Порт для подключения к серверу MQTT
char mqtt_user[255] = ""; // Логи от сервер
char mqtt_pass[255] = ""; // Пароль от сервера
char mqtt_path[255] = "/";






void tick()
{
  //toggle state
  int state = digitalRead(STATUS_LED);  // get the current state of GPIO1 pin
  digitalWrite(STATUS_LED, !state);     // set pin to the opposite state
}

void sos()
{
  for (int x = 1; x <= 3; x++) {
    digitalWrite(STATUS_LED, LOW);
    delay(100);
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
  }
  delay(30);
  for (int x = 1; x <= 3; x++) {
    digitalWrite(STATUS_LED, LOW);
    delay(300);
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
  }
  delay(30);
  for (int x = 1; x <= 3; x++) {
    digitalWrite(STATUS_LED, LOW);
    delay(100);
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
  }
  digitalWrite(STATUS_LED, LOW);
}



byte bytes[] = {0B00010101, 0B00110011, 0B10100011, 0B00000010};
uint32_t ms, ms1 = 0;
uint8_t  blink_loop = 0;

void setup() {

  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  ticker.attach(0.3, tick);
  if (SPIFFS.begin()) {
    MY_SERIALDEVICE.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(mqtt_path, json["mqtt_path"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  }

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect(ssid, pass)) {
    doWifiConnect();
  }

  ticker.detach();
}

void presentation()
{

}

void set_error()
{


  if (!error) {
    //ticker.attach(2.0, sos);
    MY_SERIALDEVICE.println("error state");

    error = true;
  }
  //sos();
}

void set_normal()
{
  if (error) {
    Serial.println("normal state");
    ticker.detach();
    error = false;
  }
}

void soft_reset() {

  MY_SERIALDEVICE.println("Soft reset");
  ESP.restart();
}// - See more at: http://www.esp8266.com/viewtopic.php?f=8&t=7527#sthash.GNljsoT7.dpuf




//String incoming = ""; int opt = 0; String opts[5]; String cmd = ""; bool esc = false;
void loop() {

  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    MY_SERIALDEVICE.println("PIN");
    inclusionModeSet(true);
  }

  if (error) {
    // blink sos
    ms = millis();
    // Событие срабатывающее каждые 125 мс
    if ( ( ms - ms1 ) > 125 || ms < ms1 ) {
      ms1 = ms;
      // Выделяем сдвиг светодиода (3 бита)
      uint8_t n_shift = blink_loop & 0x07;
      // Выделяем номер байта в массиве (2 байта со здвигом 3 )
      uint8_t b_count = (blink_loop >> 3) & 0x3;
      if (  bytes[b_count] & 1 << n_shift ) digitalWrite(STATUS_LED, HIGH);
      else  digitalWrite(STATUS_LED, LOW);
      blink_loop++;
    }
  }

  uint8_t i;



  // подключаемся к wi-fi
  //if (ssid.length() > 0 && WiFi.status() != WL_CONNECTED) {
  //  doWifiConnect();
  //}
  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {

    if (mqtt_server[0] != 0) {
      if (MQTTClient.connected()) {
        set_normal();
        MQTTClient.loop();

        if (tm == 0)
        {
          SystemSend();

          tm = 300; // пауза меду отправками значений температуры коло 3 секунд
        }
        tm--;

        delay(10);
      } else {

        if (tm == 0)
        {
          if (!doMqttConnect()) {
            set_error();
          }

          tm = 300;
        }
        tm--;

        delay(10);
      }
    } else {
      set_error();
    }



  } else {
    set_error();
  }
}



void SystemSend()
{
  digitalWrite(STATUS_LED, HIGH);

  char bufk[50], bufv[50];


  sprintf(bufk, "%sip", mqtt_path);
  sprintf(bufv, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] );
  MQTTClient.publish(bufk, bufv);


  sprintf(bufk, "%ssignal", mqtt_path);
  sprintf(bufv, "%d", WiFi.RSSI());
  MQTTClient.publish(bufk, bufv);
}

bool doMqttConnect() {

  return true;

  
}

void doWifiConnect()
{
  ticker.attach(0.6, tick);
  

  MY_SERIALDEVICE.print("Connecting to ");
  MY_SERIALDEVICE.print(ssid);
  MY_SERIALDEVICE.println("...");


  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 255);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 255);
  WiFiManagerParameter custom_mqtt_password("pass", "mqtt password", mqtt_pass, 255);
  WiFiManagerParameter custom_mqtt_path("path", "mqtt path", mqtt_path, 255);

  if (firstConfig) {
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_mqtt_user);
    wifiManager.addParameter(&custom_mqtt_password);
    wifiManager.addParameter(&custom_mqtt_path);
  }

  if (!wifiManager.startConfigPortal("mysensors_gw")) {
    MY_SERIALDEVICE.println("failed to connect and hit timeout");
  } else {
    //read updated parameters


    //save the custom parameters to FS
    if (shouldSaveConfig) {
      if (firstConfig) {
        strcpy(mqtt_server, custom_mqtt_server.getValue());
        strcpy(mqtt_port, custom_mqtt_port.getValue());
        strcpy(mqtt_user, custom_mqtt_user.getValue());
        strcpy(mqtt_pass, custom_mqtt_password.getValue());
        strcpy(mqtt_path, custom_mqtt_path.getValue());
      }
      MY_SERIALDEVICE.println("saving config");
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["mqtt_server"] = mqtt_server;
      json["mqtt_port"] = mqtt_port;
      json["mqtt_user"] = mqtt_user;
      json["mqtt_pass"] = mqtt_pass;
      json["mqtt_path"] = mqtt_path;

      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        MY_SERIALDEVICE.println("failed to open config file for writing");
      }

      json.printTo(MY_SERIALDEVICE);
      json.printTo(configFile);
      configFile.close();


      soft_reset();
    }
    firstConfig = false;
  }
  ticker.detach();
  MY_SERIALDEVICE.println("connected - " + WiFi.localIP().toString());
}

void doWifiScan() {
  // scan for nearby networks:
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1) {
    MY_SERIALDEVICE.println("Couldn't get a wifi connection");

  } else {

    // print the list of networks seen:
    MY_SERIALDEVICE.print("number of available networks:");
    MY_SERIALDEVICE.println(numSsid);

    // print the network number and name for each network found:
    for (int thisNet = 0; thisNet < numSsid; thisNet++) {
      MY_SERIALDEVICE.print(thisNet);
      MY_SERIALDEVICE.print(") ");
      MY_SERIALDEVICE.print(WiFi.SSID(thisNet));
      MY_SERIALDEVICE.print("\tSignal: "); MY_SERIALDEVICE.print(WiFi.RSSI(thisNet)); MY_SERIALDEVICE.print(" dBm");
      MY_SERIALDEVICE.print("\tEncryption: ");
      switch (WiFi.encryptionType(thisNet)) {
        case ENC_TYPE_WEP:
          MY_SERIALDEVICE.println("WEP");
          break;
        case ENC_TYPE_TKIP:
          MY_SERIALDEVICE.println("WPA");
          break;
        case ENC_TYPE_CCMP:
          MY_SERIALDEVICE.println("WPA2");
          break;
        case ENC_TYPE_NONE:
          MY_SERIALDEVICE.println("None");
          break;
        case ENC_TYPE_AUTO:
          MY_SERIALDEVICE.println("Auto");
          break;
      }
    }
  }
}




