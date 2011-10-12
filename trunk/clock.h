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

#ifndef CLOCK_H_
#define CLOCK_H_

#include <inttypes.h>
#include <avr/pgmspace.h>

extern volatile uint8_t clock_second;
extern volatile uint8_t clock_minute;
extern volatile uint8_t clock_hour;
extern volatile uint8_t clock_day;
extern volatile uint8_t clock_month;
extern volatile uint8_t clock_year; // year - 2000

extern volatile uint32_t clock_elapsedQuarterSeconds;

char* MakeDateString(char* str, uint8_t day, uint8_t month);
char* MakeShortTimeString(char* str, uint8_t hour, uint8_t minute);
char* MakeTimeString(char* str, uint8_t hour, uint8_t minute, uint8_t second);
char* MakeApproxTimeString(char* str, uint16_t minutes);
char* MakePastTimeString(char* str, uint16_t timeAgo);
int StrToMonth(PGM_P str);
void ClockInit();
void ClockTick();


#endif /* CLOCK_H_ */