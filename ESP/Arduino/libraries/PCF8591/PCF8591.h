/*************************************************** 
  This is a library for the SHT31 Digital Humidity & Temp Sensor

  Designed specifically to work with the SHT31 Digital sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These sensors use I2C to communicate, 2 pins are required to interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif
#include "Wire.h"



class PCF8591 {
 public:
  PCF8591(int8_t sdapin,int8_t sclpin);
  boolean begin(uint8_t i2caddr);
  boolean synchronize(void);
  byte A0(void);
  byte A1(void);
  byte A2(void);
  byte A3(void);
 
 private:
  bool _ReadExecuted;
  int8_t _sda, _scl;
  uint8_t _i2caddr;
  byte _a0,_a1,_a2,_a3;
 };

