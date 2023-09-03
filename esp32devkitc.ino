//Include the libraries
#include "WiFi.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <MQUnifiedsensor.h>
#include <Wire.h>

/**************************** Definitions ************************************/
#define placa "ESP-32"
#define Voltage_Resolution 3.3
#define pin 33 //Analog input for gas sensor
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 12 
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm

#define LIGHT_SENSOR_PIN 32 // ESP32 pin GIOP32
#define LDR_LIGHT 25 // ESP32 pin GIOP25
#define GAS_LIGHT 26 // ESP32 pin GIOP26
#define BUZZER 12 // ESP32 pin GIOP12
#define D_gas 27 // ESP32 pin GIOP27
#define motor 14 // ESP32 pin GIOP14
#define BUZZER_CHANNEL 5

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "set_yours"
#define WLAN_PASS       "set_yours"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "set_yours"
#define AIO_KEY         "set_yours"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP32 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiClientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/**************************** Declare Sensor ************************************/
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

/****************************** Feeds ***************************************/

// Setup a feed called 'temp + hum' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>

Adafruit_MQTT_Publish CO2_reads = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/CO2");
Adafruit_MQTT_Publish NH4_reads = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/NH4");
Adafruit_MQTT_Publish PPM_reads = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/PPM");
Adafruit_MQTT_Publish LDR_reads = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/LDR");


//variables to keep track of the timing 
unsigned long start_time = 0;  
unsigned long last_time = 0;

/*************************** Sketch Code ************************************/

void MQTT_connect();

void setup() {
  
  Serial.begin(115200);
  delay(10);
  pinMode(LDR_LIGHT, OUTPUT);
  pinMode(GAS_LIGHT, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(D_gas, INPUT);
  pinMode(motor, OUTPUT);
  ledcAttachPin(BUZZER, 5);
  //Set math model to calculate the PPM concentration and the value of constants
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.init(); 
  MQ135.setRL(1);
  
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

/*****************************  MQ CAlibration ********************************************/ 
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");

  if(isinf(calcR0)) {Serial.println("Warning: Conection issue, R0 is infinite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply"); while(1);}

  Serial.println("** Values from MQ-135 ****");
  Serial.println("|    CO    |  Alcohol |   CO2      |  Toluen  |  NH4     |  Aceton  |   PPM   |     LDR     |");
}


void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  MQTT_connect();

  MQ135.update(); // Update data, the arduino will read the voltage from the LDR pin
  int ppm = analogRead(33);
  int LDRvalue = analogRead(LIGHT_SENSOR_PIN);

  MQ135.setA(605.18); MQ135.setB(-3.937); // Configure the equation to calculate CO concentration value
  float CO = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  MQ135.setA(77.255); MQ135.setB(-3.18); //Configure the equation to calculate Alcohol concentration value
  float Alcohol = MQ135.readSensor(); // SSensor will read PPM concentration using the model, a and b values set previously or from the setup

  MQ135.setA(110.47); MQ135.setB(-2.862); // Configure the equation to calculate CO2 concentration value
  float CO2 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  MQ135.setA(44.947); MQ135.setB(-3.445); // Configure the equation to calculate Toluen concentration value
  float Toluen = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup
  
  MQ135.setA(102.2 ); MQ135.setB(-2.473); // Configure the equation to calculate NH4 concentration value
  float NH4 = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  MQ135.setA(34.668); MQ135.setB(-3.369); // Configure the equation to calculate Aceton concentration value

  float Aceton = MQ135.readSensor(); // Sensor will read PPM concentration using the model, a and b values set previously or from the setup

  Serial.print("|   "); Serial.print(CO); 
  Serial.print("   |   "); Serial.print(Alcohol);
  Serial.print("   |   "); Serial.print(CO2 + 400); 
  Serial.print("   |   "); Serial.print(Toluen); 
  Serial.print("   |   "); Serial.print(NH4); 
  Serial.print("   |   "); Serial.print(Aceton);
  Serial.print("   |   "); Serial.print(ppm);
  Serial.print("   |   "); Serial.print(LDRvalue);
  Serial.println("   |"); 


  // Now we can publish stuff!
  
  CO2_reads.publish(CO2+400);
  NH4_reads.publish(NH4);
  PPM_reads.publish(ppm);
  LDR_reads.publish(LDRvalue);

  if (LDRvalue < 800) {
    digitalWrite(LDR_LIGHT, HIGH);
  } 
  else {
    digitalWrite(LDR_LIGHT, LOW);
  } 
//Turn on the motor
  if(digitalRead(D_gas)==0){
    last_time = millis();
    start_time = last_time;
    digitalWrite(GAS_LIGHT, HIGH);
    analogWrite(motor, 255);
    tone(BUZZER, 4000,500);
    delay(1000);
    
  }
  else{
    start_time = millis();
    if (start_time - last_time > 15000) {
      digitalWrite(GAS_LIGHT, LOW);
      analogWrite(motor, 0);
    }
  }


  delay(20000);

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

  uint8_t retriess = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retriess--;
       if (retriess == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}