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

#ifndef BMP085_H_
#define BMP085_H_

extern volatile float expectedSeaLevelPressure;
extern volatile uint8_t bmpError;

char bmp085Read(unsigned char address);
int bmp085ReadInt(unsigned char address);
void bmp085Reset();
void bmp085Init();
unsigned int bmp085ReadUT();
unsigned long bmp085ReadUP();
short bmp085ConvertTemperature(unsigned int ut);
long bmp085ConvertPressure(unsigned long up);
float bmp085GetAltitude(float pressure);
float bmp085GetSeaLevelPressure(float stationPressure, float trueAltitude);

#endif /* BMP085_H_ */