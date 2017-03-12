
#include <stdio.h>
#include <Wire.h>

#define bq27510CMD_CNTL_LSB  0x00
#define bq27510CMD_CNTL_MSB  0x01
#define bq27510CMD_AR_LSB    0x02
#define bq27510CMD_AR_MSB    0x03
#define bq27510CMD_ARTTE_LSB 0x04
#define bq27510CMD_ARTTE_MSB 0x05
#define bq27510CMD_TEMP_LSB  0x06
#define bq27510CMD_TEMP_MSB  0x07
#define bq27510CMD_VOLT_LSB  0x08
#define bq27510CMD_VOLT_MSB  0x09
#define bq27510CMD_FLAGS_LSB 0x0A
#define bq27510CMD_FLAGS_MSB 0x0B
#define bq27510CMD_NAC_LSB   0x0C
#define bq27510CMD_NAC_MSB   0x0D
#define bq27510CMD_FAC_LSB   0x0E
#define bq27510CMD_FAC_MSB   0x0F
#define bq27510CMD_RM_LSB    0x10
#define bq27510CMD_RM_MSB    0x11
#define bq27510CMD_FCC_LSB   0x12
#define bq27510CMD_FCC_MSB   0x13
#define bq27510CMD_AI_LSB    0x14
#define bq27510CMD_AI_MSB    0x15
#define bq27510CMD_TTE_LSB   0x16
#define bq27510CMD_TTE_MSB   0x17
#define bq27510CMD_TTF_LSB   0x18
#define bq27510CMD_TTF_MSB   0x19
#define bq27510CMD_SI_LSB    0x18
#define bq27510CMD_SI_MSB    0x19
#define bq27510CMD_STTE_LSB  0x1A
#define bq27510CMD_STTE_MSB  0x1B
#define bq27510CMD_SOH_LSB   0x1C
#define bq27510CMD_SOH_MSB   0x1D
#define bq27510CMD_CC_LSB    0x1E
#define bq27510CMD_CC_MSB    0x1F
#define bq27510CMD_SOC_LSB   0x20
#define bq27510CMD_SOC_MSB   0x21
#define bq27510CMD_INSC_LSB  0x22
#define bq27510CMD_INSC_MSB  0x23
#define bq27510CMD_ITLT_LSB  0x28
#define bq27510CMD_ITLT_MSB  0x29
#define bq27510CMD_RS_LSB    0x2A
#define bq27510CMD_RS_MSB    0x2B
#define bq27510CMD_OPC_LSB   0x2C
#define bq27510CMD_OPC_MSB   0x2D
#define bq27510CMD_DCAP_LSB  0x2E
#define bq27510CMD_DCAP_MSB  0x2F

#define bq27510_ADR      0x55


void setup()
{
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  delay(500);
  Serial.println("Temp, Volts, mA, SOC, RCAP, DCAP, TTE, TTF");

}
  int Volts = 0;
  int  Amps = 0;
  int  Temp = 0;
  int SOC = 0; 
  int DCAP = 0;
  int RCAP = 0;
  unsigned int TTE = 0;
  unsigned int TTF = 0;

void loop()
{
  
Temp  = getValue(bq27510_ADR, bq27510CMD_TEMP_LSB )/ 10 - 273;
Volts = getValue(bq27510_ADR, bq27510CMD_VOLT_LSB );
Amps = getValue(bq27510_ADR, bq27510CMD_AI_LSB);
SOC = getValue(bq27510_ADR, bq27510CMD_SOC_LSB);
DCAP = getValue(bq27510_ADR, bq27510CMD_DCAP_LSB);
RCAP = getValue(bq27510_ADR, bq27510CMD_RM_LSB);
TTE = getValue(bq27510_ADR, bq27510CMD_TTE_LSB);
TTF = getValue(bq27510_ADR, bq27510CMD_TTF_LSB);
char output[80];
sprintf(output, "%2d, %u , %4d , %2d, %d, %d, %u, %u", Temp ,Volts ,Amps ,SOC, RCAP, DCAP, TTE, TTF);
  Serial.println(output);
  delay(5000);
}


int getValue(int port, int cmd)
{
  int tmp1=0;
  int tmp2 = 0;
  int response = 0;
  Wire.beginTransmission(port);
  Wire.write(byte(cmd));
  Wire.endTransmission(true);
  // according to all the demos, first reading is unstable so ignore it ??
  // Seems yo have to do this twice to get it to work, if you dont is gets screwed up
  Wire.beginTransmission(port);
  Wire.write(byte(cmd));
  // request 2 bytes from slave device port
  response = Wire.requestFrom(port, 2);   
  Wire.endTransmission(true);

  while(Wire.available())    // slave may send less than requested
  { 
    tmp1 = Wire.read() ;
	tmp2 = Wire.read() ;
  }
  return transBytes2Int(tmp2, tmp1);
}

/**
  * @brief  Translate two bytes into an integer
  * @param  
  * @retval The calculation results
  */
unsigned int transBytes2Int(unsigned char msb, unsigned char lsb)
{
    unsigned int tmp;
    tmp = ((msb << 8) & 0xFF00);
    return ((unsigned int)(tmp + lsb) & 0x0000FFFF);
}

