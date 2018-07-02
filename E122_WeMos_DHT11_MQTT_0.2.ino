/*
 E122 - Wemos With MQTT ,DHT11, and Pulsecounter - Version 0.2
 Updated: April 17th, 2018.

 Integrated from components picked off the net by Prof.KP. 
 Check  the configuration section for what you can tinker with. 
 Modify code on your own risk.... But, ... do take risks....

********/
/*  Installation of drivers and other set up needed - Check CANVAS...

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/


#include <ESP8266WiFi.h>
#include "PubSubClient.h"
#include "info.h"
#include "DHT.h"

// CONFIGURATION SETTINGS ....BEGIN

//Wifi Settings
//const char* ssid = "DLabsPrivate1";
//const char* password = "L3tsM@keSometh1n";
const char* ssid = "Stevens-Media";
const char* password = "Stevens1870";
// ANY WEMOS WITH A STICKER ON THE BACK IS REGISTERED TO THE NETWORK... 
// THOSE WITHOUT THE STICKERS DO NOT WORK... 


//MQTT Settings
const char* mqtt_server = "155.246.18.226";
const char* MQusername = "jojo";
const char* MQpassword = "hereboy";
// This magic word is added to the topic for control messages E122/<MAC4>/Control/MAGIC_WORD Param, Value
// Params Accepted PUBLISH_DELAY  VALUE in ms
//                 SLEEP_DELAY VALUE in ms
// 
const char* MAGIC_WORD ="XYZZYPQQRT";
#define PUBLISH_DELAY  10000  
#define SLEEP_DELAY 2000
#define DEEP_SLEEP_SECONDS  10
//const char* mqtt_server = "broker.hivemq.com"; //This is a public server - no username/pwd
// Sleep delay is how long Wemos goes into sleep mode before re-checking --
// May help conserve power in the field -- you may have to check. 
// Want to put it in Deepsleep while in field -else will burn through batteries. - power conserve mode...
  // convert to microseconds
  //ESP.deepSleep(sleepSeconds * 1000000);



//Set up the DHT11
#define DHTPIN 4 //This is GPIO4 which is WeMos D2. Probably just hide this from students
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);


//MQTT  - 5000 ms is 5 sec. 


// 
//Note that since this is a real Wemos board -- it runs forever as opposed to 
// the fakemos -- http://www.dmi.stevens.edu/fakemos/

// setup pulsecounter
volatile unsigned long counter;
const unsigned long interval = 1000;  // ms
#define signalPin 13 
 //GPIO13 in  WeMos D1 R2 is D7
void eventISR (){
    counter++; } 

    
// CONFIGURATION SETTINGS ....END

WiFiClient espClient;
info board_info;
PubSubClient client(espClient);
long lastMsg = 0;
char msg1[20],msg2[20],msg3[20];
char MQTopic1[50],MQTopic2[50],MQTopic3[50];
char ControlTopic[50];
char BoardMac4[5];
int value = 0;
float temp, hum,freq;
int tm = 0;
int dt,ta,ha;


void setup_wifi() {
//pulsecounter
// We are sticking to 115200 for this project 
  Serial.begin (115200);    
// Set the DIO Pin to input
  pinMode (signalPin, INPUT); 
// end pulsecounter

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  // Board Last 4.
 BoardMac4[0]=board_info.mac()[12];
 BoardMac4[1]=board_info.mac()[13];
 BoardMac4[2]=board_info.mac()[15];
 BoardMac4[3]=board_info.mac()[16];
 BoardMac4[4]='\0'; 
 //
 // Need the null termination for the string stuff to work, declare 1 extra character...
 //
  Serial.println(" MAC  4 of the board");
  Serial.println(BoardMac4);
  sprintf(MQTopic1,"E122/%4s/Temperature",BoardMac4);
  sprintf(MQTopic2,"E122/%4s/Humidity",BoardMac4);
  sprintf(MQTopic3,"E122/%4s/WindSpeed",BoardMac4);
  sprintf(ControlTopic,"E122/%4s/control/%s",BoardMac4,MAGIC_WORD);
  Serial.println("Setting the topics");
  Serial.println(MQTopic1);
  Serial.println(MQTopic2);
  Serial.println("This Wemos Control channel is");
  Serial.println(ControlTopic);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQusername,MQpassword)) {
//    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish(MQTopic1, "00000");
      // ... and resubscribe ---- Dont subscribe KP
      // Wemos subscribes to its control channel E122/<mac4>/control
       client.subscribe(ControlTopic);
      // #KP - No announcements --- No Subscribes. 
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  delay(1000);
  Serial.println("********************************************************");
  Serial.println("* E 122 Design II: Field Sustainable Systems           *");
  Serial.println("*                                                      *");
  Serial.println("* Wemos Firmware with MQTT and DHT11 Support           *");
  Serial.println("*                                                      *");
  Serial.println("* Version 0.1  Feb 20 2018  (Stevens)                  *");
  Serial.println("********************************************************");
  Serial.print("Board Mac Address:");
  Serial.println(board_info.mac());
  setup_wifi();
  dht.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

//pulsecounter
// We are sticking to 115200 for this project 
  Serial.begin (115200);    
// Set the DIO Pin to input
  pinMode (signalPin, INPUT); 
// end pulsecounter

}

void loop() {
// Pulsecounter
unsigned long thiscount;

// this helps to process, save and ship off to MQTT.  
  thiscount = countPulses(1000);
 // Serial.println(thiscount);
  
// end pulsecount
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  dt = now - lastMsg ;
 
  if (dt > PUBLISH_DELAY) {
    lastMsg = now;
    temp = dht.readTemperature(true);
    hum = dht.readHumidity();
    snprintf (msg1, 20, "%d", (int) temp);
    snprintf (msg2, 20, "%d", (int) hum);
    snprintf (msg3, 20, "%d", (int) thiscount);
    Serial.print("Published :" );
    Serial.print(MQTopic1);
    Serial.print(" with value: " );
     Serial.println(msg1);
    client.publish(MQTopic1, msg1);
     Serial.print("Published :" );
    Serial.print(MQTopic2);
    Serial.print(" with value: " );
    Serial.println(msg2);
    client.publish(MQTopic2, msg2);
    Serial.print("Published :" );
    Serial.print(MQTopic3);
    Serial.print(" with value: " );
    Serial.println(msg3);
    client.publish(MQTopic3, msg3);
}
//ESP.deepSleep(DEEP_SLEEP_SECONDS * 1000000);
//digitalWrite(BUILTIN_LED, LOW); 
delay(SLEEP_DELAY);
//digitalWrite(BUILTIN_LED, HIGH); 
}

//pulsecount
unsigned long countPulses (int interval)
// interval defines 
  {
// reset  set counter to zero 
    counter = 0;
// Attach the interrupt - count - detach
    attachInterrupt (digitalPinToInterrupt(signalPin), eventISR, FALLING);
    delay(interval);
    detachInterrupt(1);
// Return the counter to the caller
       return counter;
  }  
// end pulsecount
