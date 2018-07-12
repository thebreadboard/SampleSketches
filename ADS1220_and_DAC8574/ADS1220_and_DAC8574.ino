#include "Protocentral_ADS1220.h"
#include <SPI.h>
#include <Wire.h>

#define PGA 1
#define VREF 2.048
#define VFSR VREF/PGA
#define FSR (((long int)1<<23)-1)

//#define DAC8574_ADD 0x98
#define DAC8574_ADD 0x4C // allowing for RW bit being added
#define DAC8574_CTR 0x34 // broadcast to all dac channels

volatile byte MSB;
volatile byte data;
volatile byte LSB;
//volatile char SPI_RX_Buff[3];
volatile byte *SPI_RX_Buff_Ptr;
byte val = 0;
//Protocentral_ADS1220 ADS1220;

unsigned int Sin_tab[256] = {  32768,33572,34376,35178,35980,36779,37576,38370,39161,39947,40730,41507,42280,
                               43046,43807,44561,45307,46047,46778,47500,48214,48919,49614,50298,50972,51636,
                               52287,52927,53555,54171,54773,55362,55938,56499,57047,57579,58097,58600,59087,
                               59558,60013,60451,60873,61278,61666,62036,62389,62724,63041,63339,63620,63881,
                               64124,64348,64553,64739,64905,65053,65180,65289,65377,65446,65496,65525,65535,
                               65525,65496,65446,65377,65289,65180,65053,64905,64739,64553,64348,64124,63881,
                               63620,63339,63041,62724,62389,62036,61666,61278,60873,60451,60013,59558,59087,
                               58600,58097,57579,57047,56499,55938,55362,54773,54171,53555,52927,52287,51636,
                               50972,50298,49614,48919,48214,47500,46778,46047,45307,44561,43807,43046,42280,
                               41507,40730,39947,39161,38370,37576,36779,35980,35178,34376,33572,32768,31964,
                               31160,30358,29556,28757,27960,27166,26375,25589,24806,24029,23256,22490,21729,
                               20975,20229,19489,18758,18036,17322,16617,15922,15238,14564,13900,13249,12609,
                               11981,11365,10763,10174,9598,9037,8489,7957,7439,6936,6449,5978,5523,5085,4663,
                               4258,3870,3500,3147,2812,2495,2197,1916,1655,1412,1188,983,797,631,483,356,247,
                               159,90,40,11,1,11,40,90,159,247,356,483,631,797,983,1188,1412,1655,1916,2197,
                               2495,2812,3147,3500,3870,4258,4663,5085,5523,5978,6449,6936,7439,7957,8489,9037,
                               9598,10174,10763,11365,11981,12609,13249,13900,14564,15238,15922,16617,17322,
                               18036,18758,19489,20229,20975,21729,22490,23256,24029,24806,25589,26375,27166,
                               27960,28757,29556,30358,31160,31964};


void setup()
{
//  pinMode(ADS1220_CS_PIN, OUTPUT);
//  pinMode(ADS1220_DRDY_PIN, INPUT);
  Wire.begin(DAC8574_ADD);
//  InitDAC();
//  ADS1220.begin();
}
 
void loop()
{
//  long int bit32;
//  long int bit24;
//  byte *config_reg;
//  
//  //if((digitalRead(ADS1220_DRDY_PIN)) == LOW)             //        Wait for DRDY to transition low
//  { 
//    SPI_RX_Buff_Ptr = ADS1220.Read_Data();
//  }
//
//  if(ADS1220.NewDataAvailable = true)
//  {
//  ADS1220.NewDataAvailable = false;
//
//  MSB = SPI_RX_Buff_Ptr[0];    
//  data = SPI_RX_Buff_Ptr[1];
//  LSB = SPI_RX_Buff_Ptr[2];
//
//  bit24 = MSB;
//  bit24 = (bit24 << 8) | data;
//  bit24 = (bit24 << 8) | LSB;                                 // Converting 3 bytes to a 24 bit int
//  
//  bit24= ( bit24 << 8 );
//  bit32 = ( bit24 >> 8 );                      // Converting 24 bit two's complement to 32 bit two's complement
//  float Vout = (float)((bit32*VFSR*1000)/FSR);     //In  mV
//  Serial.print("Vout in mV : ");  
//  Serial.print(Vout,3);
//  Serial.print("  32bit HEX : ");
//  Serial.println(bit32,HEX); 
//  }


// Wire.beginTransmission(44); // transmit to device #44 (0x2c)
//                               // device address is specified in datasheet
//   Wire.write(val);             // sends value byte  
//   Wire.endTransmission();     // stop transmitting
//
//   val++;        // increment value
//   if(val == 64) // if reached 64th position (max)
//   {
//     val = 0;    // start over from lowest value
//   }
 
  SetDAC(0,0x4000);
  SetDAC(1,0x8000);
  SetDAC(2,0xC000);
  SetDAC(3,0xFFFF);
while(1)
{
  for (int x= 0 ; x<256 ; x++)
  {
   SetDAC(0, Sin_tab[x]);
  }
}
}
// 
//void InitDAC()
//{
//  Wire.beginTransmission(DAC8574_ADD); // transmit to device
//  Wire.endTransmission();    // stop transmitting
//} 
void SetDAC(unsigned int channel, unsigned int value)
{
  int DACCmd = 0;
  switch (channel)
  {
   case 0:  DACCmd=  0x10; break;
   case 1:  DACCmd=  0x12; break;   
   case 2:  DACCmd=  0x14; break;   
   case 3:  DACCmd=  0x16; break;  
   default: break;
 }
  Wire.beginTransmission(DAC8574_ADD); // transmit to device 
  Wire.write(DACCmd); 
  Wire.write(value);        // 
//  Wire.write(0x00);              // sends one byte
  Wire.endTransmission();    // stop transmitting
}
