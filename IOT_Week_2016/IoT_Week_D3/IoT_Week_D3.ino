// Includes For temperature sensor
#include <Wire.h> 

// Includes For Sharp96 LCD Display
#include "SPI.h"
#include "OneMsTaskTimer.h"
#include "LCD_SharpBoosterPack_SPI.h"


//TMP275 temperature sensor variables and constants
const int Tmp275_address = 0x4F;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_12Bit = 0x60;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_TemperatureReg = 0x00;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_ConfigReg = 0x01;  // I2C Temp probe write address for Tmp275  0.5%
double temp_C;
boolean P_N;              // Bit flag for Positive and Negative
unsigned int TRaw;     // Variable hold decimal value

const long SerialDelay = 10000;
long SerialTime = millis() + SerialDelay;
long TemperatureDelay = 1000;
long TemperatureTime = millis() + TemperatureDelay;

// Sharp96 LCD Variables and class reference.
LCD_SharpBoosterPack_SPI sharp96;
uint8_t myOrientation = 0;
const long ScreenDelay = 1000;
long ScreenTime = millis() + ScreenDelay;

//Fan Variables
const int fanPin = 20; // pin 20 connector (Bottom right)
const int fanPowerOn = 0;
const int fanPowerOff = 1;
int fanTempOn = 26.00;
int fanTempOff = 22.00;
bool fanState = false; // fan is off

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
}

void loop()
{
	updateTemperature();
	updateSerial();
	updateScreen();
        updateFan();
}


/*************************************************
**************************************************
**
**  Sharp96 Functions
**
**************************************************
**************************************************/
void initFan()
{
         // Setup GPIO for Fan
        pinMode(fanPin, OUTPUT); 
        digitalWrite(fanPin, HIGH);  // start with fan off
}

// Note we have a little bit of hysterisis to prevent hunting of the state
void updateFan()
{
  // if the fan is off, then check to see if we should turn it on
  if(fanState == false)
  {
    if (temp_C >= fanTempOn) digitalWrite(fanPin, LOW);  // turn fan on
  }
  // the fan must be on so check to see if we should turn it off
  else
  {
        if (temp_C <= fanTempOff) digitalWrite(fanPin, LOW);  // turn fan on

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
        sharp96.text(10, 10, "TI IoT Week");
	for (uint8_t i = 10; i<LCD_HORIZONTAL_MAX - 10; i++)
	{
		sharp96.setXY(i, 20, 1);
	}
        sharp96.setCharXY(10, 30);
	sharp96.print("Temperature");
}

/*************************************************
**************************************************
**
**  Serial Port (Console) Functions
**
**************************************************
**************************************************/
void updateScreen()
{
	if (ScreenTime < millis())
	{
		sharp96.setFont(1);
		sharp96.setCharXY(10, 40);
		sharp96.print(temp_C);     sharp96.print("C ");
		sharp96.setFont(0);
		sharp96.setCharXY(10, 87);
		sharp96.print("Time "); sharp96.print(millis() / 1000);
		sharp96.flush();
		ScreenTime = millis() + ScreenDelay;
	}
}

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
