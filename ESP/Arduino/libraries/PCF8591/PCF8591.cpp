
#include "PCF8591.h"

PCF8591::PCF8591(int8_t SDApin, int8_t SCLpin) 
{
	_sda=SDApin;
	_scl=SCLpin;
}

boolean PCF8591::begin(uint8_t i2caddr) {
  Wire.begin(_sda, _scl);
  _i2caddr = i2caddr;
/*   _ReadExecuted=true; 
 Wire.beginTransmission(_i2caddr); // wake up PCF8591
  Wire.write(0x02); // control byte: reads ADC0 then auto-increment
  Wire.endTransmission(); // end t
  _ReadExecuted=false; */
  return true;
}

boolean PCF8591::synchronize(void) 
{
  if (_ReadExecuted==false)
  {
   _ReadExecuted=true;
    Wire.requestFrom(_i2caddr, (uint8_t)1);
	delay(1000);
  while (Wire.available())   // slave may send less than requested
  {
    //byte _bagger = Wire.read(); // throw this one away
   // delay(1);
	_a0 = Wire.read();
	//delay(10);
   // _a1 = Wire.read();
	//delay(10);
   // _a2 = Wire.read();
	//delay(10);
  //  _a3 = Wire.read();
	//delay(10);
	
	//Serial.println("_bagger");
	//Serial.println(_bagger);
	Serial.println("_a0");
	Serial.println(_a0);
	Serial.println("_a1");
	Serial.println(_a1);
	Serial.println("_a2");
	Serial.println(_a2);
	Serial.println("_a3");
	Serial.println(_a3);
	
	_ReadExecuted=false;
	return true;
  }
   _ReadExecuted=false;
  }
  return false;
  }

byte PCF8591::A0(void) 
{
   Wire.beginTransmission(_i2caddr); // wake up PCF8591
  Wire.write(0x00); // control byte: reads ADC0 then auto-increment
  Wire.endTransmission(); // end t
   Wire.requestFrom(_i2caddr, (uint8_t)1);
  return Wire.read();;
}
byte PCF8591::A1(void) 
{
     Wire.beginTransmission(_i2caddr); // wake up PCF8591
  Wire.write(0x01); // control byte: reads ADC0 then auto-increment
  Wire.endTransmission(); // end t
   Wire.requestFrom(_i2caddr, (uint8_t)1);
    return Wire.read();
}
byte PCF8591::A2(void) 
{
     Wire.beginTransmission(_i2caddr); // wake up PCF8591
  Wire.write(0x02); // control byte: reads ADC0 then auto-increment
  Wire.endTransmission(); // end t
   Wire.requestFrom(_i2caddr, (uint8_t)1);
    return Wire.read();
}
byte PCF8591::A3(void) 
{
    Wire.beginTransmission(_i2caddr); // wake up PCF8591
  Wire.write(0x03); // control byte: reads ADC0 then auto-increment
  Wire.endTransmission(); // end t
   Wire.requestFrom(_i2caddr, (uint8_t)1);
    return Wire.read();
}



/*********************************************************************/
