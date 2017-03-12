/*
   Demo program for TI Power Week and Breadboard Power System Controller
   and the Home and Industrial Automation Series
   by Peter Oakes

   Temperature Measurement using the TI TMP175 and TMP275 I2C Temperature Sensor
   http://www.ti.com/lit/ds/symlink/tmp275.pdf

   www.youtube.com/c/thebreadboardca
   16th May 2016
   The two devices are hardware and software compatable, difference is only accuracy and price
   accuracy:- TMP175 = 2% , TMP275=0.5%
   Demo uses full 12Bits so limited to 220mS per temperature update
*/

#include <Wire.h>
#include "Nextion.h"

#define LED 13

/*
  Declare objects [page id:0,component id:1, component name: "b0"].
*/
//pages
NexPage page0    = NexPage(0, 0, "page0");
NexPage page1    = NexPage(1, 0, "page1");

//page 0
NexText     acVolts =     NexText    (0, 1, "acVolts");
NexText     acAmps =      NexText    (0, 2, "acAmps");
NexText     acPower =     NexText    (0, 3, "acPower");
NexText     tTemperature = NexText    (0, 4, "acTemp");
NexWaveform wTrend =      NexWaveform(0, 5, "trend");
NexDSButton   OnOff =     NexDSButton  (0, 6, "OnOff");
NexButton   clrWattHr =   NexButton  (0, 7, "clrWattHr");
NexButton   setAmps =   NexButton  (0, 10, "setAmps");
NexButton   setVolts =   NexButton  (0, 11, "setVolts");
NexText     spVolts =     NexText    (0, 12, "spVolts");
NexText     spAmps =      NexText    (0, 13, "spAmps");
NexText     acWattHr =    NexText    (0, 14, "acWattHr");
NexDSButton   graphVolts =  NexDSButton  (0, 15, "graphVolts");
NexDSButton   graphAmps =   NexDSButton  (0, 16, "graphAmps");
NexDSButton   graphPower =  NexDSButton  (0, 17, "graphPower");
NexDSButton   graphTemp =   NexDSButton  (0, 18, "graphTemp");

// page 1
NexButton   bSave =       NexButton  (1, 1, "save");
NexButton   bCancel =       NexButton  (1, 2, "cancel");
NexText     propText =     NexText    (1, 3, "propText");
NexText     propDesc =     NexText    (1, 4, "propDesc");

char buffer[100] = {0};
char tempbuffer[10] = {0};

/*
   Register a button object to the touch event list.
*/
NexTouch *nex_listen_list[] =
{
  &page0,
  &OnOff,
  &clrWattHr,
  &setAmps,
  &setVolts,
  &bSave,
  &bCancel,
  &graphVolts,
  &graphAmps,
  &graphPower,
  &graphTemp,
  NULL
};

// TEMPERATURE MEASUREMENT VARIABLES
#define Tmp275_address   0x4F   // I2C Temp probe write address for Tmp275  0.5%
#define Tmpx75_12Bit   0x60   // I2C Temp probe write address for Tmp275  0.5%
#define Tmpx75_TemperatureReg   0x0   // I2C Temp probe write address for Tmp275  0.5%
#define Tmpx75_ConfigReg   0x01   // I2C Temp probe write address for Tmp275  0.5%
#define  voltsPer   30.0 / 1023.0
#define  ampsPer   5.0 / 1023.0
float temp_C;
boolean P_N;              // Bit flag for Positive and Negative
unsigned int TRaw;     // Variable hold decimal value

//POWER SUPPLY VARIABLES
#define MaxVolts  30.00
#define MAXAmps   5.00
double currentVolts = 0;
double currentAmps = 0;
double acAmpsVal = 0;
double acVoltsVal = 0;
double acWattsVal = 0;
double acWatthrVal = 0;
bool setVoltsPage1 = true;
// Nextion Variables
bool bgraphVolts = false;
bool bgraphAmps = false;
bool bgraphWatts = false;
bool bgraphTemp = false;

//TIMERS
#define sensDelay   100  // 100 mill second
long sensUpdate = millis() + sensDelay;
#define dispDelay   5000  // 1 second
long dispUpdate = millis() + dispDelay;
#define trendDelay  1000  // 10 second //Need some averaging really
long trendUpdate = millis() + trendDelay;
#define tempDelay   10000  // 10 second
long tempUpdate = millis() + tempDelay;
bool page1Active = false;


void setup()
{
  Serial.begin(9600);

  Wire.begin();             // join i2c bus (address optional for master)

  Wire.beginTransmission(Tmp275_address);
  Wire.write(Tmpx75_ConfigReg);             // Setup configuration register
  Wire.write(Tmpx75_12Bit);          // 12-bit
  Wire.endTransmission();

  Wire.beginTransmission(Tmp275_address);
  Wire.write(Tmpx75_TemperatureReg);             // Setup Pointer Register to 0
  Wire.endTransmission();

  pinMode(LED, OUTPUT);

  nexInit();
  /* Register the event callback function for NEXTION Display. */
  OnOff.attachPop(OnOffCallback, &OnOff);
  
  clrWattHr.attachPop(clrWattHrCallback, &clrWattHr);
  
  setAmps.attachPop(setAmpsCallback, &setAmps);
  setVolts.attachPop(setVoltsCallback, &setVolts);
  bSave.attachPop(bSaveCallback, &bSave);
  bCancel.attachPop(bCancelCallback, &bCancel);
  
  graphVolts.attachPop(graphVoltsCallback, &graphVolts);
  graphAmps.attachPop(graphAmpsCallback, &graphAmps);
  graphPower.attachPop(graphPowerCallback, &graphPower);
  graphTemp.attachPop(graphTempCallback, &graphTemp);


  spVolts.setText("00.00");
  spAmps.setText("00.00");


sendCommand("q0.picc=2");
delay(1000);
sendCommand("q0.picc=0");
sendCommand("vis q0,0");

  dbSerialPrintln("setup done");
}

void loop()
{
  if (tempUpdate <= millis() )
  {
    getTemperature();
    tempUpdate = millis() + tempDelay; // reset the clock
  }
  if (sensUpdate <= millis() )
  {
    getSensors();
    sensUpdate = millis() + sensDelay; // reset the clock
  }
  if (!page1Active)
  {
    if (trendUpdate <= millis() )
    {
      updateTrendGraph();
      trendUpdate = millis() + trendDelay; // reset the clock
    }
    if (dispUpdate <= millis() )
    {
      updateDisplays();
      dispUpdate = millis() + dispDelay; // reset the clock
    }
  }
  nexLoop(nex_listen_list);
}
void getSensors()
{
  acAmpsVal = ampsPer * analogRead(A0);
  acVoltsVal = voltsPer * analogRead(A1);
}

void updateDisplays()
{
  ftoa(tempbuffer, temp_C, 2);
  tTemperature.setText(tempbuffer);

  ftoa(tempbuffer, acAmpsVal, 2);
  acAmps.setText(tempbuffer);
  ftoa(tempbuffer, acVoltsVal, 2);
  acVolts.setText(tempbuffer);
  acWattsVal = acAmpsVal * acVoltsVal;
  acWatthrVal += acWattsVal / 3600.0;
  ftoa(tempbuffer, acWattsVal, 2);
  acPower.setText(tempbuffer);
  ftoa(tempbuffer, acWatthrVal, 2);
  acWattHr.setText(tempbuffer);
}

void updateTrendGraph()
{
  if (bgraphVolts) wTrend.addValue(0, analogRead(A0) >> 2);
  if (bgraphAmps) wTrend.addValue(1, analogRead(A1 >> 2));
  if (bgraphWatts)wTrend.addValue(2, (analogRead(A0 >> 2)*analogRead(A1 >> 2)) >> 8);
  if (bgraphTemp) wTrend.addValue(3, (int)temp_C * 5);
}
void getTemperature()
{
  // Read temperature value for TMP275
  Wire.requestFrom(Tmp275_address, 2); // read 2 bytes from chip
  while (Wire.available())
  {
    //Adjust 2 seperate bytes into 12 bit value (See Data Sheet)
    temp_C = Cal_Temp ( Wire.read() << 4 | Wire.read() >> 4);
  }
}

/*
   Button component pop callback function.
*/
void setAmpsCallback(void *ptr)
{
  delay(100);
  page1Active = true;
  setVoltsPage1 = false;
  page1.show();

}
void setVoltsCallback(void *ptr)
{
  delay(100);
  page1Active = true;
  setVoltsPage1 = true;
  page1.show();

}

void bCancelCallback(void *ptr)
{
  page1Active = false;
  page0.show();
  delay(100);
}
void bSaveCallback(void *ptr)
{
  double number;
  char temp1[18] = {0};
  propText.getText(temp1, 18);
  page0.show();
  delay(100);
  
  // There is a bug in that I have 10 Characters reserved in the text box but only 8 are returned from GetText call
  if (setVoltsPage1 == true)
  {
    spVolts.setText(temp1);
  }
  else
  {
    spAmps.setText(temp1);
  }
  page1Active = false;
}

void OnOffCallback(void *ptr)
{
  NexDSButton *btn = (NexDSButton *)ptr;
  dbSerialPrintln("b0PopCallback");
  dbSerialPrint("ptr=");
  dbSerialPrintln((uint32_t)ptr);


  /* Get the state value of dual state button component . */
  memset(buffer, 0, sizeof(buffer));
  uint32_t dual_state;
  OnOff.getValue(&dual_state);
  if (dual_state)
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
  }
}

void clrWattHrCallback(void *ptr)
{
  acWatthrVal = 0;
  ftoa(tempbuffer, acWatthrVal, 2);
  acWattHr.setText(tempbuffer);
}

void graphVoltsCallback(void *ptr)
{
  uint32_t dual_state;
  /* Get the state value of dual state button component . */
  graphVolts.getValue(&dual_state);
  bgraphVolts = dual_state;
}
void graphAmpsCallback(void *ptr)
{
  uint32_t dual_state;
  /* Get the state value of dual state button component . */
  graphAmps.getValue(&dual_state);
  bgraphAmps = dual_state;

}
void graphPowerCallback(void *ptr)
{
  uint32_t dual_state;
  graphPower.getValue(&dual_state);
  bgraphWatts = dual_state;
}
void graphTempCallback(void *ptr)
{
  uint32_t dual_state;
  graphTemp.getValue(&dual_state);
  bgraphTemp = dual_state;
}

/*************************************************
**************************************************
**
**  Obsolete Functions
**
**************************************************
**************************************************/

void sVoltsPopCallback(void *ptr)
{
  uint32_t number = 0;
  char temp[18] = {0};

  dbSerialPrint("spVoltsPopCallback ");

  //sVolts.getValue(&number);
  ftoa(temp, (double)number / 100, 2);
  //utoa(number / 100, temp, 10);
  dbSerialPrintln(temp);
  spVolts.setText(temp);
}
void sAmpsPopCallback(void *ptr)
{
  uint32_t number = 0;
  char temp[18] = {0};

  dbSerialPrint("spAmpsPopCallback ");
  //sAmps.getValue(&number);
  ftoa(temp, (double)number / 100, 2);
  //utoa(number / 100, temp, 10);
  dbSerialPrintln(temp);
  spAmps.setText(temp);
}


/*************************************************
**************************************************
**
**  Utility Functions
**
**************************************************
**************************************************/

float Cal_Temp(unsigned int TempRaw)
{
  float temperature ;
  if (TempRaw & 0x0800)
  {
    TempRaw = 0x0FFF - TempRaw ; // convert 2s compliment negative to positive
    temperature = TempRaw  * .0625 * -1 ;  // 0.0625 deg C / count
  }
  else
  {
    temperature = (TempRaw & 0x07FF) * .0625 ;  // Remove sign from middle and adjust for Deg C
  }
  return temperature;
}

char *ftoa(char *a, double f, int precision)
{
  long p[] = {
    0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000
  };

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}

