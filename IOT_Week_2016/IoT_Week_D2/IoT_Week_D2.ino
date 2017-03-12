#include <Wire.h> 

//temperature sensor variables
const int Tmp275_address = 0x4F;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_12Bit = 0x60;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_TemperatureReg = 0x00;  // I2C Temp probe write address for Tmp275  0.5%
const int Tmpx75_ConfigReg = 0x01;  // I2C Temp probe write address for Tmp275  0.5%
double temp_C;
boolean P_N;              // Bit flag for Positive and Negative
unsigned int TRaw;     // Variable hold decimal value

long SerialDelay = 1000;
long SerialTime = millis() + SerialDelay;
long TemperatureDelay = 1000;
long TemperatureTime = millis() + TemperatureDelay;


void setup()
{
  Serial.begin(115200);
  
     // Setup TMP275
  Wire.begin();             // join i2c bus (address optional for master) 
  
  initTemperature();
  
}

void loop()
{
   updateTemperature();
   updateSerial();
 }

void updateSerial()
{
  if (SerialTime < millis())
  {
    Serial.print(millis()/1000);  Serial.print(" ");
    Serial.print(temp_C);   Serial.println("C ");


  SerialTime = millis() + SerialDelay;
  } 
}


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
  while(Wire.available())
  { 
    //Adjust 2 seperate bytes into 12 bit value (See Data Sheet)                               
    temp_C = Cal_Temp ( Wire.read()<<4 | Wire.read() >>4);
  }  
    TemperatureTime = millis() + TemperatureDelay;
  } 
}

double Cal_Temp(unsigned int TempRaw)
{
  double temperature ;
  if (TempRaw&0x0800)  
  {
    TempRaw = 0x0FFF - TempRaw ; // convert 2s compliment negative to positive
    temperature = TempRaw  *.0625 * -1 ;   // 0.0625 deg C / count
  }
  else
  {
    temperature = (TempRaw & 0x07FF) *.0625 ;   // Remove sign from middle and adjust for Deg C
  }  
  return temperature;
}


