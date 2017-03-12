/*
   Demo program for Breadboard Power System Controller
   and the Home and Industrial Automation Series
   by Peter Oakes

   Temperature Measurement using the TI TMP175 and TMP275 I2C Temperature Sensor
   http://www.ti.com/lit/ds/symlink/tmp275.pdf

   www.youtube.com/c/thebreadboardca
   26th June 2016

This progrm currently assumes a Nextion 7" Display is connected to Serial3 and debug uses Serial0 (Default)
This version reads ADC0 and ADC1 for Volts and Amps but does not currently output setpoints to a DAC. 
A later version will include better resolution ADC's and DACs for connecting to a real power stage.

*/

#include <Wire.h>
#include "Nextion.h"
#include <ClickEncoder.h>
#include <TimerOne.h>
#define VERBOSECASE(label) case label: Serial.println(#label); break;

#define LED 13

/*
  Declare NEXTION objects [page id:0,component id:1, component name: "b0"].
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

// Keypad
NexButton   bSave =       NexButton  (0, 21, "save");
NexButton   bCancel =       NexButton  (0, 20, "cancel");
NexText     propText =     NexText    (0, 35, "propText");
NexText     propDesc =     NexText    (0, 34, "propDesc");
NexText     errorMSG =     NexText    (0, 37, "msg");

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
#define MAXVolts  30.00
#define MINVolts 0.0
#define MINAmps 0.0
#define MAXAmps   5.00
double currentVolts = 0;
double currentAmps = 0;
double acAmpsVal = 0;
double acVoltsVal = 0;
double acWattsVal = 0;
double acWatthrVal = 0;
bool setVoltsKeypad = true;
// Nextion Variables
bool bgraphVolts = false;
bool bgraphAmps = false;
bool bgraphWatts = false;
bool bgraphTemp = false;

//TIMERS
#define sensDelay   100  // 100 mill second
long sensUpdate = millis() ;
#define dispDelay   5000  // 1 second
long dispUpdate = millis() ;
#define trendDelay  1000  // 10 second //Need some averaging really
long trendUpdate = millis() + trendDelay;
#define tempDelay   10000  // 10 second
long tempUpdate = millis();

//Rotary Encoder Stuff
ClickEncoder *encoderVolts;
ClickEncoder *encoderAmps;
double encoderLastVolts, encoderLastAmps;

void timerIsr() {
  encoderVolts->service();
  encoderAmps->service();
}


void setup()
{
  Serial.begin(9600);
  Wire.begin();             // join i2c bus (address optional for master)
  pinMode(LED, OUTPUT);

  initEncoders();

  initTemperature();

  nexInit();
  showSplash();
  /* Register the event callback function for NEXTION Display. */
  initNextionCallBacks();

  spVolts.setText("00.00");
  spAmps.setText("00.00");

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
  nexLoop(nex_listen_list);
  getEncoderValues();
}

/*************************************************
**************************************************
**
**  Encoder Functions
**
**************************************************
**************************************************/
void initEncoders()
{
  encoderVolts = new ClickEncoder(2, 3, 4, 4);
  encoderAmps = new ClickEncoder(5, 6, 7, 4);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  encoderLastVolts = -1;
  encoderLastAmps = -1;
}
void getEncoderValues()
{
  bool updateSP = false;
  double tempCurrentVolts = currentVolts;
  double MovedVolts = encoderVolts->getValue();  // get change since last read
  tempCurrentVolts += MovedVolts / 100;

  if (tempCurrentVolts != encoderLastVolts)
  {
    if (tempCurrentVolts > MAXVolts) tempCurrentVolts = MAXVolts;
    else if (tempCurrentVolts < MINVolts) tempCurrentVolts = MINVolts;
    encoderLastVolts = tempCurrentVolts;
    currentVolts = tempCurrentVolts;
    updateSP = true;
    dbSerialPrint("encoderVolts Value: ");
    dbSerialPrintln(tempCurrentVolts);
  }

  double tempCurrentAmps = currentAmps;
  double MovedAmps = encoderAmps->getValue();  // get change since last read
  tempCurrentAmps += MovedAmps / 100;
  if (tempCurrentAmps != encoderLastAmps)
  {
    if (tempCurrentAmps > MAXAmps) tempCurrentAmps = MAXAmps;
    else if (tempCurrentAmps < MINAmps) tempCurrentAmps = MINAmps;
    encoderLastAmps = tempCurrentAmps;
    currentAmps = tempCurrentAmps;
    updateSP = true;
    dbSerialPrint("encoderAmps Value: ");
    dbSerialPrintln(tempCurrentAmps);
  }
  if (updateSP) updateSetpoints();

  // Check Buttons
  ClickEncoder::Button encBtnVolts = encoderVolts->getButton();
  if (encBtnVolts != ClickEncoder::Open)
  {
    dbSerialPrint("encBtnVolts: ");
    switch (encBtnVolts) {
        VERBOSECASE(ClickEncoder::Pressed);
        VERBOSECASE(ClickEncoder::Held)
        VERBOSECASE(ClickEncoder::Released)
        VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
        dbSerialPrintln("ClickEncoder::DoubleClicked");
        encoderVolts->setAccelerationEnabled(!encoderVolts->getAccelerationEnabled());
        dbSerialPrint("  Acceleration is ");
        dbSerialPrintln((encoderVolts->getAccelerationEnabled()) ? "enabled" : "disabled");

        break;
    }
  }
  ClickEncoder::Button encBtnAmps = encoderAmps->getButton();
  if (encBtnAmps != ClickEncoder::Open)
  {
    dbSerialPrint("encBtnAmps: ");
    switch (encBtnAmps) {
        VERBOSECASE(ClickEncoder::Pressed);
        VERBOSECASE(ClickEncoder::Held)
        VERBOSECASE(ClickEncoder::Released)
        VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
        dbSerialPrintln("ClickEncoder::DoubleClicked");
        encoderAmps->setAccelerationEnabled(!encoderAmps->getAccelerationEnabled());
        dbSerialPrint("  Acceleration is ");
        dbSerialPrintln((encoderAmps->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    }
  }

}

/*************************************************
**************************************************
**
**  Sensor Functions
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
void getSensors()
{
  acAmpsVal = ampsPer * analogRead(A0);
  acVoltsVal = voltsPer * analogRead(A1);
}

/*************************************************
**************************************************
**
**  NEXTION Functions
**
**************************************************
**************************************************/
void showSplash()
{
  sendCommand("keypad.picc=4");
  sendCommand("vis keypad,1");
  delay(4000);
  sendCommand("vis keypad,0");
}

void updateDisplays()
{
  // Update Set Point Values
  updateSetpoints();
  // Update Actual Values
  ftoa(tempbuffer, acAmpsVal, 2);
  acAmps.setText(tempbuffer);
  ftoa(tempbuffer, acVoltsVal, 2);
  acVolts.setText(tempbuffer);
  // Update power
  ftoa(tempbuffer, acWattsVal, 2);
  acPower.setText(tempbuffer);
  acWattsVal = acAmpsVal * acVoltsVal;
  acWatthrVal += acWattsVal / 3600.0;
  ftoa(tempbuffer, acWatthrVal, 2);
  acWattHr.setText(tempbuffer);
  // Update Temperature
  ftoa(tempbuffer, temp_C, 2);
  tTemperature.setText(tempbuffer);
}

void updateSetpoints()
{
  // Update Set Point Values
  ftoa(tempbuffer, currentVolts, 3);
  spVolts.setText(tempbuffer);
  ftoa(tempbuffer, currentAmps, 3);
  spAmps.setText(tempbuffer);
}

void updateTrendGraph()
{
  if (bgraphVolts) wTrend.addValue(0, analogRead(A0) >> 2);
  if (bgraphAmps) wTrend.addValue(1, analogRead(A1 >> 2));
  if (bgraphWatts)wTrend.addValue(2, (analogRead(A0 >> 2)*analogRead(A1 >> 2)) >> 8);
  if (bgraphTemp) wTrend.addValue(3, (int)temp_C * 5);
}

// This is slow
// A fix could be to use 115200 Baud
void hideKeypad()
{
  sendCommand("vis 255,0");
  sendCommand("vis acVolts,1");
  sendCommand("vis acAmps,1");
  sendCommand("vis acPower,1");
  sendCommand("vis acTemp,1");
  sendCommand("vis trend,1");
  sendCommand("vis OnOff,1");
  sendCommand("vis clrWattHr,1");
  sendCommand("vis constVolt,1");
  sendCommand("vis constAmps,1");
  sendCommand("vis setAmps,1");
  sendCommand("vis setVolts,1");
  sendCommand("vis spVolts,1");
  sendCommand("vis spAmps,1");
  sendCommand("vis acWattHr,1");
  sendCommand("vis graphVolts,1");
  sendCommand("vis graphAmps,1");
  sendCommand("vis graphPower,1");
  sendCommand("vis graphTemp,1");
}
// This is slow
// A fix could be to use 115200 Baud
void showKeypad()
{
  sendCommand("vis 255,0");
  sendCommand("keypad.picc=2");
  sendCommand("vis keypad,1");
  sendCommand("vis cancel,1");
  sendCommand("vis save,1");
  sendCommand("vis b0,1");
  sendCommand("vis b1,1");
  sendCommand("vis b2,1");
  sendCommand("vis b3,1");
  sendCommand("vis b4,1");
  sendCommand("vis b5,1");
  sendCommand("vis b6,1");
  sendCommand("vis b7,1");
  sendCommand("vis b8,1");
  sendCommand("vis b9,1");
  sendCommand("vis b10,1");
  sendCommand("vis b11,1");
  sendCommand("vis msg,1");
  sendCommand("vis propDesc,1");
  sendCommand("vis propText,1");
}

/*
   Call back init and functions.
*/
void initNextionCallBacks()
{
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
}
void setAmpsCallback(void *ptr)
{
  setVoltsKeypad = false;
  showKeypad();
}
void setVoltsCallback(void *ptr)
{
  setVoltsKeypad = true;
  showKeypad();
}

void bCancelCallback(void *ptr)
{
  hideKeypad();
}
void bSaveCallback(void *ptr)
{
  bool isValidNum = false;
  bool isOORNum = true;
  double number;
  char temp1[18] = {0};
  propText.getText(temp1, 18);
  if (isOnlyDouble(temp1))
  {
    number = atof(temp1);
    dbSerialPrint("Value="); dbSerialPrintln(number);
    isValidNum = true;
  }
  else
  {
    dbSerialPrint("Invalid Value="); dbSerialPrintln(temp1);
  }
  // There is a potential bug in that I have 10 Characters reserved
  // in the text box but only 8 are returned from GetText call so i
  if (isValidNum)
  {
    if (setVoltsKeypad == true)
    {
      if (number > MAXVolts)
      {
        errorMSG.setText("Volts out of Range, (0-30V)");
        dbSerialPrintln("Volts out of Range");
      }
      else
      {
        isOORNum = false;
        currentVolts = number;
      }
    }
    else
    {
      if (number > MAXAmps)
      {
        errorMSG.setText("Amps out of Range, (0-5A)");
        dbSerialPrintln("Amps out of Range");
      }
      else
      {
        isOORNum = false;
        currentAmps = number;
      }
    }
  }
  if (!isOORNum)
  {
    hideKeypad();
    updateSetpoints();
  }
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
bool isOnlyDouble(const char* str)
{
  char* endptr = 0;
  strtod(str, &endptr);
  return !(*endptr != '\0' || endptr == str);
}
