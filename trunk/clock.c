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

uint8_t StrToMonth(PGM_P str)
{
	for (uint8_t i=0; i<12; i++)
	{
		PGM_P pMonthName = (PGM_P)pgm_read_word(&monthNames[i]);
		uint8_t j;
		for (j=0; j<3; j++)
		{
			uint8_t c1 = pgm_read_byte(&str[j]);
			uint8_t c2 = pgm_read_byte(&pMonthName[j]);
			if (toupper(c1) != toupper(c2))
				break;
		}
		
		if (j == 3)
			return i+1;
	}	
	
	return 0;
}	

char* MakeDateString(char* str, uint8_t day, uint8_t month)
{
	strcpy_P(str, (PGM_P)pgm_read_word(&monthNames[month-1]));
	
	if (day < 10)
	{
		strcat_P(str, PSTR(" 0"));
	}
	else
	{
		strcat_P(str, PSTR(" "));
	}
	itoa(day, &str[strlen(str)], 10);
	return str;			
}

char* MakeShortTimeString(char* str, uint8_t hour, uint8_t minute)
{
	*str = 0;
	
	uint8_t h = (hour == 0 || hour == 12) ? 12 : hour % 12;
	if (h < 10)
	{
		strcat_P(str, PSTR("0"));
	}
	itoa(h, &str[strlen(str)], 10);
	
	strcat_P(str, PSTR(":"));
	if (minute < 10)
	{
		strcat_P(str, PSTR("0"));
	}
	itoa(minute, &str[strlen(str)], 10);
	
	strcat_P(str, hour < 12 ? PSTR("A") : PSTR("P"));
	
	return str;			
}	

char* MakeTimeString(char* str, uint8_t hour, uint8_t minute, uint8_t second)
{
	*str = 0;
	
	uint8_t h = (hour == 0 || hour == 12) ? 12 : hour % 12;
	if (h < 10)
	{
		strcat_P(str, PSTR("0"));
	}
	itoa(h, &str[strlen(str)], 10);
	
	strcat_P(str, PSTR(":"));
	if (minute < 10)
	{
		strcat_P(str, PSTR("0"));
	}
	itoa(minute, &str[strlen(str)], 10);
	
	strcat_P(str, PSTR(":"));
	if (second < 10)
	{
		strcat_P(str, PSTR("0"));
	}
	itoa(second, &str[strlen(str)], 10);
	
	strcat_P(str, hour < 12 ? PSTR("A") : PSTR("P"));

	return str;			
}

char* MakeApproxTimeString(char* str, uint16_t minutes)
{
	// result must be 5 chars or less
	if (minutes < 60)
	{
		itoa(minutes, str, 10);
		strcat_P(str, PSTR("m"));
	}		
	else if (minutes == 84)
	{
		strcpy_P(str, PSTR("90m")); // actually 84m
	}
	else if (minutes == 128)
	{
		strcpy_P(str, PSTR("2h")); // actually 128m
	}
	else if (minutes == 18*84)
	{
		strcpy_P(str, PSTR("24h")); // actually 25h12m
	}
	else if (minutes == 30*84)
	{
		strcpy_P(str, PSTR("2d")); // actually 1.75d
	}
	else if (minutes == 60*84)
	{
		strcpy_P(str, PSTR("4d")); // actually 3.5d
	}
	else if (minutes <= 36*60)
	{
		itoa(minutes/60, str, 10);
		strcat_P(str, PSTR("h"));
	}
	else
	{
		itoa(minutes/(60*24), str, 10);
		strcat_P(str, PSTR("d"));
	}
	
	return str;
}

char* MakePastTimeString(char* str, uint16_t timeAgo)
{
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
	
	uint8_t februaryLength = (pastYear % 4 == 0) ? 29 : 28;
		
	if (pastDay==0 ) // pastDay < 0
	{
		pastMonth--;
			
		if (pastMonth == 0)
		{
			pastMonth = 12;
			pastYear--;
		}
			
		if (pastMonth == 2)
			pastDay = februaryLength;
		else if (pastMonth == 1 || pastMonth == 3 || pastMonth == 5 || pastMonth == 7 || pastMonth == 8 || pastMonth == 10 || pastMonth == 12)
			pastDay = 31;
		else 
			pastDay = 30;
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
			
			if (pastMonth == 2)
				pastDay = februaryLength;
			else if (pastMonth == 1 || pastMonth == 3 || pastMonth == 5 || pastMonth == 7 || pastMonth == 8 || pastMonth == 10 || pastMonth == 12)
				pastDay = 31;
			else 
				pastDay = 30;
		}
	}	
	
	char dateStr[7];
	char timeStr[9];
	
	MakeDateString(dateStr, pastDay, pastMonth);
	MakeShortTimeString(timeStr, pastHour, pastMinute);
	strcpy(str, dateStr);
	strcat_P(str, PSTR(" "));
	strcat(str, timeStr);
	
	return str;
}

/*uint8_t Str2ToByte(PGM_P p)
{
	uint8_t tensByte = pgm_read_byte(p);
	uint8_t tens = tensByte == ' ' ? 0 : tensByte - '0';
	uint8_t ones = pgm_read_byte(p+1) - '0';
	
	return tens*10 + ones;	
}	
*/
	
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
	
	if ((clock_elapsedQuarterSeconds & 0x3) == 0)
	{
		clock_second++;
		
		if (clock_second == 60)
		{
			clock_second = 0;
			clock_minute++;
			
			if (clock_minute == 60)
			{
				clock_minute = 0;
				clock_hour++;
				
				if (clock_hour == 24)
				{
					clock_hour = 0;
					clock_day++;
					
					uint8_t februaryLength = (clock_year % 4 == 0) ? 29 : 28;
					
					if ((clock_month == 2 && clock_day == februaryLength+1) ||
						((clock_month == 4 || clock_month == 6 || clock_month == 9 || clock_month == 11) && clock_day == 31) ||
						(clock_day == 32)
						)
					{
						clock_day = 1;
						clock_month++;
						
						if (clock_month == 13)
						{
							// happy new year!
							clock_month = 1;
							clock_year++;
						}	
					}
				}
			}
		}
	}
}

