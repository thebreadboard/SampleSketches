
#include <SPI.h>

// define the buffer size... 
#define serialbufferSize 50 
#define commandDelimeters "|,.- "
#define analogToVolts 5/1023 // Volts per step in ADC 

// Now the real varibles
char inputBuffer[serialbufferSize]   ; 
int serialIndex = 0; // keep track of where we are in the buffer
// End of real variables

#define VREF 5.000
// Default values below assume 0-10V Unipolar or -5 to +5 bipolar output
#define DAC_VFSR VREF*2 // may be 0 - 10 or -5 to +5

//********************************
// PICK THE RIGHT BOARD FOR THE CS
//********************************
//EXP430FR5969 use P1_5 for CS
//#define DAC_8734_CS_PIN P1_5

//Arduino UNO use 9 for CS
//#define DAC_8734_CS_PIN 9

//EXP430F5529 use P2_6 for CS
//#define DAC_8734_CS_PIN P2_6

//EXP432P401R use A5 for CS
#define DAC_8734_CS_PIN A7

//TM4C1294XL use PN_2 for CS
#define DAC_8734_CS_PIN PN_2

//*******************************
//END CHIP SELECTS
//*******************************

#define DAC_8734_Command 0 // command register
#define DAC_8734_DataBase 0x04 // register offset to zero cal base register
#define DAC_8734_CalZeroBase 0x08 // register offset to zero cal base register
#define DAC_8734_CalGainBase 0x0C // register offset to gain cal base register
#define DACMAX 0xFFFF
#define DACMIN 0x0000

int DAC_Gain0 = 0; // 0 = *2, 1 = *4
int DAC_Gain1 = 0; // 0 = *2, 1 = *4
int DAC_Gain2 = 0; // 0 = *2, 1 = *4
int DAC_Gain3 = 0; // 0 = *2, 1 = *4
int DAC_GPIO0 = 1; // 1 = Group A in bipolar 0=Unipolar (External connection to control pin)
int DAC_GPIO1 = 1; // 1 = Group B in bipolar 0=Unipolar (External connection to control pin)
int DAC_PD_A = 0; // 1 = group A power down
int DAC_PD_B = 0; // 1 = group B power down
int DAC_DSDO = 0; // 1 = Disable SDO bit.

//
//// Sine table
byte val = 0;
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

int DAC_CAL_tab[8] = { 0xFF,0xfF,0x50,0x5F,   0x80,0x80,0x80,0x80}; // zero's then gain's

void setup()
{
  Serial.begin(19200);
  help();
  pinMode(DAC_8734_CS_PIN, OUTPUT);
  SPI.begin();
  InitDAC();
  CalDAC8734();

}
 
void loop()
{

  SetDAC(0,0x0ffff);
  SetDAC(1,0x0ffff);
  SetDAC(2,0x0ffff);
  SetDAC(3,0x0ffff);

while(1)
  {
    if (CheckSerial()) DoCommand(inputBuffer); 
  } // loop forever
}

void sinX(int loopcount)
{
 if (loopcount > 255) loopcount=255;
for (int y = 0 ; y< loopcount ; y++)
  {
    for (int x= 0 ; x<256 ; x++)
    {
    SetDAC(0, Sin_tab[x]);
    }
  }
  Serial.println("Done");
}


void WriteDACRegister(int reg, int val)
{
  digitalWrite(DAC_8734_CS_PIN, LOW);
  SPI.transfer(reg); // command register
  SPI.transfer(val >> 8); // set gain high etc
  SPI.transfer(val & 0xFF); // set gain low etc
  digitalWrite(DAC_8734_CS_PIN, HIGH);
}

void InitDAC()
{
  // for now, gain of 2, gpio hiZ, 
  int DAC_INIT = 0x0000 | DAC_PD_A << 12 | DAC_PD_B << 11 | DAC_GPIO1 << 9 | DAC_GPIO0 << 8 | DAC_DSDO << 7 | DAC_Gain3 << 5 | DAC_Gain2 << 4 | DAC_Gain1 << 3 | DAC_Gain0 << 2 ;
  Serial.print("Cmd Reg = "); Serial.print(DAC_INIT, HEX); Serial.print(" "); Serial.println(DAC_INIT, BIN);
  WriteDACRegister(DAC_8734_Command,DAC_INIT);
} 

void SetDAC(unsigned int channel, unsigned int value)
{
  if (channel > 3) return; // channel value too high
  WriteDACRegister(DAC_8734_DataBase + channel, value);
}

void CalDAC8734()
{
  for (int x= 0 ; x<4 ; x++)
  {
    WriteDACRegister(DAC_8734_CalZeroBase + x, DAC_CAL_tab[x]) ;// Zero cal
    WriteDACRegister(DAC_8734_CalGainBase + x, DAC_CAL_tab[x+4]); // Gain cal
  }
}

/*
Checks the serial input for a string, returns true once a '\n' is seen
users can always look at the global variable "serialIndex" to see if characters have been received already
*/
boolean CheckSerial()
{
  boolean lineFound = false;
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    //Read a character as it comes in:
    //currently this will throw away anything after the buffer is full or the \n is detected
    char charBuffer = Serial.read(); 
      if (charBuffer == '\n') {
           inputBuffer[serialIndex] = 0; // terminate the string
           lineFound = (serialIndex > 0); // only good if we sent more than an empty line
           serialIndex=0; // reset for next line of data
         }
         else if(charBuffer == '\r') {
           // Just ignore the Carrage return, were only interested in new line
         }
         else if(serialIndex < serialbufferSize && lineFound == false) {
           /*Place the character in the string buffer:*/
           inputBuffer[serialIndex++] = charBuffer; // auto increment index
         }
  }// End of While
  return lineFound;
}// End of CheckSerial()

// Enhanced Command Processor using strtok to strip out command from multi parameter string
boolean DoCommand(char * commandBuffer)
{
  Serial.println("Cmd");
  char* Command; // Command Parameter
  char* Parameter; // Additional Parameter
  unsigned int analogVal = 0; // additional parameter converted to analog if possible
  

  // Get the command from the input string
  Command = strtok(commandBuffer,commandDelimeters); // get the command
  Parameter = strtok(NULL, commandDelimeters); // get the parameter if any
  //if there are more than one parameter they will be ignored for now

  // Make sure we have an analog value if we are to allow PWM output
  unsigned int outparameter = isNumeric (Parameter);

  //if it is a number then convert it
  if (outparameter) 
  {
    analogVal = atoi(Parameter);
    // check the analog value is in the correct range
    if (analogVal < DACMIN || analogVal > DACMAX) outparameter = false;
  }
  // Standard way to handle commands
  if (strcmp(Command,"help")==0){
    help();
  }  
  // now  the DAC Outputs if we have a valid analog parameter
  else if (strcmp(Command, "DAC0")==0  && outparameter ){
      Serial.print("DAC 0 = ");
      Serial.println(analogVal);
      SetDAC(0,analogVal);    // Set the DAC 0 output
    }
    // now the PWM Outputs
      else if (strcmp(Command, "DAC1")==0  && outparameter ){
      Serial.print("DAC 1 = ");
      Serial.println(analogVal);    
      SetDAC(1, analogVal);    // Set the DAC 1 output
    }
    // now the PWM Outputs
      else if (strcmp(Command, "DAC2")==0  && outparameter ){
      Serial.print("DAC 2 = ");
      Serial.println(analogVal);
      SetDAC(2, analogVal);    // Set the DAC 2 output
    }
    // now the PWM Outputs
      else if (strcmp(Command, "DAC3")==0  && outparameter ){
      Serial.print("DAC 3 = ");
      Serial.println(analogVal); 
      SetDAC(3, analogVal);    // Set the DAC 3 output
    }
    // now the PWM Outputs
      else if (strcmp(Command, "sinX")==0  && outparameter ){
      Serial.print("SineX = ");
      Serial.println(analogVal); 
      sinX(analogVal);    // Set the DAC 3 output
    }

    
    // UNIPOLAR
      else if (strcmp(Command, "UP")==0 ){
      Serial.println("Set UniPolar");
      DAC_GPIO0 = 1; DAC_GPIO1 = 1;
      InitDAC();
    }
    // BIPOLAR
      else if (strcmp(Command, "BP")==0 ){
      Serial.println("Set BiPolar");
      DAC_GPIO0 = 0; DAC_GPIO1 = 0;
      InitDAC();
    }    
    // Power Down
      else if (strcmp(Command, "OFF")==0 ){
      Serial.println("Off");
      DAC_PD_A = 1; DAC_PD_B = 1;
      InitDAC();
    }
    // Power Up
      else if (strcmp(Command, "ON")==0 ){
      Serial.println("On ");
      DAC_PD_A = 0; DAC_PD_B = 0;
      InitDAC();
    }    
  // Catch All
  else {
    Serial.print("Error You said: ");
    Serial.println(commandBuffer);
    //    Do some other work here
    //    and here
    //    and here
  }

return true;
}
void help()
{
    // Print some pretty instructions
  Serial.println("DAC8734 Test Program:");
  Serial.println("Peter Oakes, July 2015, use as you like:\n");
  Serial.println("Commands Available:");
  Serial.println("type \"help\" for this menu" );
  Serial.println("or \"DACx where x is from 0 to 3\" and a value 0 to 65535" );
  Serial.println("or \"BP to change mode to Bipolar (-5.00 to +5.00V\"" );
  Serial.println("or \"UP to change mode to Unipolar (0 to 10.00V\"" );
  Serial.println("or \"OFF to enter low power mode\"" );
  Serial.println("or \"ON to enter low power mode\"" );
  Serial.println("or \"Sine x to output a sinewave on Channel 0 for the specified cycles\"" );
  Serial.println();
  Serial.println("make sure \"NewLine\" is on in the console" );

}
int isNumeric (const char * s)
{
  while(*s)
  {
    if(!isdigit(*s)) return 0;
    s++;
  }
  return 1;
}
