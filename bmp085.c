/* 
  Copyright (c) 2011 Steve Chamberlin
  Permission is hereby granted, free of charge, to any person obtaining a copy of this hardware, software, and associated documentation 
  files (the "Product"), to deal in the Product without restriction, including without limitation the rights to use, copy, modify, merge, 
  publish, distribute, sublicense, and/or sell copies of the Product, and to permit persons to whom the Product is furnished to do so, 
  subject to the following conditions: 

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Product. 

  THE PRODUCT IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
  ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
  THE PRODUCT OR THE USE OR OTHER DEALINGS IN THE PRODUCT.
*/

// Bosch BMP085 pressure/temperature sensor

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <math.h>

#include "i2c.h"
#include "bmp085.h"

#define BMP085_PIN_XCLR PC3

#define BMP085_ADDRESS 0xEE  // I2C address of BMP085

// Oversampling Setting
#define OSS 3

// expected pressure at sea level: average is 101325.0f
volatile float expectedSeaLevelPressure;

// Calibration values
uint16_t calibrationData[11];
#define ac1 ((int16_t)calibrationData[0])
#define ac2 ((int16_t)calibrationData[1])
#define ac3 ((int16_t)calibrationData[2])
#define ac4 calibrationData[3]
#define ac5 calibrationData[4]
#define ac6 calibrationData[5]
#define b1 ((int16_t)calibrationData[6]) 
#define b2 ((int16_t)calibrationData[7])
#define mb ((int16_t)calibrationData[8])
#define mc ((int16_t)calibrationData[9])
#define md ((int16_t)calibrationData[10])

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 

void bmp085Reset()
{
	expectedSeaLevelPressure = 101325.0f;
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
//
// Returns non-zero value if initialization was successful, 0 if initialization failed.
uint8_t bmp085Init()
{
  DDRC |= (1<<BMP085_PIN_XCLR);
	
  PORTC &= ~(1<<BMP085_PIN_XCLR); 
  _delay_ms(1);
  PORTC |= (1<<BMP085_PIN_XCLR);
  _delay_ms(10);
  		
  bmp085Reset();
  
  i2cInit();
  i2cSetAddress(BMP085_ADDRESS);
		
  // read 11 2-byte calibration values beginning at address 0xAA		
  if (i2cReadBytes(0xAA, 22, (uint8_t*)calibrationData))
  {
	  // byte swap the values
	  uint8_t count = 0;
	  for (uint8_t* pWord=(uint8_t*)calibrationData; count<11; pWord+=2, count++)
	  {
		  uint8_t temp = *pWord;
		  *pWord = *(pWord+1);
		  *(pWord+1) = temp;
	  }			 
	  return 1;
  }
  else
  {
	  return 0;
  }  
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT()
{
  unsigned int ut;
  
  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  i2cWriteByte(0xF4, 0x2E);
  
  // Wait at least 4.5ms
  _delay_ms(5);
  
  // Read two bytes from registers 0xF6 and 0xF7
  // Return the value read if i2c was successful, else return 0
  uint8_t result[2];
  if (i2cReadBytes(0xF6, 2, result))
  {
	ut = (((uint16_t) result[0] << 8) | result[1]);
	return ut;
  }
  else
  {
	return 0;
  }	
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP()
{
  unsigned long up = 0;
  
  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  i2cWriteByte(0xF4, 0x34 + (OSS<<6));
  
  // Wait for conversion, delay time dependent on OSS
  _delay_ms(2 + (3<<OSS));
  
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  // Return the value read if i2c was successful, else return 0
  uint8_t result[3];
  if (i2cReadBytes(0xF6, 3, result))
  {
	up = (((unsigned long) result[0] << 16) | ((unsigned long) result[1] << 8) | (unsigned long) result[2]) >> (8-OSS);
	return up;
  }
  else
  {
	return 0;
  }	
}

// Calculate temperature given raw ut.
// Value returned will be in units of 0.1 deg C
short bmp085ConvertTemperature(unsigned int ut)
{
  long x1, x2;
  
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return ((b5 + 8)>>4);  
}

// Calculate pressure given raw up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa (hundredths of a millibar)
long bmp085ConvertPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;
  
  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = ((((int32_t)ac1 * 4 + x3) << OSS) + 2) >> 2;

  
  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
  
  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
    
  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  
  return p;
}

float bmp085GetAltitude(float pressure)
{
	// return the altitude in meters that corresponds to the given pressure in hundredths of a millibar
	return (float)44330 * (1 - pow(((float) pressure/expectedSeaLevelPressure), 0.190295));
}

float bmp085GetSeaLevelPressure(float stationPressure, float trueAltitude)
{
	// return the pressure in hundredths of a millibar that corresponds to the station pressure in hundredths of a millibar and true altitude in meters
	return stationPressure / pow((1-trueAltitude/44330), 5.255);
}
