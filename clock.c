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

#include <util/atomic.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "clock.h"
#include "hikea.h"

volatile uint8_t clock_second ;
volatile uint8_t clock_minute ;
volatile uint8_t clock_hour ;
volatile uint8_t clock_day;
volatile uint8_t clock_month;
volatile uint8_t clock_year; // year - 2000

volatile uint32_t clock_elapsedQuarterSeconds;

const char month1[] PROGMEM = "Jan";
const char month2[] PROGMEM = "Feb";
const char month3[] PROGMEM = "Mar";
const char month4[] PROGMEM = "Apr";
const char month5[] PROGMEM = "May";
const char month6[] PROGMEM = "Jun";
const char month7[] PROGMEM = "Jul";
const char month8[] PROGMEM = "Aug";
const char month9[] PROGMEM = "Sep";
const char month10[] PROGMEM = "Oct";
const char month11[] PROGMEM = "Nov";
const char month12[] PROGMEM = "Dec";

const char* monthNames[] PROGMEM = { 
	month1, month2, month3, month4, month5, month6, month7, month8, month9, month10, month11, month12 
};

uint8_t monthLen[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// example: 01	
void AppendTwoDigitNumber(char* str, uint8_t val)
{
	str = &str[strlen(str)];
	
	if (val < 10)
	{
		*str = '0';
		itoa(val, str+1, 10);
	}
	else
	{
		itoa(val, str, 10);
	}		
}	

// example: Jan 01
char* MakeDateString(char* str, uint8_t day, uint8_t month)
{
	strcpy_P(str, (PGM_P)pgm_read_word(&monthNames[month-1]));
	strcat_P(str, PSTR(" "));
	AppendTwoDigitNumber(str, day);
	return str;			
}

// example: 12:34P
char* MakeShortTimeString(char* str, uint8_t hour, uint8_t minute)
{
	*str = 0;
	
	uint8_t h = (hour == 0 || hour == 12) ? 12 : hour % 12;
	AppendTwoDigitNumber(str, h);
	
	strcat_P(str, PSTR(":"));
	AppendTwoDigitNumber(str, minute);
	
	strcat_P(str, hour < 12 ? PSTR("A") : PSTR("P"));
	
	return str;			
}	

// example: 12:34:56P
char* MakeTimeString(char* str, uint8_t hour, uint8_t minute, uint8_t second)
{
	*str = 0;
	
	uint8_t h = (hour == 0 || hour == 12) ? 12 : hour % 12;
	AppendTwoDigitNumber(str, h);
	
	strcat_P(str, PSTR(":"));
	AppendTwoDigitNumber(str, minute);
	
	strcat_P(str, PSTR(":"));
	AppendTwoDigitNumber(str, second);
	
	strcat_P(str, hour < 12 ? PSTR("A") : PSTR("P"));

	return str;			
}
		
char* MakePastTimeString(char* str, uint16_t timeAgo)
{
	// timeAgo must be less than 11820 (197 hours or about 8 days) to prevent overflow errors
	uint8_t pastMinute;
	uint8_t pastHour;
	uint8_t pastDay;
	uint8_t pastMonth;	
	uint8_t pastYear;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		pastMinute = clock_minute;
		pastHour = clock_hour;
		pastDay = clock_day;
		pastMonth = clock_month;
		pastYear = clock_year;	
	}

	pastMinute -= timeAgo % 60;
	if (pastMinute > 59) // pastMinute < 0
	{
		pastHour--;
		pastMinute += 60;
	}
	
	timeAgo /= 60;
	pastHour -= timeAgo % 24;
	if (pastHour > 23) // pastHour < 0
	{
		pastDay--;
		pastHour += 24;
	} 

	// determine February length
	monthLen[2] = ((pastYear % 4) == 0) ? 29 : 28;
		
	if (pastDay==0 ) // pastDay < 0
	{
		pastMonth--;
			
		if (pastMonth == 0)
		{
			pastMonth = 12;
			pastYear--;
		}
			
		pastDay = monthLen[pastMonth];
	}
	
	timeAgo /= 24;
	while (timeAgo)
	{
		timeAgo--;
		pastDay--;
		
		if (pastDay==0 || pastDay > 31) // pastDay < 0
		{
			pastMonth--;
			
			if (pastMonth == 0)
			{
				pastMonth = 12;
				pastYear--;
			}
			
			pastDay = monthLen[pastMonth];
		}
	}	
	
	MakeDateString(str, pastDay, pastMonth);
	strcat_P(str, PSTR(" "));
	MakeShortTimeString(&str[strlen(str)], pastHour, pastMinute);
	return str;
}
	
void ClockInit()
{
	clock_year = 0;
	clock_month = 1;
	clock_day = 1;
	clock_hour = 0;
	clock_minute = 0;
	clock_second = 0;	
}	
				
void ClockTick()
{
	clock_elapsedQuarterSeconds++;
	if ((clock_elapsedQuarterSeconds & 0x3) != 0)
		return;	

	clock_second++;	
	if (clock_second != 60)
		return;
		
	clock_second = 0;
	clock_minute++;
	if (clock_minute != 60)
		return;
		
	clock_minute = 0;
	clock_hour++;
	if (clock_hour != 24)
		return;
		
	clock_hour = 0;
	clock_day++;
	// determine February length
	monthLen[2] = ((clock_year % 4) == 0) ? 29 : 28;
	if (clock_day != monthLen[clock_month]+1)
		return;
		
	clock_day = 1;
	clock_month++;
	if (clock_month != 13)
		return;
		
	// happy new year!
	clock_month = 1;
	clock_year++;
}

