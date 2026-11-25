
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define DEBUG  1 //1=use Serial debug 0=do not pollute the rx/tx lines.

#if DEBUG == 1

  #define debugPrint(x) Serial.print(x)
  #define debugPrintln(x) Serial.println(x) 
  #else
  #define debugPrint(x) 
  #define debugPrintln(x)
  
#endif

volatile unsigned long lasttime = 0;  // Global counter variable of current position relative to index switch
volatile int Vac = 1;
volatile int Enabled = 0;
volatile int High = 0;
volatile int Low  = 0;

enum CONTROLStates {   // Define enumerated type for state machine states
  IDLE, 
  STOP,
  READPRESSURE,       //From Sensor
  DISPLAYPRESSURE,    //TM1637
  READMQTT,
  PUBLISHMQTT,        //Status, Duty Cycle
  INCREASE,           //trigger onpoint setting
  DECREASE,           //trigger offpoint setting
  ONPOINT,
  OFFPOINT
  
};
CONTROLStates prior_state, state;            // Global variables to store the prior and current states
int MQTTretries = 0;
uint32_t x=0;
int MQTTPings = 0;

//define PINS
#define pVac  36
#define pUP   17 
#define pDN   18 
#define pSWITCH 19
#define LED 2
#include "secrets.h"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>


// Setup Publish 
Adafruit_MQTT_Publish VacOn = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/VacOn");
Adafruit_MQTT_Publish VacOff = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/VacOff");
Adafruit_MQTT_Publish VacDuty = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/VacDuty");
Adafruit_MQTT_Publish VacVal = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/VacVal");
Adafruit_MQTT_Publish MQTTConnections = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/MQTTConnections");

//Setup READ Access
Adafruit_MQTT_Subscribe ReadVacEnable = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/SetVacEnable", MQTT_QOS_1);
Adafruit_MQTT_Subscribe ReadVacOn = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/SetVacON", MQTT_QOS_1);
Adafruit_MQTT_Subscribe ReadVacOff = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/SetVacOFF", MQTT_QOS_1);

// Setup Write Access
Adafruit_MQTT_Publish SetVacEnable = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SetVacEnable");
Adafruit_MQTT_Publish SetVacOn = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SetVacON");
Adafruit_MQTT_Publish SetVacOff = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/SetVacOFF");

void IRAM_ATTR handleButtonUP() {// button manage with Interrupt
}
void IRAM_ATTR handleButtonDN() {
}

void callbackSetVacEnable(char *data, uint16_t len) {
  debugPrint("* SetVacEnable callback: ");
  debugPrintln(data);
  
  if (strcmp((char *)ReadVacEnable.lastread, "ON") == 0) {
    digitalWrite(LED, LOW); 
  }//if

  if (strcmp((char *)ReadVacEnable.lastread, "OFF") == 0) {
    digitalWrite(LED, HIGH); 
  }//if
}//positionSetVacEnable

void callbackSetVacON(unsigned long x) {
  debugPrint("* SetVacON callback: ");
  debugPrintln(x);
  

}//callbackSetVacON

void callbackSetVacOFF(unsigned long x) {
  debugPrint("* SetVacOFF callback: ");
  debugPrintln(x);

}//callbackSetVacOFF

void MQTT_connect(); //gotta function-prototype for functions in support.ino

void setup() {

#if DEBUG == 1
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif
  
  //debugPrintln(ESP.getFlashChipSize());
  // Connect to WiFi access point.
  debugPrint("Connecting to ");
  debugPrintln(WLAN_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(5000);
    debugPrint(".");
  }
  debugPrint(millis());
  debugPrintln(AIO_USERNAME);

  debugPrintln("WiFi connected");
  debugPrintln("IP address: "); 
  debugPrintln(WiFi.localIP());
#if DEBUG == 1
  listNetworks();
#endif

// pinSETUP

  pinMode(pSWITCH,OUTPUT);
//  attachInterrupt(digitalPinToInterrupt(pEndstop), handleENDSTOP, FALLING);  // Attach interrupt on falling edge (low pulse)
  

//INITIALIZE MQTT
  ReadVacEnable.setCallback(callbackSetVacEnable);
  ReadVacOn.setCallback(callbackSetVacON);
  ReadVacOff.setCallback(callbackSetVacOFF);
  
  mqtt.subscribe(&ReadVacEnable);
  mqtt.subscribe(&ReadVacOn);
  mqtt.subscribe(&ReadVacOff);
  MQTT_connect();
  mqtt.processPackets(5000);
  delay(5000);
  mqtt.processPackets(5000);
  delay(5000);
  mqtt.processPackets(5000);
  delay(5000);
  mqtt.processPackets(5000);

// would really like to understand why ^ this isn't reading...  
  Serial.print(F("ReadVacEnable: "));
  Serial.println((char *)ReadVacEnable.lastread);
  Serial.print(F("ReadVacOn: "));
  Serial.println((char *)ReadVacOn.lastread);
  Serial.print(F("ReadVacOff: "));
  Serial.println((char *)ReadVacOff.lastread);
// but then it does read in the V loop?
      
      if (strcmp((char *)ReadVacEnable.lastread, "") == 0) {
        Serial.print(F("ReadVacEnable Not Found, Publishing default"));
        if(! SetVacEnable.publish("OFF")) debugPrintln("Failed SetVacEnable.publish");
          mqtt.processPackets(10);
      }

      if (strcmp((char *)ReadVacOn.lastread, "") == 0) {
        if(! SetVacOn.publish("10")) debugPrintln("Failed SetVacOn.publish");
          mqtt.processPackets(10);
      }
      if (strcmp((char *)ReadVacOff.lastread, "") == 0) {
        if(! SetVacOff.publish("15")) debugPrintln("Failed SetVacOff.publish");
          mqtt.processPackets(10);
      }

  
  lasttime=millis();
  debugPrintln("Publishing Initial Values");
  mqtt.processPackets(10);
  
  debugPrintln("Leaving Setup");
  lasttime=millis();
  
}//setup

int fReadPressure(){
  int analogValue = analogRead(pVac);
  int scaledValue = map(analogValue,600,2096,15,0);
  //int scaledValue = analogValue;
  char msg[16];
  itoa(scaledValue,msg,10);
  
  if(! VacVal.publish(msg)) {
    debugPrintln("Failed VacVal.publish");
  state=IDLE; // this really happens after the switch statement though
  }//if
  // Simple Trigger. no need for PID with VAC.
  if ( Vac > atoi((char *)ReadVacOff.lastread) ) state=OFFPOINT;
  if ( Vac < atoi((char *)ReadVacOn.lastread) ) state=ONPOINT;

  return scaledValue;
}//fReadPressure

void fONpoint(){ 
      //triggered by VacPressure < VacON from READPRESSURE
      //if VacEnable turn on
      digitalWrite(pSWITCH,HIGH);
}//fONpoint()

void fOFFpoint(){ 
      //triggered by VacPressure > VacOFF from READPRESSURE
      //if VacEnable turn on
      digitalWrite(pSWITCH,LOW);
}//fOFFpoint()

void loop() {
  state = IDLE;
  if (millis()-lasttime > 10000) {//nonblocking wait
    //listNetworks();
    state=READPRESSURE;
    mqtt.processPackets(500);
    lasttime=millis();
    if(! VacDuty.publish(millis())) {
      debugPrintln("Failed VacDuty.publish");
        MQTT_connect();
      }//if published

  Serial.print(F("ReadVacEnable: "));
  Serial.println((char *)ReadVacEnable.lastread);
  Serial.print(F("ReadVacOn: "));
  Serial.println((char *)ReadVacOn.lastread);
  Serial.print(F("ReadVacOff: "));
  Serial.println((char *)ReadVacOff.lastread);

    }//if millis

  
  switch (state) {// ** Decisions states and actions are made in the subroutines ** 
    case IDLE:
      break;
    case STOP:
      break;
    case READPRESSURE:
      Vac = fReadPressure();
      break;
    case DISPLAYPRESSURE:
      //fDisplayPressure();
      break;
    case ONPOINT:
      fONpoint();
      break;
    case OFFPOINT:
      fOFFpoint();
      break;
    case READMQTT:
      //fReadMQTT();
      break;
}//switch case
  
// only one item processed per loop
  state=IDLE;

}//loop
