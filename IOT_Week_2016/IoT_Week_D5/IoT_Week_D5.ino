// Includes For temperature sensor
#include <Wire.h> 

// Includes For Sharp96 LCD Display
#include "SPI.h"
#include "OneMsTaskTimer.h"
#include "LCD_SharpBoosterPack_SPI.h"

//Includes for WIFI and MQTT
#include <WifiIPStack.h>
#include <Countdown.h>
#include <MQTTClient.h>

// Includes for Utility Fiunctions
#include <itoa.h>


//TMP275 temperature sensor variables and constants
const int Tmp275_address = 0x4F;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_12Bit = 0x60;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_TemperatureReg = 0x00;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_ConfigReg = 0x01;  // I2C Temp probe write address for Tmp275  0.5%
double temp_C;
boolean P_N;              // Bit flag for Positive and Negative
unsigned int TRaw;     // Variable hold decimal value

const long TemperatureDelay = 1000;
long TemperatureTime = millis() + TemperatureDelay;

// Serial Variables
const long SerialDelay = 1000;
long SerialTime = millis() + SerialDelay;

// Sharp96 LCD Variables and class reference.
LCD_SharpBoosterPack_SPI sharp96;
uint8_t myOrientation = 0;
const long ScreenDelay = 1000;
long ScreenTime = millis() + ScreenDelay;

//Fan Variables
const int fanPin = 17; // pin 17 connector
const int fanPowerOn = HIGH;
const int fanPowerOff = LOW;
double fanTempOn = 28.00;
double fanTempOff = 27.00;
bool fanState = false; // fan is off

					   //WIFI Variables
					   // your network name also called SSID
char ssid[] = "oakesclanhome";
// your network password
char password[] = "beefdeadbeefdeadc0ffee1234";
WifiIPStack ipstack;

// MQTT Vriables
#define MQTTCLIENT_QOS2 1
char printbuf[100];
char buf[100];
int arrivedcount = 0;
//Internet based MQTT Server
char hostname[] = "iot.eclipse.org";
// Local MQTT Server
//char hostname[] = "192.168.1.202";
int port = 1883;
long MQTTPubDelay = 10000;
long MQTTPubTime = millis() + MQTTPubDelay;

MQTT::Client<WifiIPStack, Countdown> client = MQTT::Client<WifiIPStack, Countdown>(ipstack);

// MQTT Subscription and publication topics
const char* topicSubscribeHigh = "thebreadboard/setHigh";
const char* topicSubscribeLow = "thebreadboard/setLow";
const char* topicPublishTemp = "thebreadboard/Temperature";
const char* topicPublishFanState = "thebreadboard/FanState";
const char* topicPublishTime = "thebreadboard/Time";
int rc = -1;

void setup()
{
	Serial.begin(115200);
	// join i2c bus
	Wire.begin();

	// Setup TMP275
	initTemperature();  // initialise the temperature sensor

						//Setup Sharp96 LCD
	initSharp96();

	// Setup GPIO for Fan
	initFan();

	// setup WIFI
	initWIFI();

	//establish MQTT publiciations and subscriptions
	connectMQTT();
}

void loop()
{
	updateTemperature();
	updateSerial();
	updateScreen();
	updateFan();
	updateMQTTPublications();
}

/*************************************************
**************************************************
**
**  MQTT Functions
**
**************************************************
**************************************************/
void updateMQTTPublications()
{
	ftoa(buf, temp_C, 2);
	publishToMQTT(topicPublishTemp, buf);
	sprintf(buf, "%d", millis() / 1000);
	publishToMQTT(topicPublishTime, buf);
	sprintf(buf, "%d", fanState);
	publishToMQTT(topicPublishFanState, buf);

	client.yield(500);
	MQTTPubTime = millis() + MQTTPubDelay;

}

int  publishToMQTT(const char* topicName, char buf[])
{
	// Reconnect if the connection was lost
	if (!client.isConnected())
		connectMQTT();

	MQTT::Message message;
	message.qos = MQTT::QOS0;
	message.retained = false;
	message.dup = false;
	message.payload = (void*)buf;
	message.payloadlen = strlen(buf) + 1;
	return client.publish(topicName, message);
}

// This function is called via a seperate listener, not the main loop
void messageArrivedHigh(MQTT::MessageData& md)
{
	MQTT::Message &message = md.message;

	sprintf(printbuf, "%s", (char*)message.payload);
	fanTempOn = atof(printbuf);
	Serial.println(printbuf);
}
// This function is called via a seperate listener, not the main loop
void messageArrivedLow(MQTT::MessageData& md)
{
	MQTT::Message &message = md.message;
	sprintf(printbuf, "%s", (char*)message.payload);
	fanTempOff = atof(printbuf);
	Serial.println(printbuf);
}

void connectMQTT()
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
	data.clientID.cstring = (char*)"IoT Week Sample";
	rc = client.connect(data);
	if (rc != 0)
	{
		sprintf(printbuf, "rc from MQTT connect is %d\n", rc);
		Serial.print(printbuf);
	}
	Serial.println("MQTT connected");

	bool FailedSubscription = false;
	rc = client.subscribe(topicSubscribeHigh, MQTT::QOS0, messageArrivedHigh);
	if (rc != 0)
	{
		FailedSubscription = true;
	}
	rc = client.subscribe(topicSubscribeLow, MQTT::QOS0, messageArrivedLow);
	if (rc != 0)
	{
		FailedSubscription = true;
	}

	if (FailedSubscription == true)
	{
		Serial.println("MQTT subscribed Fail");
		sprintf(printbuf, "rc from MQTT subscribe is %d\n", rc);
		Serial.print(printbuf);
	}
	else
	{
		Serial.println("MQTT subscribed sucess");
	}


}

/*************************************************
**************************************************
**
**  WIFI  Functions
**
**************************************************
**************************************************/
void initWIFI()
{
	// attempt to connect to Wifi network:
	Serial.print("Attempting to connect to Network named: ");
	// print the network name (SSID);
	Serial.println(ssid);
	// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		// print dots while we wait to connect
		Serial.print(".");
		delay(300);
	}
	Serial.println("\nYou're connected to the network");
	Serial.println("Waiting for an ip address");
	while (WiFi.localIP() == INADDR_NONE) {
		// print dots while we wait for an ip addresss
		Serial.print(".");
		delay(300);
	}
	Serial.print("\nIP Address obtained");
	// We are connected and have an IP address.
	Serial.println(WiFi.localIP());
}


/*************************************************
**************************************************
**
**  Fan Functions
**
**************************************************
**************************************************/
void initFan()
{
	// Setup GPIO for Fan
	pinMode(fanPin, OUTPUT);
	digitalWrite(fanPin, fanPowerOff);  // start with fan off
}

// Note we have a little bit of hysterisis to prevent hunting of the state
void updateFan()
{
	// the fan must be on so check to see if we should turn it off
	if (fanState == true)
	{
		if (fanTempOff > temp_C)
		{
			digitalWrite(fanPin, fanPowerOff);  // turn fan off
			sharp96.text(70, 87, "OFF");
			fanState = false;
		}
	}

	// if the fan is off, then check to see if we should turn it on
	if (fanState == false)
	{
		if (temp_C >  fanTempOn)
		{
			digitalWrite(fanPin, fanPowerOn);  // turn fan on
			sharp96.text(70, 87, "ON ");
			fanState = true;
		}
	}

}
/*************************************************
**************************************************
**
**  Sharp96 Functions
**
**************************************************
**************************************************/
void initSharp96()
{
	sharp96.begin();
	sharp96.clearBuffer();
	sharp96.setFont(0);
	sharp96.text(15, 5, "TI IoT Week");
	// Box arround Temperature Reading
	for (uint8_t i = 5; i<LCD_HORIZONTAL_MAX - 5; i++)
	{
		sharp96.setXY(i, 15, 1);
		sharp96.setXY(i, 50, 1);
	}
	for (uint8_t i = 16; i<50; i++)
	{
		sharp96.setXY(5, i, 1);
		sharp96.setXY(91, i, 1);
	}
	sharp96.setCharXY(15, 20);
	sharp96.print("Temperature");

	// Box arround Setpoints	
	for (uint8_t i = 5; i<LCD_HORIZONTAL_MAX - 5; i++)
	{
		sharp96.setXY(i, 53, 1);
		sharp96.setXY(i, 74, 1);
	}
	for (uint8_t i = 54; i<74; i++)
	{
		sharp96.setXY(5, i, 1);
		sharp96.setXY(91, i, 1);
	}
	sharp96.setCharXY(15, 55);
	sharp96.print("Low");
	sharp96.setCharXY(55, 55);
	sharp96.print("High");


	sharp96.text(70, 80, "FAN");
	sharp96.text(70, 87, "OFF");
}

void updateScreen()
{
	if (ScreenTime < millis())
	{
		sharp96.setFont(1);
		sharp96.setCharXY(15, 30);
		sharp96.print(temp_C);     sharp96.print("C");
		sharp96.setFont(0);
		sharp96.setCharXY(5, 87);
		sharp96.print("Time "); sharp96.print(millis() / 1000);
		sharp96.flush();
		ScreenTime = millis() + ScreenDelay;

		sharp96.setCharXY(10, 64);
		sharp96.print(fanTempOff);
		sharp96.setCharXY(50, 64);
		sharp96.print(fanTempOn);
	}
}

/*************************************************
**************************************************
**
**  Serial Port (Console) Functions
**
**************************************************
**************************************************/
void updateSerial()
{
	if (SerialTime < millis())
	{
		Serial.print(millis() / 1000);
		Serial.print(" ");
		Serial.print(temp_C);
		Serial.println("C ");
		SerialTime = millis() + SerialDelay;
	}
}


/*************************************************
**************************************************
**
**  TMP275 Temperature Sensor Functions
**
**************************************************
**************************************************/
void initTemperature()
{
	Wire.beginTransmission(Tmp275_address);
	Wire.write(Tmpx75_ConfigReg);             // Setup configuration register
	Wire.write(Tmpx75_12Bit);          // 12-bit
	Wire.endTransmission();
	Wire.beginTransmission(Tmp275_address);
	Wire.write(Tmpx75_TemperatureReg);             // Setup Pointer Register to 0
	Wire.endTransmission();
}

void updateTemperature()
{
	// Read temperature value for TMP275
	if (TemperatureTime < millis())
	{
		Wire.requestFrom(Tmp275_address, 2); // read 2 bytes from chip
		while (Wire.available())
		{
			//Adjust 2 seperate bytes into 12 bit value (See Data Sheet)                               
			temp_C = Cal_Temp(Wire.read() << 4 | Wire.read() >> 4);
		}
		TemperatureTime = millis() + TemperatureDelay;
	}
}

double Cal_Temp(unsigned int TempRaw)
{
	double temperature;
	if (TempRaw & 0x0800)
	{
		TempRaw = 0x0FFF - TempRaw; // convert 2s compliment negative to positive
		temperature = TempRaw  *.0625 * -1;   // 0.0625 deg C / count
	}
	else
	{
		temperature = (TempRaw & 0x07FF) *.0625;   // Remove sign from middle and adjust for Deg C
	}
	return temperature;
}

/*************************************************
**************************************************
**
**  Utility Functions
**
**************************************************
**************************************************/

char *ftoa(char *a, double f, int precision)
{
	long p[] = {
		0,10,100,1000,10000,100000,1000000,10000000,100000000 };

	char *ret = a;
	long heiltal = (long)f;
	itoa(heiltal, a, 10);
	while (*a != '\0') a++;
	*a++ = '.';
	long desimal = abs((long)((f - heiltal) * p[precision]));
	itoa(desimal, a, 10);
	return ret;
}

