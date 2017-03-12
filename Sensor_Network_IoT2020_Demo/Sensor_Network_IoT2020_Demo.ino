#include <stdlib.h>
#include <SPI.h>
#include <Bridge.h>
#include <YunClient.h>
#include <IPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>
#include <Process.h>

String timeCurl = "4";
String baseUrl="http://192.168.1.229:1880/arduino/";

#define serialbufferSize 100
char inputBuffer[serialbufferSize]   ;  // we will use the buffer for serial
char printbuf[100];
int serialIndex = 0; // keep track of where we are in the buffer


#include "DHT.h"
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

int sensorPin = A0; // select the input pin for ldr
int sensorValue = 0; // variable to store the value coming from the sensor


/***************************************************************************************
****************************************************************************************
  START OF MQTT STUFF
****************************************************************************************
***************************************************************************************/

YunClient yunClient;
IPStack ipstack(yunClient);
MQTT::Client<IPStack, Countdown> client = MQTT::Client<IPStack, Countdown>(ipstack);

byte mac[] = { 0x01, 0x11, 0x22, 0x33, 0x55, 0x66 };  // replace with your device's MAC

#define  aliveTimer 30000 // 30 seconds
#define  tempTimer 10000 // 10 seconds
#define  co2Timer 10000 // 10 seconds
#define  luxTimer 10000 // 10 seconds

long isAlive = 0;  // loop timer to publish im alive and listening
long isTemp = 0;  // loop timer to publish im alive and listening
long isCo2 = 0;  // loop timer to publish im alive and listening
long isLux = 0;  // loop timer to publish im alive and listening

const char* topicHbTopic = "TheBreadboard";
const char* topicHbMessage = "Sensor 1";
const char* topicTemp = "TEMP";
const char* topicHum = "HUM";
const char* topicCo2 = "CO2";
const char* topicLux = "LUX";
const char* topicSub1 = "sub1";
const char* topicSub2 = "sub2";
const char* topicSub3 = "sub3";

char * hostname = "192.168.1.229";
#define port 1883

void messageArrived(MQTT::MessageData& md)
{
  MQTT::Message &message = md.message;
  char myPayload[30] = {""};
  char myTopic[30] = {""};
  strcpy(myPayload, (char*)message.payload); // get the payload first so it is not overwritten
  myPayload[message.payloadlen] = 0; // terminate the payload string
  md.topicName.lenstring.data[md.topicName.lenstring.len] = 0; // now terminate the topic correctly
  strcpy(myTopic, md.topicName.lenstring.data); // now copy the topic

  sprintf(printbuf, "%d %s %s", message.payloadlen, myTopic,  myPayload);
  Serial.println(printbuf);
  if (message.payloadlen > 2) // make sure we have some data
  {
    if ((strcmp (topicSub1, myTopic) == 0))
    {
      sprintf(printbuf, "SETCOLOUR %s", myPayload);
      //SendNRF(1, printbuf);
    }
    if ((strcmp (topicSub2, myTopic) == 0))
    {
      sprintf(printbuf, "SETNAME %s", myPayload);
      //SendNRF(2, printbuf);
    }
    if ((strcmp (topicSub3, myTopic) == 0))
    {
      //sprintf(printbuf, "SETCOLOUR %s", myPayload);
      //SendNRF(3, myPayload);
    }
  }
}


void connect()
{
  sprintf(printbuf, "Connecting to %s:%d\n", hostname, port);
  Serial.print(printbuf);
  int rc = ipstack.connect(hostname, port);
  if (rc != 1)
  {
    sprintf(printbuf, "rc from TCP connect is %d\n", rc);
    Serial.print(printbuf);
  }

  Serial.println("MQTT connecting");
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.MQTTVersion = 3;
  data.clientID.cstring = (char*)"Sensor Hub 1";
  rc = client.connect(data);
  if (rc != 0)
  {
    sprintf(printbuf, "rc from MQTT connect is %d\n", rc);
    Serial.print(printbuf);
  }
  Serial.println("MQTT connected");
  rc = client.subscribe(topicSub1, MQTT::QOS0, messageArrived);
  rc = client.subscribe(topicSub2, MQTT::QOS0, messageArrived);
  rc = client.subscribe(topicSub3, MQTT::QOS0, messageArrived);
  if (rc != 0)
  {
    sprintf(printbuf, "rc from MQTT subscribe is %d\n", rc);
    Serial.print(printbuf);
  }
  Serial.println("MQTT subscribed");
}

/***************************************************************************************
****************************************************************************************
  END OF MQTT STUFF
****************************************************************************************
***************************************************************************************/


void setup() {

  Serial.begin(57600);
  Bridge.begin();
  Console.begin();

  printf("\n\r Starting \n\r");
  connect();

  dht.begin();

  printf("\n\r examples GettingStarted \n\r");

  isAlive = millis();
  isTemp  = millis();
  isCo2 = millis();
  isLux = millis();
}

void loop(void) {

  //MQTT STUFF
  if (!client.isConnected())  connect();
  client.yield(300);
  //END MQTT STUFF
  sendHeartbeat(); // im alive
  sendTempHum(); // Temperature and Humidity
  sendLux();// Light Level
}

void sendHeartbeat() {
  if (millis() - isAlive >= aliveTimer) {
    MQTT::Message message;
    int arrivedcount = 0;
    char buf[16];
    sprintf(buf, topicHbMessage);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf) + 1; message.payloadlen = strlen(buf) + 1;
    int rc = client.publish(topicHbTopic, message);
    while (arrivedcount == 1) client.yield(1000); // wait for conformation
    isAlive = millis();
  }
}
void sendTempHum() {
  if (millis() - isTemp >= tempTimer) {
    MQTT::Message message;
    int arrivedcount = 0;
    char buf[16];

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    dtostrf(h, 3, 2, buf);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf) + 1; message.payloadlen = strlen(buf) + 1;
    int rc = client.publish(topicHum, message);
    while (arrivedcount == 1) client.yield(1000); // wait for conformation
    dtostrf(t, 3, 2, buf);
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf) + 1; message.payloadlen = strlen(buf) + 1;
    rc = client.publish(topicTemp, message);
    while (arrivedcount == 1) client.yield(1000); // wait for conformation
    String mystring = "Temperature/" +String(t);
    runCurl(mystring);
    mystring = "Humidity/" +String(h);
    runCurl(mystring);
    isTemp = millis();
  }
}
void sendLux() {
  if (millis() - isLux >= luxTimer) {
    MQTT::Message message;
    int arrivedcount = 0;
    char buf[16];
    sensorValue = analogRead(sensorPin); // read the value from the sensor

    itoa(sensorValue, buf, 10);
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf) + 1; message.payloadlen = strlen(buf) + 1;
    int rc = client.publish(topicLux, message);
    while (arrivedcount == 1) client.yield(1000); // wait for conformation
    String mystring = "light1/" +String(sensorValue);;
    runCurl(mystring);
    isLux = millis();
  }
}

void runCurl(String myvalue) {
  Console.println("Call runCurl");
  String mystring=baseUrl+myvalue;
  Console.println(mystring);
  Process p;    // Create a process and call it "p"
  p.begin("curl");  // Process that launch the "curl" command
  p.addParameter("-m");
  p.addParameter(timeCurl);
  p.addParameter(mystring); // Add the URL parameter to "curl"
  p.run();    // Run the process and wait for its termination

}


