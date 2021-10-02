#include "config.h"             // mqtt credentials

#include <ESP8266WiFi.h>        // Wifi library
#include <PubSubClient.h>       // MQTT library

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic 
#include <Ticker.h>

Ticker ticker;
WiFiClient wifiClient;               // WiFi
PubSubClient client(wifiClient);     // MQTT

bool isConnected = false;

// PINS
#define PIN_RED  3   // RX
#define PIN_GREEN  1 // TX

// PARAMETERS
int value = 0;
int newValue = 0;
int animationVal = 0;

float R;
const int pwmIntervals = 255;
const int MIN_BLINK = 150;
bool incr = true;
bool gain = true;

void setup() {
    WiFiManager wifiManager;
    WiFiManagerParameter custom_text("<p>(c) 2021 by <a href=\"mailto:hoi@joszuijderwijk.nl\">Jos Zuijderwijk</a></p>");
    wifiManager.addParameter(&custom_text);

    if (wifiManager.autoConnect("StockWatcher", "")){
      isConnected = true;
    }
  
  // MQTT
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);

  // Turn lights off
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, HIGH);

  R = (pwmIntervals * log10(2))/(log10(255));
}

// Try restore the MQTT connection
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect(MQTT_CLIENT_NAME, MQTT_USER, MQTT_PASS, "stockwatcher/connection", 0, 1, "0")) {
      // Send Hello World!
      client.publish("stockwatcher/connection", "1", 1);
      client.subscribe("stockwatcher/input/#");
    }
  }
}


// Handle incoming messages
void callback(char* topic, byte* payload, unsigned int len) {
    
    String msg = ""; // payload
    for (int i = 0; i < len; i++) {
      msg += ((char)payload[i]);
    }

  if ( strcmp(topic, "stockwatcher/input") == 0 ){

    int x = 0;
    if (msg.startsWith("-")){
      msg.remove(0,1);
      x = msg.toInt();
      x *= -1;
    } else{
      x = msg.toInt();
    }

    newValue = max(min(x, 255),-255);
    ticker.detach();

    if (x > 255 || x < -255){
      // big gain or loss
      gain = x > 255;
      incr = !gain;
      value = gain ? 255 : -255;

      analogWrite(gain ? PIN_RED : PIN_GREEN, 255);
      ticker.attach_ms(20, blinkAnimation);
    } else{
      //reguar fade
      ticker.attach_ms(20, fadeAnimation);
    }

  }
}

void loop() {

    if (!client.connected() && isConnected){
     reconnect();
    }

    client.loop();
}

// Fade to new value
void fadeAnimation(){

  value += (newValue > value) ? 1 : -1;

  int brightness = 255 - (pow (2, (abs(value) / R)) - 1);
  analogWrite(value >= 0 ? PIN_GREEN : PIN_RED, brightness);

  if (value == 0)
    analogWrite(PIN_RED, 255);

  if (value == newValue)
    ticker.detach();
}


// Big gain / loss
void blinkAnimation(){
  
  value += incr ? 1 : -1;
  int x = gain ? 1 : -1;

  if (value == MIN_BLINK * x || value == 255 * x)
    incr = !incr;
  
  int brightness = 255 - (pow (2, (abs(value) / R)) - 1);
  analogWrite(gain ? PIN_GREEN : PIN_RED, brightness);
}
