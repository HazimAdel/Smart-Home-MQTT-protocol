#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>



// for OLED screen
// You can use any (4 or) 5 pins
#define sclk 14
#define mosi 13
#define cs   15
#define rst  2
#define dc   16

// For bar segment
#define b1   0
#define b2   1
#define b3   3
#define b4   4
#define b5   5
#define b67  12
#define b8   17

// Color definitions
#define BLACK            0x0000
#define BLUE             0x001F
#define RED              0xF800
#define GREEN            0x07E0
#define CYAN             0x07FF
#define MAGENTA          0xF81F
#define YELLOW           0xFFE0
#define WHITE            0xFFFF

Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, cs, dc, rst);

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "set_yours"
#define WLAN_PASS       "set_yours"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "set_yours"
#define AIO_KEY         "set_yours"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feeds for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temp_led = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp_led");
Adafruit_MQTT_Publish door_led = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/door_led");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe temp = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Subscribe hum  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Subscribe door = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/door");
Adafruit_MQTT_Subscribe CO2  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/CO2");
Adafruit_MQTT_Subscribe NH4  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/NH4");
Adafruit_MQTT_Subscribe LDR  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/LDR");
Adafruit_MQTT_Subscribe PPM  = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/PPM");


/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);
  display.begin();
  pinMode(b1, OUTPUT);
  pinMode(b2, OUTPUT);
  pinMode(b3, OUTPUT);
  pinMode(b4, OUTPUT);
  pinMode(b5, OUTPUT);
  pinMode(b67, OUTPUT);
  pinMode(b8, OUTPUT);

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  // mqtt.subscribe(&temp);
  // mqtt.subscribe(&hum);
  // mqtt.subscribe(&door);
  mqtt.subscribe(&CO2);
  mqtt.subscribe(&NH4);
  mqtt.subscribe(&PPM);
  mqtt.subscribe(&LDR);
}

int d = 0;
void loop() {
  MQTT_connect();

  /*if(atoi((char *)door.lastread)==0){
    d = !d;
    door_led.publish(d);
  }*/

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10000))) {
    display.fillScreen(BLACK);
    display.setCursor(0, 0);
    display.setTextColor(YELLOW);
    display.setTextSize(1);
    display.print("Temp: "); display.print(20/*(char *)temp.lastread*/);display.println(" C");
    display.print("Hum: "); display.print(85/*(char *)hum.lastread*/); display.println(" %");
    display.print("CO2: "); display.print((char *)CO2.lastread); display.println(" PPM");
    display.print("NH4: "); display.print((char *)NH4.lastread); display.println(" PPM");
    display.print("LDR: ");

    if (atoi((char *)LDR.lastread) < 40) {
    display.println("Dark");
    } else if (atoi((char *)LDR.lastread) < 800) {
    display.println("Dim");
    } else if (atoi((char *)LDR.lastread) < 2000) {
    display.println("Light");
    } else {
    display.println("Bright");
    }

    if(d==0) {
      display.print("Door: "); display.println("Closed");
    } else {
      display.print("Door: "); display.println("Opened");
    }
  }
  
  if (atoi((char *)PPM.lastread) < 400) {
    digitalWrite(b1, HIGH);
    digitalWrite(b2, LOW);
    digitalWrite(b3, LOW);
    digitalWrite(b4, LOW);
    digitalWrite(b5, LOW);
    digitalWrite(b67, LOW);
    digitalWrite(b8, LOW);
  } else if (atoi((char *)PPM.lastread) < 750) {
    digitalWrite(b1, HIGH);
    digitalWrite(b2, HIGH);
    digitalWrite(b3, HIGH);
    digitalWrite(b4, LOW);
    digitalWrite(b5, LOW);
    digitalWrite(b67, LOW);
    digitalWrite(b8, LOW);
  } else if (atoi((char *)PPM.lastread) < 1200) {
    digitalWrite(b1, HIGH);
    digitalWrite(b2, HIGH);
    digitalWrite(b3, HIGH);
    digitalWrite(b4, HIGH);
    digitalWrite(b5, HIGH);
    digitalWrite(b67, LOW);
    digitalWrite(b8, LOW);
  } else {
    digitalWrite(b1, HIGH);
    digitalWrite(b2, HIGH);
    digitalWrite(b3, HIGH);
    digitalWrite(b4, HIGH);
    digitalWrite(b5, HIGH);
    digitalWrite(b67, HIGH);
    digitalWrite(b8, HIGH);
  }
  /*if(atoi((char *)temp.lastread)>19){
    temp_led.publish("on");
  }
  else{
    temp_led.publish("off");
  }*/
  delay(10000);

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
