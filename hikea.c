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

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "hikea.h"
#include "avrsensors.h"

#ifdef NOKIA_LCD
#include "noklcd.h"
#endif
#ifdef SSD1306_LCD
#include "ssd1306.h"
#include "logo.h"
#endif

#ifdef SHAKE_SENSOR
#include "shake.h"
#endif

#include "bmp085.h"
#include "avrsensors.h"
#include "sampling.h"
#include "clock.h"
#include "speaker.h"
#include "serial.h"

// Atmel's calib_32kHz.c file has no header
void PerformCalibration(void);

#define BUTTON_NEXT PB0
#define BUTTON_SELECT PB1
#define BUTTON_PREV PB2

uint8_t sleepDelay;

#define UINT_MOD(value, modulus) ((uint8_t)(value) > 0xF0 ? (value) + (modulus) : (uint8_t)(value) >= (modulus) ? (value) - (modulus) : (value))

enum {
	MODE_CURRENT_DATA = 0,
	MODE_ALTITUDE_INFO,
	MODE_ALTITUDE_GRAPH,
	MODE_TEMP_INFO,
	MODE_TEMP_GRAPH,
	MODE_PRESSURE_INFO,
	MODE_PRESSURE_GRAPH,
	MODE_SNAPSHOTS,
	MODE_SYSTEM,
	MODE_TOP_LEVEL_COUNT,
	MODE_DATA_TYPE = MODE_TOP_LEVEL_COUNT,
	MODE_GRAPH,
	MODE_SET_TIME,
	MODE_ENTER_NUMBER,
	MODE_CHOICE
};

volatile uint8_t newSampleNeeded = 1;
volatile uint8_t screenUpdateNeeded = 1;
volatile uint8_t screenClearNeeded = 0;
volatile uint8_t graphClearNeeded = 0;
volatile uint8_t snapshotNeeded = 0;
volatile uint8_t lcdResetNeeded = 0;

volatile uint8_t mode = MODE_CURRENT_DATA;
volatile uint8_t submode = 0;
volatile uint8_t menuLevel = 0;
volatile uint8_t selectedMenuItemIndex = 0;
volatile uint8_t parentSelectedMenuItemIndex = 0;
volatile uint8_t subMenuItemIndex = 0;
volatile uint8_t topMenuItemIndex = 0;
volatile uint8_t exploringGraph = 0;
volatile uint8_t graphCursor = 0;
volatile uint8_t hibernating = 0;
volatile uint8_t unlockState = 0;

// main screen

// system screen	
const char dateStr[] PROGMEM = __DATE__;
const char timeStr[] PROGMEM = __TIME__;

volatile uint8_t setting_second;
volatile uint8_t setting_minute;
volatile uint8_t setting_hour;
volatile uint8_t setting_day;
volatile uint8_t setting_month;
volatile uint8_t setting_year; // year - 2000

// altitude
volatile int16_t altitudeDestination;

// main screen menu
const char exitMenu[] PROGMEM = "Exit Menu";

enum {
	MENU_MAIN_EXIT_MENU = 0,
	MENU_MAIN_SETUP1,
	MENU_MAIN_SETUP2,
	MENU_MAIN_SETUP3,
	MENU_MAIN_SETUP4,
	MENU_MAIN_SETUP5,
	MENU_MAIN_SETUP6
};

const char main1[] PROGMEM = "Line 1 Data";
const char main2[] PROGMEM = "Line 2 Data";
const char main3[] PROGMEM = "Line 3 Data";
const char main4[] PROGMEM = "Line 4 Data";
const char main5[] PROGMEM = "Line 5 Data";
const char main6[] PROGMEM = "Line 6 Data";

const char* mainMenu[] PROGMEM = { 
	exitMenu, main1, main2, main3, main4, main5, main6, NULL 
};

// altitude menu
enum {
	MENU_ALTITUDE_EXIT_MENU = 0,
	MENU_ALTITUDE_CALIBRATE,
	MENU_ALTITUDE_SET_GOAL
};

const char altitude1[] PROGMEM = "Calibrate";
const char altitude2[] PROGMEM = "Set Destination";
const char altitude2a[] PROGMEM = "Reset Destination";

const char* altitudeMenu[] PROGMEM = { 
	exitMenu, altitude1, altitude2, NULL 
};

// system menu
enum {
	MENU_SYSTEM_EXIT_MENU = 0,
	MENU_SYSTEM_DATE_TIME,
	MENU_SYSTEM_SLEEP_DELAY,
	MENU_SYSTEM_SOUND,
	MENU_SYSTEM_LCD_CONTRAST,
	MENU_SYSTEM_LCD_BIAS,
	MENU_SYSTEM_LCD_TEMP_COEF,
	MENU_SYSTEM_RESTORE_DEFAULTS,
	MENU_SYSTEM_ERASE_ALL_GRAPHS,
	MENU_SYSTEM_COUNT
};

const char system1[] PROGMEM = "Date and Time";
const char system2[] PROGMEM = "Sleep Delay";
const char system3[] PROGMEM = "Sound";
const char system4[] PROGMEM = "LCD Contrast";
const char system5[] PROGMEM = "LCD Bias";
const char system6[] PROGMEM = "LCD Temp Coef";
const char system7[] PROGMEM = "Restore Defaults";
const char system8[] PROGMEM = "Erase All Data";

const char* systemMenu[] PROGMEM = { 
	exitMenu, system1, system2, system3, system4, system5, system6, system7, system8, NULL 
};

// pre-baked version number string
char versionStr[11];

// data type menu

enum {
	MENU_DATA_EXIT_MENU = 0,
	MENU_DATA_TEMP,
	MENU_DATA_ALTITUDE,
	MENU_DATA_RATE_OF_ASCENT,
	MENU_DATA_TIME_TO_DESTINATION,
	MENU_DATA_PRESSURE_STATION,
	MENU_DATA_PRESSURE_SEALEVEL,
	MENU_DATA_TIME_OF_DAY,
	MENU_DATA_DATE_AND_TIME,
	MENU_DATA_TEMP_TREND,
	MENU_DATA_PRESSURE_TREND,
	MENU_DATA_FORECAST,
#ifdef SHAKE_SENSOR
	MENU_DATA_TRIP_TIME,
#endif	
	MENU_DATA_EMPTY
};

const char data1[] PROGMEM = "Temperature";
const char data2[] PROGMEM = "Altitude";
const char data3[] PROGMEM = "Rate of Ascent";
const char data4[] PROGMEM = "Time to Destination";
const char data5[] PROGMEM = "Station Pressure";
const char data6[] PROGMEM = "Sea Level Pressure";
const char data7[] PROGMEM = "Time of Day";
const char data8[] PROGMEM = "Date and Time";
const char data9[] PROGMEM = "Temperature Trend";
const char data10[] PROGMEM = "Pressure Trend";
const char data11[] PROGMEM = "Weather Forecast";
const char data12[] PROGMEM = "Empty";
#ifdef SHAKE_SENSOR
const char data13[] PROGMEM = "Trip Time";
#endif

const char* dataMenu[] PROGMEM = { 
	exitMenu, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, 
#ifdef SHAKE_SENSOR
	data13,
#endif	
	data12, NULL 
};

volatile uint8_t mainScreenDataType[6];

// snapshots menu

enum {
	MENU_SNAPSHOTS_EXIT_MENU = 0,
	MENU_SNAPSHOTS_VIEW,
	MENU_SNAPSHOTS_TAKE
};

const char snapshots1[] PROGMEM = "View Snapshot Log";
const char snapshots2[] PROGMEM = "Take Snapshot";

const char* snapshotsMenu[] PROGMEM = { 
	exitMenu, snapshots1, snapshots2, NULL 
};

// graph menu

enum {
	MENU_GRAPH_EXIT_MENU = 0,
	MENU_GRAPH_EXPLORE,
	MENU_GRAPH_TYPE,
	MENU_GRAPH_YAXIS_MIN,
	MENU_GRAPH_YAXIS_MAX
};

const char graph1[] PROGMEM = "Explore Graph";
const char graph2[] PROGMEM = "Drawing Style";
const char graph3[] PROGMEM = "Custom Y-Axis Min";
const char graph4[] PROGMEM = "Custom Y-Axis Max";
const char graph3a[] PROGMEM = "Auto Y-Axis Min";
const char graph4a[] PROGMEM = "Auto Y-Axis Max";

const char* graphMenu[] PROGMEM = {
	exitMenu, graph1, graph2, graph3, graph4, NULL
};

// rates of change
enum {
	CHANGE_SOARING = 0,
	CHANGE_RISING_QUICKLY,
	CHANGE_RISING,
	CHANGE_RISING_SLOWLY,
	CHANGE_STEADY,
	CHANGE_FALLING_SLOWLY,
	CHANGE_FALLING,
	CHANGE_FALLING_QUICKLY,
	CHANGE_PLUMMETING
};

const char change1[] PROGMEM = "Soaring";
const char change2[] PROGMEM = "Rising Fast";
const char change3[] PROGMEM = "Rising";
const char change4[] PROGMEM = "Rising Slow";
const char change5[] PROGMEM = "Steady";
const char change6[] PROGMEM = "Falling Slow";
const char change7[] PROGMEM = "Falling";
const char change8[] PROGMEM = "Falling Fast";
const char change9[] PROGMEM = "Plummeting";

const char* changeStrings[] PROGMEM = { 
	change1, change2, change3, change4, change5, change6, change7, change8, change9
};
	
const char** modeMenus[] = {
	mainMenu, altitudeMenu, 0, 0, 0, 0, 0, snapshotsMenu, systemMenu, dataMenu, graphMenu
};
	
uint8_t parentMode;	
uint8_t grandparentMode;
int16_t enterNumberMax;
int16_t enterNumberMin;
int16_t enterNumberValue;
char* enterNumberUnits;
char* enterNumberPrompt;

char* choicePrompt;
char* choice1;
char* choice2;
	
void DrawMenu()
{
#ifdef SSD1306_LCD
	const int menuLines = 7;
	const int charWidth = 6;
#endif
#ifdef NOKIA_LCD
	const int menuLines = 5;
	const int charWidth = 4;
#endif	
	const char** menu = modeMenus[mode];
	uint8_t numMenuItems = 0;
	while (pgm_read_word(&menu[numMenuItems]))
	{
		numMenuItems++;
	}
			
	for (uint8_t i=0; i<menuLines && topMenuItemIndex+i < numMenuItems; i++)
	{
		const char* pMenuItem = (PGM_P)pgm_read_word(&menu[topMenuItemIndex+i]);
		
		// special case for Set/Reset Altitude Destination menu item
		if (mode == MODE_ALTITUDE_INFO && topMenuItemIndex+i == MENU_ALTITUDE_SET_GOAL && altitudeDestination != 0x7FFF)
		{
			pMenuItem = altitude2a;
		}	
		
		// special case for Custom/Auto Y-axis menu items
		if (mode == MODE_GRAPH)
		{	
			uint8_t graphType = parentMode == MODE_ALTITUDE_GRAPH ? GRAPH_ALTITUDE : parentMode == MODE_TEMP_GRAPH ? GRAPH_TEMPERATURE : GRAPH_PRESSURE;
			
			if (topMenuItemIndex+i == MENU_GRAPH_YAXIS_MIN && graphYMin[graphType] != 0x7FFF)
			{
				pMenuItem = graph3a;
			}
			if (topMenuItemIndex+i == MENU_GRAPH_YAXIS_MAX && graphYMax[graphType] != 0x7FFF)
			{
				pMenuItem = graph4a;
			}
		}
						
		LcdGoto(0, i+1);
		for (int j=0; j<LCD_WIDTH; j++)
		{
			LcdWrite(LCD_DATA, topMenuItemIndex+i == selectedMenuItemIndex ? 0x7F : 0x00);
		}
		
		LcdGoto(4, i+1);
		char str[22];
		strncpy_P(str, pMenuItem, 21);
		LcdTinyString(str, topMenuItemIndex+i == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);
		
		if (i == 0 && topMenuItemIndex != 0)
		{
			LcdGoto(LCD_WIDTH-charWidth,i+1);
			LcdTinyString("^", topMenuItemIndex == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);
		}
	
		if (i==menuLines-1 && topMenuItemIndex+i+1 != numMenuItems)
		{
			LcdGoto(LCD_WIDTH-charWidth,i+1);
			LcdTinyString(";", topMenuItemIndex+i == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);			
		}
	}			
}

void HandleSetTimeSelect()
{
	SpeakerBeep(BEEP_ENTER);
	selectedMenuItemIndex++;
	if (selectedMenuItemIndex == 7)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			clock_second = setting_second;
			clock_minute = setting_minute;
			clock_hour = setting_hour;
			clock_day = setting_day;
			clock_month = setting_month;
			clock_year = setting_year; 
		}
		
		mode = MODE_SYSTEM;
		menuLevel = 0;
		screenClearNeeded = 1;				
	}	
}

void HandleSetTimePrevNext(uint8_t step)
{
	uint8_t februaryLength = (setting_year % 4 == 0) ? 29 : 28;
	uint8_t monthLength;
	if (setting_month == 2)
		monthLength = februaryLength;
	else if (setting_month == 4 || setting_month == 6 || setting_month == 9 || setting_month == 11)
		monthLength = 30;
	else
		monthLength = 31;
	
	uint8_t pm = setting_hour >= 12;
			
	switch (selectedMenuItemIndex)
	{
		case 0:
			setting_month += step;
			setting_month = 1+UINT_MOD(setting_month-1, 12);
			break;
		case 1:
			setting_day += step;
			setting_day = 1+UINT_MOD(setting_day-1, monthLength);
			break;	
		case 2:
			setting_year += step;
			break;
		case 3:
			setting_hour += step;
			if (pm)
				setting_hour = 12+UINT_MOD(setting_hour-12, 12);
			else
				setting_hour = UINT_MOD(setting_hour, 12);
			break;
		case 4:
			setting_minute += step;
			setting_minute = UINT_MOD(setting_minute, 60);
			break;
		case 5:
			setting_second += step;
			setting_second = UINT_MOD(setting_second, 60);
			break;
		case 6:
			if (step == 1)
				setting_hour += 12;
			else
				setting_hour -= 12;
			setting_hour = UINT_MOD(setting_hour, 24);
			break;			
	}	
}	

void StartEnterNumber(int16_t vMin, int16_t vMax, int16_t value, char* units)
{
	parentMode = mode;
	enterNumberMax = vMax;
	enterNumberMin = vMin;
	enterNumberValue = value;	
	enterNumberUnits = units;
	enterNumberPrompt = NULL;
	
	mode = MODE_ENTER_NUMBER;
} 

void HandleEnterNumberSelect()
{
	SpeakerBeep(BEEP_ENTER);
				
	if (parentMode == MODE_SYSTEM)
	{
		if (selectedMenuItemIndex == MENU_SYSTEM_SLEEP_DELAY)
		{
			sleepDelay = enterNumberValue;
		}
	}
	else if (parentMode == MODE_ALTITUDE_INFO)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_ALTITUDE_CALIBRATE:
				expectedSeaLevelPressure = bmp085GetSeaLevelPressure((float)last_pressure, 0.3048f * enterNumberValue);
				last_altitude = (3.2808399f * bmp085GetAltitude((float)last_pressure));		
				// last_altitude should now equal enterNumberValue
				last_calibration_altitude = enterNumberValue;
				break;						
				
			case MENU_ALTITUDE_SET_GOAL:
				altitudeDestination = enterNumberValue;
				break;			
		}
	}
	else if (grandparentMode == MODE_ALTITUDE_GRAPH)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_GRAPH_YAXIS_MIN:
				graphYMin[GRAPH_ALTITUDE] = enterNumberValue;
				break;
			case MENU_GRAPH_YAXIS_MAX:
				graphYMax[GRAPH_ALTITUDE] = enterNumberValue;
				break;
		}
		parentMode = grandparentMode;		
	}
	else if (grandparentMode == MODE_TEMP_GRAPH)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_GRAPH_YAXIS_MIN:
				graphYMin[GRAPH_TEMPERATURE] = enterNumberValue;
				break;
			case MENU_GRAPH_YAXIS_MAX:
				graphYMax[GRAPH_TEMPERATURE] = enterNumberValue;
				break;
		}	
		parentMode = grandparentMode;	
	}
	else if (grandparentMode == MODE_PRESSURE_GRAPH)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_GRAPH_YAXIS_MIN:
				graphYMin[GRAPH_PRESSURE] = enterNumberValue;
				break;
			case MENU_GRAPH_YAXIS_MAX:
				graphYMax[GRAPH_PRESSURE] = enterNumberValue;
				break;
		}	
		parentMode = grandparentMode;	
	}
			
	mode = parentMode;
	menuLevel = 0;
				
	screenClearNeeded = 1;	
}

void HandleEnterNumberPrevNext(uint8_t step)
{
	if (step < 0x80)
	{
		enterNumberValue += step;
	}
	else
	{
		enterNumberValue -= (0x100 - step);	
	}
	
	if (enterNumberValue < enterNumberMin)
	{
		enterNumberValue = enterNumberMin;
	}	
	if (enterNumberValue > enterNumberMax)
	{
		enterNumberValue = enterNumberMax;
	}		
	 
	if (parentMode == MODE_SYSTEM)
	{
#ifdef NOKIA_LCD
		// TODO: this could cause problems if this code is executed in an interrupt while the main code
		// is writing something else to the LCD
		if (selectedMenuItemIndex == MENU_SYSTEM_LCD_CONTRAST)
		{
			lcd_vop = 0x80 | (enterNumberValue + 27);
			LcdWrite(LCD_CMD, 0x21); // LCD Extended Commands.
			LcdWrite(LCD_CMD, lcd_vop);
			LcdWrite(LCD_CMD, 0x20); // LCD Normal Commands.
		}
		else if (selectedMenuItemIndex == MENU_SYSTEM_LCD_BIAS)
		{
			lcd_bias = 0x10 | enterNumberValue;
			LcdWrite(LCD_CMD, 0x21); // LCD Extended Commands.
			LcdWrite(LCD_CMD, lcd_bias);
			LcdWrite(LCD_CMD, 0x20); // LCD Normal Commands.
		}
		else if (selectedMenuItemIndex == MENU_SYSTEM_LCD_TEMP_COEF)
		{
			lcd_tempCoef = 0x04 | enterNumberValue;
			LcdWrite(LCD_CMD, 0x21); // LCD Extended Commands.
			LcdWrite(LCD_CMD, lcd_tempCoef);
			LcdWrite(LCD_CMD, 0x20); // LCD Normal Commands.
		}	
#endif		
#ifdef SSD1306_LCD
		if (selectedMenuItemIndex == MENU_SYSTEM_LCD_CONTRAST)
		{
			lcd_contrast = enterNumberValue;
			LcdWrite(LCD_CMD, SSD1306_SETCONTRAST);  // 0x81
			LcdWrite(LCD_CMD, lcd_contrast);  // contrast
		}
#endif	
	}
}	

void Start2Choice(char* prompt, char* c1, char* c2)
{
	parentMode = mode;
	choicePrompt = prompt;
	choice1 = c1;
	choice2 = c2;
	subMenuItemIndex = 0;
	
	mode = MODE_CHOICE;
}

void HandleChoiceSelect()
{
	SpeakerBeep(BEEP_ENTER);
	
	if (parentMode == MODE_SYSTEM)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_SYSTEM_ERASE_ALL_GRAPHS:
				if (subMenuItemIndex == 1)
				{
					graphClearNeeded = 1;
				
				}				
				break;
			
			case MENU_SYSTEM_RESTORE_DEFAULTS:
				if (subMenuItemIndex == 1)
				{
					LcdReset(); // should probably set a flag and do this in the main loop. will cause problems if reset while writing to LCD
					InitSettings();
					soundEnable = 1;
					bmp085Reset();
					last_calibration_altitude = 0;
				}				
				break;
			
			case MENU_SYSTEM_SOUND:
				soundEnable = 1 - subMenuItemIndex;
				break;
		}
	}
	else if (grandparentMode == MODE_ALTITUDE_GRAPH)
	{
		graphDrawPoints[GRAPH_ALTITUDE] = subMenuItemIndex;
		parentMode = grandparentMode;
	}	
	else if (grandparentMode == MODE_TEMP_GRAPH)
	{
		graphDrawPoints[GRAPH_TEMPERATURE] = subMenuItemIndex;
		parentMode = grandparentMode;
	}
	else if (grandparentMode == MODE_PRESSURE_GRAPH)
	{
		graphDrawPoints[GRAPH_PRESSURE] = subMenuItemIndex;
		parentMode = grandparentMode;
	}		
			
	mode = parentMode;
	menuLevel = 0;
				
	screenClearNeeded = 1;	
}	

void HandleChoicePrevNext(uint8_t step)
{
	subMenuItemIndex = 1 - subMenuItemIndex;
}	
	
void HandleMenuItem()
{
	if (mode == MODE_SYSTEM)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_SYSTEM_DATE_TIME:
				mode = MODE_SET_TIME;
				selectedMenuItemIndex = 0;
		
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					setting_second = clock_second;
					setting_minute = clock_minute;
					setting_hour = clock_hour;
					setting_day = clock_day;
					setting_month = clock_month;
					setting_year = clock_year; 
				}
				break;
				
			case MENU_SYSTEM_SLEEP_DELAY:
				StartEnterNumber(0, 120, sleepDelay, "s");
				break;

#ifdef NOKIA_LCD				
			case MENU_SYSTEM_LCD_CONTRAST:
				StartEnterNumber(0, 100, (lcd_vop & 0x7F) - 27, "");
				break;
				
			case MENU_SYSTEM_LCD_BIAS:
				StartEnterNumber(0, 7, (lcd_bias & 0x07), "");
				break;
				
			case MENU_SYSTEM_LCD_TEMP_COEF:
				StartEnterNumber(0, 3, (lcd_tempCoef & 0x03), "");
				break;
#endif	
#ifdef SSD1306_LCD
			case MENU_SYSTEM_LCD_CONTRAST:
				StartEnterNumber(0, 255, lcd_contrast, "");
				break;
#endif			
			case MENU_SYSTEM_RESTORE_DEFAULTS:
				Start2Choice("Are you sure?", "No", "Yes");
				break;
				
			case MENU_SYSTEM_ERASE_ALL_GRAPHS:
				Start2Choice("Are you sure?", "No", "Yes");
				break;
				
			case MENU_SYSTEM_SOUND:
				Start2Choice("", "On", "Off");
				subMenuItemIndex = 1 - soundEnable;				
				break;
		}				
	}
	else if (mode == MODE_ALTITUDE_INFO)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_ALTITUDE_CALIBRATE:
				StartEnterNumber(-1000, 15000, last_altitude + 0.5f, "ft");
				enterNumberPrompt = "Current True Altitude";
				break;
				
			case MENU_ALTITUDE_SET_GOAL:
				if (altitudeDestination != 0x7FFF)
				{
					altitudeDestination = 0x7FFF;
					menuLevel = 0;
					screenClearNeeded = 1;
				}
				else
				{
					StartEnterNumber(-1000, 15000, last_altitude + 0.5f, "ft");
				}
				break;
		}					
	}
	else if (mode == MODE_CURRENT_DATA)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_MAIN_SETUP1:
			case MENU_MAIN_SETUP2:
			case MENU_MAIN_SETUP3:
			case MENU_MAIN_SETUP4:
			case MENU_MAIN_SETUP5:
			case MENU_MAIN_SETUP6:
				SpeakerBeep(BEEP_ENTER);
				parentMode = mode;
				parentSelectedMenuItemIndex = selectedMenuItemIndex;
				mode = MODE_DATA_TYPE;
				selectedMenuItemIndex = mainScreenDataType[selectedMenuItemIndex-1];
				if (selectedMenuItemIndex > 4)
					topMenuItemIndex = selectedMenuItemIndex - 4;
				else
					topMenuItemIndex = 0;
				screenClearNeeded = 1;
				break;
		}
	}	
	else if (mode == MODE_DATA_TYPE)
	{
		mainScreenDataType[parentSelectedMenuItemIndex-1] = selectedMenuItemIndex;
		mode = parentMode;
		selectedMenuItemIndex = parentSelectedMenuItemIndex;
		if (selectedMenuItemIndex > 4)
			topMenuItemIndex = selectedMenuItemIndex - 4;
		else
			topMenuItemIndex = 0;
		
		menuLevel = 1;
		screenClearNeeded = 1;
	}	
	else if (mode == MODE_SNAPSHOTS)
	{
		switch (selectedMenuItemIndex)
		{
			case MENU_SNAPSHOTS_TAKE:
				snapshotNeeded = 1;
				menuLevel = 0;
				screenClearNeeded = 1;
				break;
				
			case MENU_SNAPSHOTS_VIEW:
				menuLevel++;
				topMenuItemIndex = 0;
				selectedMenuItemIndex = 0;
				break;
		}
	}
	else if (mode == MODE_GRAPH)
	{
		grandparentMode = parentMode;
		switch (selectedMenuItemIndex)
		{
			case MENU_GRAPH_EXPLORE:
				exploringGraph = 1;
				mode = parentMode;
				menuLevel = 0;
				screenClearNeeded = 1;
				break;
				
			case MENU_GRAPH_TYPE:
				Start2Choice("", "Lines", "Points");
				if (grandparentMode == MODE_TEMP_GRAPH)
					subMenuItemIndex = graphDrawPoints[GRAPH_TEMPERATURE];
				else if (grandparentMode == MODE_ALTITUDE_GRAPH)
					subMenuItemIndex = graphDrawPoints[GRAPH_ALTITUDE];
				else if (grandparentMode == MODE_PRESSURE_GRAPH)
					subMenuItemIndex = graphDrawPoints[GRAPH_PRESSURE];
				break;
				
			case MENU_GRAPH_YAXIS_MIN:
				if (parentMode == MODE_TEMP_GRAPH)
				{
					if (graphYMin[GRAPH_TEMPERATURE] != 0x7FFF)
					{
						graphYMin[GRAPH_TEMPERATURE] = 0x7FFF;
						menuLevel = 0;
						mode = parentMode;
						screenClearNeeded = 1;
					}
					else
					{
						StartEnterNumber(-10, 118, graphCurrentYMin, "'F");
					}		
				}					
				else if (parentMode == MODE_ALTITUDE_GRAPH)
				{
					if (graphYMin[GRAPH_ALTITUDE] != 0x7FFF)
					{
						graphYMin[GRAPH_ALTITUDE] = 0x7FFF;
						menuLevel = 0;
						mode = parentMode;
						screenClearNeeded = 1;
					}
					else
					{
						StartEnterNumber(-1384, 15000, graphCurrentYMin, "ft");
					}						
				}
				else if (parentMode == MODE_PRESSURE_GRAPH)
				{
					if (graphYMin[GRAPH_PRESSURE] != 0x7FFF)
					{
						graphYMin[GRAPH_PRESSURE] = 0x7FFF;
						menuLevel = 0;
						mode = parentMode;
						screenClearNeeded = 1;
					}
					else
					{
						StartEnterNumber(5, 37, graphCurrentYMin, "in");
					}						
				}
				break;
				
			case MENU_GRAPH_YAXIS_MAX:
				if (parentMode == MODE_TEMP_GRAPH)
				{
					if (graphYMax[GRAPH_TEMPERATURE] != 0x7FFF)
					{
						graphYMax[GRAPH_TEMPERATURE] = 0x7FFF;
						menuLevel = 0;
						mode = parentMode;
						screenClearNeeded = 1;
					}
					else
					{
						StartEnterNumber(-10, 118, graphCurrentYMax, "'F");
					}						
				}
				else if (parentMode == MODE_ALTITUDE_GRAPH)
				{
					if (graphYMax[GRAPH_ALTITUDE] != 0x7FFF)
					{
						graphYMax[GRAPH_ALTITUDE] = 0x7FFF;
						menuLevel = 0;
						mode = parentMode;
						screenClearNeeded = 1;
					}
					else
					{
						StartEnterNumber(-1384, 15000, graphCurrentYMax, "ft");
					}						
				}
				else if (parentMode == MODE_PRESSURE_GRAPH)
				{
					if (graphYMax[GRAPH_PRESSURE] != 0x7FFF)
					{
						graphYMax[GRAPH_PRESSURE] = 0x7FFF;
						menuLevel = 0;
						mode = parentMode;
						screenClearNeeded = 1;
					}
					else
					{
						StartEnterNumber(5, 37, graphCurrentYMax, "in");			
					}						
				}
				break;
		}
	}
}
 
void HandleMenuPrevNext(uint8_t step)
{
#ifdef SSD1306_LCD
	const int menuLines = 7;
#endif
#ifdef NOKIA_LCD
	const int menuLines = 5;
#endif
	
	const char** menu = modeMenus[mode];
	uint8_t numMenuItems = 0;
	while (pgm_read_word(&menu[numMenuItems]))
	{
		numMenuItems++;
	}
		
	selectedMenuItemIndex += step;
	if (selectedMenuItemIndex == numMenuItems)
		selectedMenuItemIndex = 0;
	else if (selectedMenuItemIndex == 0xFF)
		selectedMenuItemIndex = numMenuItems-1;
				
	if (selectedMenuItemIndex < topMenuItemIndex)
	{
		topMenuItemIndex = selectedMenuItemIndex;
	}	
	else if (selectedMenuItemIndex > topMenuItemIndex + menuLines - 1)
	{
		topMenuItemIndex = selectedMenuItemIndex - (menuLines - 1);
	}
}

// main				
void InitSettings()
{
	sleepDelay = 20;
	altitudeDestination = 0x7FFF;	
	
	mainScreenDataType[0] = MENU_DATA_ALTITUDE;
	mainScreenDataType[1] = MENU_DATA_TEMP;
	mainScreenDataType[2] = MENU_DATA_EMPTY;
#ifdef SHAKE_SENSOR	
	mainScreenDataType[3] = MENU_DATA_TRIP_TIME;
	mainScreenDataType[4] = MENU_DATA_RATE_OF_ASCENT;
#else
	mainScreenDataType[3] = MENU_DATA_RATE_OF_ASCENT;
	mainScreenDataType[4] = MENU_DATA_EMPTY;
#endif
	mainScreenDataType[5] = MENU_DATA_DATE_AND_TIME;
}

int main(void) 
{		
	// enable the internal pull-up resistors for buttons	
	PORTB |= (1<<BUTTON_NEXT) | (1<<BUTTON_SELECT) | (1<<BUTTON_PREV); 
											
	LcdReset();
	LcdClear();
	LcdPowerSave(0);
	_delay_ms(20);
		
	ClockInit();
	SamplingInit(0);
	
	SpeakerInit();
	SerialInit();
	InitSettings();
	
#ifdef SHAKE_SENSOR
	ShakeInit();
#endif
	
	// calibrate the internal RC oscillator
	PerformCalibration();
	// do it twice, because on reset immediately after programming, it always seems to get a bogus result
	_delay_ms(1);
	PerformCalibration(); 
	
	sei();	
	
	uint8_t try = 0;
	
	do {
		bmp085Init(); // interrupts must be enabled before initializing BMP085, since it uses I2C (aka TWI)
		if (bmpError != 0)	
		{
			LcdGoto(0,1);
			char str[21];
			strcpy_P(str, PSTR("Sensor err: "));
			itoa(bmpError, &str[strlen(str)], 10);
			LcdString(str);
			
			LcdGoto(0,2);
			strcpy_P(str, PSTR("Retry "));
			itoa(try, &str[strlen(str)], 10);
			LcdString(str);
		
			_delay_ms(2000);
			try++;
		}	
	} while (bmpError != 0);
			
	// initialize timer 2 to use an external 32768 Hz crystal 
    ASSR |= (1 << AS2); // use TOSC1/TOSC2 oscillator as timer 2 clock	
	ASSR &= ~(1 << EXCLK); // use a crystal oscillator rather than an external clock
    TCCR2A = 0; // normal counter mode
	TCCR2B = (1<<CS21) | (1<<CS20); // use oscillator/32 
    while (ASSR & 0x0F) {} // wait until all the asynchronous busy flags (low 4 bits) are zero
    _delay_ms (10); // wait until oscillator has stabilized   
    TIFR2 = (1 << TOV2); // clear the timer 2 interrupt flags 
    TIMSK2 = (1 << TOIE2); // enable timer 2 overflow interrupt

	// set the pin change interrupt masks for buttons
	PCMSK0 |= (1<<PCINT0) | (1<<PCINT1) | (1<<PCINT2);
	// set the state change interrupt mask for the serial input
	PCMSK0 |= (1<<PCINT4) ;
	// enable the interrupts
	PCICR |= (1<<PCIE0); 
				
	// disable unused peripherals
	PRR |= (1<<PRTWI) | (1<<PRSPI) | (1<<PRTIM1) | (1<<PRTIM0) | (1<<PRUSART0) | (1<<PRADC);

	// construct the version string
	uint8_t ind = 0;
	versionStr[ind++] = pgm_read_byte(&dateStr[9]);
	versionStr[ind++] = pgm_read_byte(&dateStr[10]);
	uint8_t month = StrToMonth(dateStr);
	versionStr[ind++] = '0' + (month/10);
	versionStr[ind++] = '0' + (month%10);
	uint8_t dayTens = pgm_read_byte(&dateStr[4]);
	if (dayTens == ' ')
	{
		versionStr[ind++] = '0';
	}
	else
	{
		versionStr[ind++] = dayTens;
	}		
	versionStr[ind++] = pgm_read_byte(&dateStr[5]);
	for (uint8_t i=0; i<5; i++)
	{
		if (i != 2)
		{
			versionStr[ind++] = pgm_read_byte(&timeStr[i]);		
		}			
	}	
	versionStr[ind++] = 0;
		
	newSampleNeeded = 1;
			
	while (1) 
	{	
		if (lcdResetNeeded)
		{
			lcdResetNeeded = 0;
			LcdReset();
			LcdClear();
			LcdPowerSave(0);
			screenUpdateNeeded = 1;
		}
		
		if (graphClearNeeded)
		{
			graphClearNeeded = 0;
			LcdGoto(0,0);
			LcdClear();
			SamplingInit(1);
		}
		
#ifdef SHAKE_SENSOR		
		// update the shake state once per minute, using the newSampleNeeded flag. 
		// must be done during the main loop, and not interrupt, this the ugly re-use of flag.
		if (newSampleNeeded)
		{
			ShakeUpdate();
		}
#endif	
		
		// take new sample
		if (newSampleNeeded || snapshotNeeded)
		{		
			short tempc;
			long pressure;
		
			PRR &= ~(1<<PRTWI); // enable I2C
		
			unsigned int ut = bmp085ReadUT();	
			tempc = bmp085ConvertTemperature(ut);
			unsigned long up = bmp085ReadUP();	
			pressure = bmp085ConvertPressure(up);
			
			PRR |= (1<<PRTWI); // disable I2C
				
			if (newSampleNeeded)
			{
				StoreSample(tempc, pressure);		
			}
						
			if (snapshotNeeded)
			{
				uint32_t packedYearMonthDayHourMin;
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					packedYearMonthDayHourMin = clock_year;
					packedYearMonthDayHourMin = packedYearMonthDayHourMin*13 + clock_month;
					packedYearMonthDayHourMin = packedYearMonthDayHourMin*32 + clock_day;
					packedYearMonthDayHourMin = packedYearMonthDayHourMin*24 + clock_hour;
					packedYearMonthDayHourMin = packedYearMonthDayHourMin*60 + clock_minute;
				}
				
				StoreSnapshot(tempc, pressure, packedYearMonthDayHourMin);
			}
			
			newSampleNeeded = 0;
			snapshotNeeded = 0;
		}
		
		// clear display
		if (screenClearNeeded)
		{
			screenClearNeeded = 0;
			LcdClear();
		}
		
		// update display
		if (screenUpdateNeeded)
		{	
			screenUpdateNeeded = 0;
			DrawModeScreen();
		}
		
		// keep sleeping until a redraw is required or a new sample is needed
		while (!newSampleNeeded && !screenUpdateNeeded && !snapshotNeeded)
		{
			if (!speaker_in_use)
			{
				set_sleep_mode(SLEEP_MODE_PWR_SAVE);
				sleep_mode();		
			}				
		}			
	}
}

long GetRateOfAscent()
{
	// get altitude 10 minutes ago
	const uint8_t ascentRateWindow = 10; // max window size is SAMPLES_PER_GRAPH-1
	uint8_t index = GetTimescaleNextSampleIndex(0); // get next sample index from the 1-minute timescale
	index--; // index of most recent sample
	index -= ascentRateWindow;
	if (index > SAMPLES_PER_GRAPH)
		index += SAMPLES_PER_GRAPH;
	Sample* pPastSample = GetSample(0, index);
			
	long ratePerMinute = 0;
	if (pPastSample->altitude)
	{	
		long pastAltitude = 200L * SAMPLE_TO_ALTITUDE(pPastSample->altitude);
				
		// compute rate of ascent
		ratePerMinute = ((long)(last_altitude*100) - pastAltitude) / ascentRateWindow;
		ratePerMinute = (ratePerMinute + 50) / 100;
		return ratePerMinute;
	}
	else
	{
		return 0x7FFFFFFF;
	}	
}

long GetTemperatureTrend()
{
	// get temp 60 minutes ago
	const uint8_t window = 60; // max window size is SAMPLES_PER_GRAPH-1
	uint8_t index = GetTimescaleNextSampleIndex(0); // get next sample index from the 1-minute timescale
	index--; // index of most recent sample
	index -= window;
	if (index > SAMPLES_PER_GRAPH)
		index += SAMPLES_PER_GRAPH;
	Sample* pPastSample = GetSample(0, index);
			
	long ratePerHour = 0;
	if (pPastSample->temperature)
	{	
		long pastTemperature = 5L * SAMPLE_TO_TEMPERATURE(pPastSample->temperature);
				
		// compute rate of change
		short tempf = (last_temperature * 9) / 5 + 320;
		ratePerHour = 60 * (tempf - pastTemperature) / window;
		return ratePerHour;
	}
	else
	{
		return 0x7FFFFFFF;
	}
}

long GetPressureTrend1()
{
	// get pressure 60 minutes ago
	const uint8_t window = 60; // max window size is SAMPLES_PER_GRAPH-1
	uint8_t index = GetTimescaleNextSampleIndex(0); // get next sample index from the 1-minute timescale
	index--; // index of most recent sample
	index -= window;
	if (index > SAMPLES_PER_GRAPH)
		index += SAMPLES_PER_GRAPH;
	Sample* pPastSample = GetSample(0, index);
			
	long ratePerHour = 0;
	if (pPastSample->pressure)
	{	
		long pastPressure = 50L * SAMPLE_TO_PRESSURE(pPastSample->pressure); // put in units of mb * 100
				
		// compute rate of change
		long pressure = last_pressure; // units of mb*100
		ratePerHour = 60 * (pressure - pastPressure) / window;
		return ratePerHour;
	}
	else
	{
		return 0x7FFFFFFF;
	}
}

// TODO: save memory by combining this with GetPressureTrend1()
long GetPressureTrend5()
{
	// get pressure 300 minutes ago
	const uint8_t window = 75; // max window size is SAMPLES_PER_GRAPH-1
	uint8_t index = GetTimescaleNextSampleIndex(1); // get next sample index from the 4-minute timescale
	index--; // index of most recent sample
	index -= window;
	if (index > SAMPLES_PER_GRAPH)
		index += SAMPLES_PER_GRAPH;
	Sample* pPastSample = GetSample(1, index);
			
	long ratePerHour = 0;
	if (pPastSample->pressure)
	{	
		long pastPressure = 50L * SAMPLE_TO_PRESSURE(pPastSample->pressure); // put in units of mb * 100
				
		// compute rate of change
		long pressure = last_pressure; // units of mb*100
		ratePerHour = 60 * (pressure - pastPressure) / (window * 4);
		return ratePerHour;
	}
	else
	{
		return 0x7FFFFFFF;
	}
}

uint8_t GetPressureChangeType(long mbX100PerHour)
{
	uint8_t pressureChange;
	if (mbX100PerHour == 0x7FFFFFFF)
	{
		pressureChange = CHANGE_STEADY;
	}
	else if (mbX100PerHour > 200)
	{
		// 2.01+ clearing from storm, strong winds (2.0 mb/hr = +0.059 in/hr)
		pressureChange = CHANGE_SOARING;
	}
	else if (mbX100PerHour > 120)
	{
		// 1.21 - 2.00 clearing from storm, moderate winds (1.2 mb/hr = +0.036 in/hr)
		pressureChange = CHANGE_RISING_QUICKLY;
	}
	else if (mbX100PerHour > 53)
	{
		// 0.54 - 1.20 (0.53mb/hr = 0.016 in/hr)
		pressureChange = CHANGE_RISING;
	}
	else if (mbX100PerHour > 0)
	{
		// 0.01 - 0.53
		pressureChange = CHANGE_RISING_SLOWLY;
	}
	else if (mbX100PerHour == 0)
	{
		pressureChange = CHANGE_STEADY;
	}	
	else if (mbX100PerHour > -54)
	{
		// -0.53 to -0.01
		pressureChange = CHANGE_FALLING_SLOWLY;
	}
	else if (mbX100PerHour > -121)
	{
		// -1.20 to -0.54
		pressureChange = CHANGE_FALLING;
	}
	else if (mbX100PerHour > -201)
	{
		// -2.00 to -1.21, stormy, windy
		pressureChange = CHANGE_FALLING_QUICKLY;
	}
	else
	{
		// -2.01+ very stormy, strong winds
		pressureChange = CHANGE_PLUMMETING;
	}
			
	return pressureChange;
}

void MakeDataString(char* str, uint8_t dataType)
{
	switch (dataType)
	{
		case MENU_DATA_EMPTY:
			*str = 0;
			break;
			
		case MENU_DATA_TEMP:
		{
			short tempf = (last_temperature * 9) / 5 + 320;	
			//sprintf(str, "%d.%d`F", tempf/10, abs(tempf)%10);
			dtostrf((double)tempf/10, 1, 1, str);
			strcat(str, "`F");
		}		
			break;
			
		case MENU_DATA_ALTITUDE:
		{
			long altitude = (long)(last_altitude*100);
			//sprintf(str, "%ld.%ld ft", altitude/100, labs(altitude)%100);
			dtostrf((double)altitude/100, 1, 2, str);
			strcat(str, " ft");
		}			
			break;
			
		case MENU_DATA_PRESSURE_STATION:
		{
			long pressure = 2953L * last_pressure; // 100000 * 0.0295301
			pressure = pressure / 100000;
			//sprintf(str, "Station Prs %ld.%ld in", pressure/100, pressure%100);
			strcpy_P(str, PSTR("Station Prs "));
			dtostrf((double)pressure/100, 1, 2, &str[strlen(str)]);
			strcat_P(str, PSTR(" in"));
		}		
			break;
			
		case MENU_DATA_PRESSURE_SEALEVEL:
		{
			float seaLevelPressure = bmp085GetSeaLevelPressure((float)last_pressure, 0.3048f * last_calibration_altitude);
			long sealLevelLong = 2953L * seaLevelPressure; // 100000 * 0.0295301
			sealLevelLong = sealLevelLong / 100000;
			//sprintf(str, "Sea Lev Prs %ld.%ld in", sealLevelLong/100, sealLevelLong%100);
			strcpy_P(str, PSTR("Sea Lev Prs "));
			dtostrf((double)sealLevelLong/100, 1, 2, &str[strlen(str)]);
			strcat_P(str, PSTR(" in"));
		}		
			break;			
				
		case MENU_DATA_RATE_OF_ASCENT:
		{
			long rate = GetRateOfAscent();	

			if (rate >= 0)
			{
				strcpy_P(str, PSTR("Ascending "));
			}
			else
			{
				strcpy_P(str, PSTR("Descending "));
			}	
			
			if (rate != 0x7FFFFFFF)		
			{
				ltoa(labs(rate), &str[strlen(str)], 10);			
			}
			else
			{
				strcat_P(str, PSTR("---"));
			}			
	
			strcat_P(str, PSTR(" ft/min"));
		}					
			break;
		
		case MENU_DATA_TIME_TO_DESTINATION:
		{
			long ratePerMinute = GetRateOfAscent();
			
			strcpy_P(str, PSTR("Time to Dest "));	
							
			long timeToGoal = ((altitudeDestination - (long)(last_altitude)) + ratePerMinute/2) / ratePerMinute;
			if (ratePerMinute == 0x7FFFFFFF || altitudeDestination == 0x7FFF || timeToGoal < 0 || timeToGoal > 24*99 + 59)
			{
				strcat_P(str, PSTR("-----"));
			}			
			else
			{
				if (timeToGoal / 60 < 10)
				{
					strcat_P(str, PSTR("0"));
				}	
				ltoa(timeToGoal / 60, &str[strlen(str)], 10);
				strcat_P(str, PSTR(":"));
				if (timeToGoal % 60 < 10)
				{
					strcat_P(str, PSTR("0"));
				}	
				ltoa(timeToGoal % 60, &str[strlen(str)], 10);
			}
	
		}				
			break;
			
		case MENU_DATA_TEMP_TREND:
		{
			// assume a daily high about 30F higher than daily low
			// must change 30 degrees in 12 hours = 2.5 deg/hour on average
			strcpy_P(str, PSTR("Temp "));
			long tempPerHour = GetTemperatureTrend(); // last 1 hour
			uint8_t tempChange;
			if (tempPerHour > 70)
			{
				// 7.1+
				tempChange = CHANGE_SOARING;
			}
			else if (tempPerHour > 40)
			{
				// 4.1 - 7.0
				tempChange = CHANGE_RISING_QUICKLY;
			}
			else if (tempPerHour > 10)
			{
				// 1.1-4.0
				tempChange = CHANGE_RISING;
			}
			else if (tempPerHour > 0)
			{
				// 0.1-1.0
				tempChange = CHANGE_RISING_SLOWLY;
			}
			else if (tempPerHour == 0)
			{
				// 0
				tempChange = CHANGE_STEADY;
			}	
			else if (tempPerHour > -11)
			{
				// -1.0 to -0.1
				tempChange = CHANGE_FALLING_SLOWLY;
			}
			else if (tempPerHour > -41)
			{
				// -4.0 to -1.1
				tempChange = CHANGE_FALLING;
			}
			else if (tempPerHour > -71)
			{
				// -7.0 to -4.1
				tempChange = CHANGE_FALLING_QUICKLY;
			}
			else
			{
				// -7.1+
				tempChange = CHANGE_PLUMMETING;
			}
			
			if (tempPerHour != 0x7FFFFFFF)
			{
				PGM_P pString = (PGM_P)pgm_read_word(&changeStrings[tempChange]);
				strcat_P(str, pString);
			}	
			else
			{
				PGM_P pString = (PGM_P)pgm_read_word(&changeStrings[CHANGE_STEADY]);
				strcat_P(str, pString);
			}			
		}		
			break;
			
		case MENU_DATA_PRESSURE_TREND:
		{
			// see http://www.islandnet.com/~see/weather/eyes/barometer3.htm
			strcpy_P(str, PSTR("Pressure "));
			long mbX10PerHour1 = GetPressureTrend1(); // last 1 hour
			uint8_t pressureChange = GetPressureChangeType(mbX10PerHour1);
			PGM_P pString = (PGM_P)pgm_read_word(&changeStrings[pressureChange]);
			strcat_P(str, pString);
		}		
			break;	
			
		case MENU_DATA_FORECAST:
		{
			// see http://www.islandnet.com/~see/weather/eyes/barometer3.htm
			strcpy_P(str, PSTR("Forecast "));
			long mbX100PerHour1 = GetPressureTrend1(); // last 1 hour
			uint8_t pressureChange1 = GetPressureChangeType(mbX100PerHour1);
			long mbX100PerHour5 = GetPressureTrend5(); // last 5 hour
			uint8_t pressureChange5 = GetPressureChangeType(mbX100PerHour5);
			
			if (mbX100PerHour1 == 0x7FFFFFFF || mbX100PerHour5 == 0x7FFFFFFF)
			{
				strcat_P(str, PSTR("Unavailable"));
			}
			// Sudden decrease, even if small, indicates a nearby disturbance; normally bringing wind, and short showers. 
			else if ((pressureChange1 == CHANGE_FALLING_QUICKLY || pressureChange1 == CHANGE_PLUMMETING) &&
					 (pressureChange5 < CHANGE_FALLING))
			{
				strcat_P(str, PSTR("Brief Shower"));		 
			}
			// Moderate, slow fall in pressure indicates low pressure area is passing at a distance. Any marked change in weather unlikely.
			else if (pressureChange1 == CHANGE_FALLING_SLOWLY && pressureChange5 == CHANGE_FALLING_SLOWLY)
			{
				strcat_P(str, PSTR("Unchanged"));
			}
			// Large, slow decrease indicates a long period of poor weather. Coming weather will be more pronounced if pressure started rising before dropping.
			else if ((pressureChange1 == CHANGE_FALLING_SLOWLY || pressureChange1 == CHANGE_FALLING) && 
					 (pressureChange5 == CHANGE_FALLING_SLOWLY || pressureChange5 == CHANGE_FALLING))
			{
				strcat_P(str, PSTR("Poor Weather"));
			}					
			// Large pressure drop signals a coming storm in 5 to 6 hours.
			else if (pressureChange1 >= CHANGE_FALLING_QUICKLY && pressureChange5 >= CHANGE_FALLING)
			{
				strcat_P(str, PSTR("Appr. Storm"));
			}
			// If pressure rise is large and prolonged, count on a many days of good weather ahead.
			else if ((pressureChange1 == CHANGE_RISING_SLOWLY || pressureChange1 == CHANGE_RISING) && 
					 (pressureChange5 == CHANGE_RISING_SLOWLY || pressureChange5 == CHANGE_RISING))
			{
				strcat_P(str, PSTR("Good Weather"));
			}				
			// If an upward and quick change, storminess is moving out and clearing may be coming in the very near future although it may be quite windy.
			else if ((pressureChange1 == CHANGE_RISING_QUICKLY || pressureChange1 == CHANGE_SOARING) && 
					 (pressureChange5 >= CHANGE_RISING))
			{
				strcat_P(str, PSTR("Clearing"));	
			}	 					
			// If the pressure is changing by more than 1 mb/hour and if the tendency is downward, expect more stormy weather on the way.
			else if (pressureChange1 == CHANGE_PLUMMETING)
			{
				strcat_P(str, PSTR("Very Stormy"));
			}
			else if (pressureChange1 == CHANGE_FALLING_QUICKLY)
			{
				strcat_P(str, PSTR("Stormy"));
			}
			else if (pressureChange1 == CHANGE_FALLING || pressureChange1 == CHANGE_FALLING_SLOWLY)
			{
				strcat_P(str, PSTR("Rain"));
			}
			else if (pressureChange1 == CHANGE_STEADY)
			{
				strcat_P(str, PSTR("Unchanged"));
			}
			else if (pressureChange1 == CHANGE_RISING_SLOWLY || pressureChange1 == CHANGE_RISING)
			{
				strcat_P(str, PSTR("Fair"));
			}
			else if (pressureChange1 == CHANGE_RISING_QUICKLY || pressureChange1 == CHANGE_SOARING)
			{
				strcat_P(str, PSTR("Clear, Dry"));
			}
			else
			{
				strcat_P(str, PSTR("Err "));
				itoa(pressureChange1, &str[strlen(str)], 10);
				strcat_P(str, PSTR(" "));
				itoa(pressureChange5, &str[strlen(str)], 10);
			}	
		}		
			break;
			
		case MENU_DATA_TIME_OF_DAY:
			MakeTimeString(str, clock_hour, clock_minute, clock_second);
			break;
			
		case MENU_DATA_DATE_AND_TIME:
			MakeDateString(str, clock_day, clock_month);		
			strcat_P(str, PSTR(" "));	
			char timeStr[8];
			strcat(str, MakeShortTimeString(timeStr, clock_hour, clock_minute));
			break;
			
#ifdef SHAKE_SENSOR
		case MENU_DATA_TRIP_TIME:
			strcpy_P(str, PSTR("Trip Time "));
			
			uint16_t tripTime = ShakeGetTripTime();
			itoa(tripTime / 60, &str[strlen(str)], 10);
			strcat_P(str, PSTR(":"));
			tripTime = tripTime % 60;
			if (tripTime < 10)
			{
				strcat_P(str, PSTR("0"));
			}
			itoa(tripTime, &str[strlen(str)], 10);
			
			if (ShakeIsMoving())
			{
				strcat_P(str, PSTR("+"));
			}
			break;
#endif
							
		default:
			strcpy_P(str, PSTR("Unimplemented"));
			break;
	}	
}

void MakeSnapshotDateString(char* str, uint32_t packedTime)
{
	uint8_t snap_minute = packedTime % 60;
	packedTime /= 60;
	uint8_t snap_hour = packedTime % 24;
	packedTime /= 24;
	uint8_t snap_day = packedTime % 32;
	packedTime /= 32;
	uint8_t snap_month = packedTime % 13;
	if (snap_month > 12)
		snap_month = 12;
	packedTime /= 13;
	uint8_t snap_year = packedTime;
		
	MakeDateString(str, snap_day, snap_month);
	strcat_P(str, PSTR(" "));
	itoa(snap_year+2000, &str[strlen(str)], 10);
	strcat_P(str, PSTR(" "));
	MakeShortTimeString(&str[strlen(str)], snap_hour, snap_minute);	
}

void MakeSnapshotValueString(char* str, Sample* pSample)
{
	MakeSampleValueString(str, GRAPH_ALTITUDE, SAMPLE_TO_ALTITUDE(pSample->altitude));
	strcat_P(str, PSTR("ft "));
	MakeSampleValueString(&str[strlen(str)], GRAPH_TEMPERATURE, SAMPLE_TO_TEMPERATURE(pSample->temperature));
	strcat_P(str, PSTR("F "));
	MakeSampleValueString(&str[strlen(str)], GRAPH_PRESSURE, SAMPLE_TO_PRESSURE(pSample->pressure));	
}

void DrawSnapshotList()
{
	int newestIndex = GetNewestSnapshotIndex();
	
	Snapshot* pSnapshot = GetSnapshot(newestIndex);
	
	if (pSnapshot->packedYearMonthDayHourMin == 0)
	{	
		LcdGoto(0, 2);
		LcdTinyString("List Is Empty", TEXT_NORMAL);
		return;
	}
	
	uint8_t numMenuItems = GetNumSnapshots();
	char str[22];
	
	for (int i=0; i<4; i++)
	{
		LcdGoto(0, i+1);
		int entryIndex = newestIndex-topMenuItemIndex-i;
		if (entryIndex < 0)
		{
			entryIndex += GetMaxSnapshots();
		}
		
		// print date line
		pSnapshot = GetSnapshot(entryIndex);
		uint32_t packedTime = pSnapshot->packedYearMonthDayHourMin;
		
		if (packedTime == 0)
			break;
		
		LcdWrite(LCD_DATA, topMenuItemIndex+i == selectedMenuItemIndex ? 0x7F : 0x00);
		itoa(entryIndex+1,str,10);
		LcdTinyString(str, topMenuItemIndex+i == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);
		uint8_t numLen = strlen(str);
		LcdWrite(LCD_DATA, topMenuItemIndex+i == selectedMenuItemIndex ? 0x5F : 0x20);
		LcdWrite(LCD_DATA, topMenuItemIndex+i == selectedMenuItemIndex ? 0x7F : 0x00);
		LcdWrite(LCD_DATA, topMenuItemIndex+i == selectedMenuItemIndex ? 0x7F : 0x00);
		MakeSnapshotDateString(str, packedTime);
		LcdTinyString(str, topMenuItemIndex+i == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);		
		for (int x=4+(numLen+strlen(str))*4; x<LCD_WIDTH; x++)
			LcdWrite(LCD_DATA, topMenuItemIndex+i == selectedMenuItemIndex ? 0x7F : 0x00);
		
		// show up/down arrows
		if (i == 0 && topMenuItemIndex != 0)
		{
			LcdGoto(80,i+1);
			LcdTinyString("^", topMenuItemIndex == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);
		}
	
		if (i==3 && topMenuItemIndex+i+1 != numMenuItems && selectedMenuItemIndex != 0xFF)
		{
			LcdGoto(80,i+1);
			LcdTinyString(";", topMenuItemIndex+i == selectedMenuItemIndex ? TEXT_INVERSE : TEXT_NORMAL);			
		}
	}	
	
	// print value line
	int selectedIndex = newestIndex-selectedMenuItemIndex;
	if (selectedIndex < 0)
	{
		selectedIndex += GetMaxSnapshots();
	}
		
	pSnapshot = GetSnapshot(selectedIndex);
		
	LcdGoto(0, 5);
	// format: " 14999ft 118.4F 29.34"
	Sample* pSample = &pSnapshot->sample;	
	LcdWrite(LCD_DATA, 0x01);
	MakeSnapshotValueString(str, pSample);
	LcdTinyStringFramed(str);
	
	for (int x=1+strlen(str)*4; x<LCD_WIDTH; x++)
		LcdWrite(LCD_DATA, 0x01);	
}

void HandleSnapshotsPrevNext(uint8_t step)
{
	uint8_t numMenuItems = GetNumSnapshots();
		
	selectedMenuItemIndex += step;
	if (selectedMenuItemIndex == numMenuItems)
		selectedMenuItemIndex = 0;
	else if (selectedMenuItemIndex == 0xFF)
		selectedMenuItemIndex = numMenuItems-1;
				
	if (selectedMenuItemIndex < topMenuItemIndex)
	{
		topMenuItemIndex = selectedMenuItemIndex;
	}	
	else if (selectedMenuItemIndex > topMenuItemIndex + 3)
	{
		topMenuItemIndex = selectedMenuItemIndex - 3;
	}	
}

void HandleSnapshotsSelect()
{
	SpeakerBeep(BEEP_ENTER);
	menuLevel = 0;
	screenClearNeeded = 1;
}

void DrawModeScreen()
{
	char str[22];
		
	if (unlockState != 0)
	{
#ifdef SSD1306_LCD	
		LcdGoto(0,0);
		for (int i=0; i<128*6; i++)
		{
			LcdWrite(LCD_DATA, pgm_read_byte(&logo[i]));
		}
		
		uint8_t topRow = 6;
#endif	
#ifdef NOKIA_LCD	
		uint8_t topRow = 1;
#endif
		LcdGoto(0,topRow);
		strcpy_P(str, PSTR(" hold PREV and NEXT"));
		LcdTinyString(str, TEXT_NORMAL);
		LcdGoto(0,topRow+1);
		strcpy_P(str, PSTR("      to unlock"));
		LcdTinyString(str, TEXT_NORMAL);	
	}
	else if (mode == MODE_CURRENT_DATA)
	{					
		if (menuLevel == 0)
		{
#ifdef NOKIA_LCD					
			const int cTopLineLen = 14;
			const int cTopLineHalfChar = 3;
			const int cBottomLineLen = 21;
			const int cBottomLineHalfChar = 2;
#endif
#ifdef SSD1306_LCD
			const int cTopLineLen = 21;
			const int cTopLineHalfChar = 3;
			const int cBottomLineLen = 21;
			const int cBottomLineHalfChar = 3;
#endif
				
			for (int i=0; i<2; i++)
			{
				MakeDataString(str, mainScreenDataType[i]);
				str[cTopLineLen] = 0;
				LcdGoto(0,i);
				for (int j=0; j<LCD_WIDTH; j++)
				{
					LcdWrite(LCD_DATA, 0x00);
					
				}
				LcdGoto(cTopLineHalfChar*(cTopLineLen-strlen(str)),i);
				LcdString(str);
			}
			
			for (int i=2; i<6; i++)
			{
				MakeDataString(str, mainScreenDataType[i]);
				str[cBottomLineLen] = 0;
				LcdGoto(0,i);
				for (int j=0; j<LCD_WIDTH; j++)
				{
					LcdWrite(LCD_DATA, 0x00);
					
				}
				LcdGoto(cBottomLineHalfChar*(cBottomLineLen-strlen(str)),i);
				LcdTinyString(str, TEXT_NORMAL);				
			}
		}
		else
		{
			LcdDrawHeading("MAIN SCREEN SETUP", TEXT_NORMAL);
			DrawMenu();
		}				
			 								 			
	}
	else if (mode == MODE_GRAPH)
	{
		if (parentMode == MODE_TEMP_GRAPH)
		{
			strcpy_P(str, PSTR("TEMP"));
		}
		else if (parentMode == MODE_PRESSURE_GRAPH)
		{
			strcpy_P(str, PSTR("PRESSURE"));
		}
		else
		{
			strcpy_P(str, PSTR("ALTITUDE"));	
		}
		
		strcat_P(str, PSTR(" PAST "));	
		MakeApproxTimeString(&str[strlen(str)], minutesPerSample[submode] * SAMPLES_PER_GRAPH);		
		LcdDrawHeading(str, menuLevel == 0);
	
		if (menuLevel == 1)
		{
			DrawMenu();
		}
	}
	else if (mode == MODE_SYSTEM)
	{
		LcdDrawHeading("SYSTEM", menuLevel == 0);
				
		if (menuLevel == 0)
		{
			LcdGoto(0,1);
			LcdTinyString("Version ", TEXT_NORMAL);
			LcdTinyString(versionStr, TEXT_NORMAL);
					
			long avrVcc = readVcc();
			avrVcc = (avrVcc + 5) / 10; // round to hundredths of a volt
			//sprintf(str, "Battery: %ld.%02ldV ", avrVcc/100, avrVcc%100);
			strcpy_P(str, PSTR("Battery "));
			dtostrf((double)avrVcc/100, 1, 2, &str[strlen(str)]);
			strcat_P(str, PSTR("V"));
			
			LcdGoto(0, 2);
			LcdTinyString(str, TEXT_NORMAL);	
	
#ifdef NOKIA_LCD					
			LcdGoto(0, 3);
			strcpy_P(str, PSTR("Contrast "));
			itoa((lcd_vop & 0x7F) - 27, &str[strlen(str)], 10);										
			LcdTinyString(str, TEXT_NORMAL);
#endif
#ifdef SSD1306_LCD
			LcdGoto(0, 3);
			strcpy_P(str, PSTR("Contrast "));
			itoa(lcd_contrast, &str[strlen(str)], 10);										
			LcdTinyString(str, TEXT_NORMAL);
#endif				
			LcdGoto(0, 4);
			strcpy_P(str, PSTR("Sound "));
			if (soundEnable)
			{
				strcat_P(str, PSTR("On"));
			}
			else
			{
				strcat_P(str, PSTR("Off"));
			}
			LcdTinyString(str, TEXT_NORMAL);
				
			MakeDateString(str, clock_day, clock_month);		
			LcdGoto(0, 5);
			LcdTinyString(str, TEXT_NORMAL);	
			strcpy_P(str, PSTR(" "));
			itoa(2000+clock_year, &str[strlen(str)], 10);
			LcdTinyString(str, TEXT_NORMAL);	
			MakeTimeString(str, clock_hour, clock_minute, clock_second);
			uint8_t len = strlen(str);
#ifdef NOKIA_LCD			
			LcdGoto(LCD_WIDTH-(len<<2),5);		
#endif
#ifdef SSD1306_LCD
			LcdGoto(LCD_WIDTH-len*6,5);	
#endif
			LcdTinyString(str, TEXT_NORMAL);
		}	
		else
		{
			DrawMenu();
		}							
	}
	else if (mode == MODE_SNAPSHOTS)
	{
		LcdDrawHeading("SNAPSHOTS", menuLevel == 0);
		
		if (menuLevel == 0)
		{
			uint8_t newestIndex = GetNewestSnapshotIndex();	
			Snapshot* pSnapshot = GetSnapshot(newestIndex);
	
			if (pSnapshot->packedYearMonthDayHourMin == 0)
			{	
				LcdGoto(0, 2);
				LcdTinyString("Snapshots Empty", TEXT_NORMAL);
			}
			else
			{
				strcpy_P(str, PSTR("Snapshot "));
				itoa(newestIndex + 1, &str[strlen(str)], 10);
				LcdGoto(0, 2);
				LcdTinyString(str, TEXT_NORMAL);
				
				MakeSnapshotDateString(str, pSnapshot->packedYearMonthDayHourMin);
				LcdGoto(0, 3);
				LcdTinyString(str, TEXT_NORMAL);
				
				MakeSnapshotValueString(str, &pSnapshot->sample);
				LcdGoto(0, 4);
				LcdTinyString(str, TEXT_NORMAL);
			}
			
		}
		else if (menuLevel == 1)
		{
			DrawMenu();
		}			
		else if (menuLevel == 2)
		{
			DrawSnapshotList();
		}
	}
	else if (mode == MODE_TEMP_GRAPH)
	{			
		LcdDrawGraph2(submode, GRAPH_TEMPERATURE, graphCursor, exploringGraph);
	}				
	else if (mode == MODE_PRESSURE_GRAPH)
	{
		LcdDrawGraph2(submode, GRAPH_PRESSURE, graphCursor, exploringGraph);
	}
	else if (mode == MODE_ALTITUDE_GRAPH)
	{
		LcdDrawGraph2(submode, GRAPH_ALTITUDE, graphCursor, exploringGraph);
	}		
	else if (mode == MODE_SET_TIME)
	{
		LcdDrawHeading("Date and Time", TEXT_NORMAL);
				
		LcdGoto(4*4,2);
		strcpy_P(str, PSTR(" "));
		char monthStr[7];
		MakeDateString(monthStr, 1, setting_month);
		monthStr[3]=0;
		strcat(str, monthStr);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, selectedMenuItemIndex==0 ? TEXT_INVERSE : TEXT_NORMAL);
				
		strcpy_P(str, PSTR(" "));
		if (setting_day < 10)
		{
			strcat_P(str, PSTR("0"));
		}
		itoa(setting_day, &str[strlen(str)], 10);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, selectedMenuItemIndex==1 ? TEXT_INVERSE : TEXT_NORMAL);
			
		strcpy_P(str, PSTR(" "));
		itoa(2000+setting_year, &str[strlen(str)], 10);
		strcat_P(str, PSTR(" "));		
		LcdTinyString(str, selectedMenuItemIndex==2 ? TEXT_INVERSE : TEXT_NORMAL);
				
		LcdGoto(2*4,4);
		uint8_t hr = (setting_hour == 0 || setting_hour == 12) ? 12 : setting_hour % 12;
		strcpy_P(str, PSTR(" "));
		if (hr < 10)
		{
			strcat_P(str, PSTR("0"));
		}
		itoa(hr, &str[strlen(str)], 10);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, selectedMenuItemIndex==3 ? TEXT_INVERSE : TEXT_NORMAL);
				
		LcdTinyString(":", TEXT_NORMAL);
		strcpy_P(str, PSTR(" "));
		if (setting_minute < 10)
		{
			strcat_P(str, PSTR("0"));
		}
		itoa(setting_minute, &str[strlen(str)], 10);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, selectedMenuItemIndex==4 ? TEXT_INVERSE : TEXT_NORMAL);		
				
		LcdTinyString(":", TEXT_NORMAL);
		strcpy_P(str, PSTR(" "));
		if (setting_second < 10)
		{
			strcat_P(str, PSTR("0"));
		}
		itoa(setting_second, &str[strlen(str)], 10);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, selectedMenuItemIndex==5 ? TEXT_INVERSE : TEXT_NORMAL);	
				
		strcpy_P(str, PSTR(" "));
		if (setting_hour < 12)
		{
			strcat_P(str, PSTR("AM"));
		}
		else
		{
			strcat_P(str, PSTR("PM"));
		}
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, selectedMenuItemIndex==6 ? TEXT_INVERSE : TEXT_NORMAL);			
	}
	else if (mode == MODE_ENTER_NUMBER)
	{
		const char** menu = modeMenus[parentMode];
		const char* pMenuItem = (PGM_P)pgm_read_word(&menu[selectedMenuItemIndex]);
		strcpy_P(str, pMenuItem);
		LcdDrawHeading(str, TEXT_NORMAL);
			
		if (enterNumberPrompt)
		{
			LcdGoto(4*(21-strlen(enterNumberPrompt))/2, 2);
			LcdTinyString(enterNumberPrompt, TEXT_NORMAL);
		}		
		
		uint8_t digitCount = 1;
		int16_t val = enterNumberMax;
		while (val > 9)
		{
			digitCount++;
			val /= 10;
		}
				
		LcdGoto(((21-digitCount)-3)/2*4, enterNumberPrompt ? 4 : 3);
		if (enterNumberValue < 0)
			strcpy_P(str, PSTR("-"));
		else
			strcpy_P(str, PSTR(" "));
		
		int16_t env = abs(enterNumberValue);
		
		int p = 10;
		while (digitCount > 1)
		{
			if (p > env)
				strcat_P(str, PSTR("0"));
			p *= 10;
			digitCount--;
		}

		itoa(env, &str[strlen(str)], 10);
		
		strcat_P(str, PSTR(" "));			
		LcdTinyString(str, TEXT_INVERSE);	
		
		if (strlen(enterNumberUnits))
		{
			LcdTinyString(enterNumberUnits, TEXT_INVERSE);	
			LcdTinyString(" ", TEXT_INVERSE);	
		}								
	}	
	else if (mode == MODE_CHOICE)
	{
		const char** menu = modeMenus[parentMode];
		const char* pMenuItem = (PGM_P)pgm_read_word(&menu[selectedMenuItemIndex]);
		strcpy_P(str, pMenuItem);
		LcdDrawHeading(str, TEXT_NORMAL);
				
		if (*choicePrompt)
		{
			LcdGoto(4*(21 - strlen(choicePrompt))/2, 2);
			LcdTinyString(choicePrompt, TEXT_NORMAL);
		}
			
		LcdGoto((7 - strlen(choice1))*4, *choicePrompt ? 4 : 3);
		strcpy_P(str, PSTR(" "));
		strcat(str, choice1);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, subMenuItemIndex == 0 ? TEXT_INVERSE : TEXT_NORMAL);
			
		LcdGoto(11 * 4, *choicePrompt ? 4 : 3);
		strcpy_P(str, PSTR(" "));
		strcat(str, choice2);
		strcat_P(str, PSTR(" "));
		LcdTinyString(str, subMenuItemIndex == 1 ? TEXT_INVERSE : TEXT_NORMAL);			
	}
	else if (mode == MODE_ALTITUDE_INFO)
	{
		LcdDrawHeading("ALTITUDE", menuLevel == 0);
				
		if (menuLevel == 0)
		{	
			MakeDataString(str, MENU_DATA_ALTITUDE);
			LcdGoto(0,1);
			LcdTinyString(str, TEXT_NORMAL);	
			
			MakeDataString(str, MENU_DATA_RATE_OF_ASCENT);
			LcdGoto(0,2);
			LcdTinyString(str, TEXT_NORMAL);
							
			if (altitudeDestination != 0x7FFF)
			{
				LcdGoto(0,4);
				strcpy_P(str, PSTR("Destination "));
				itoa(altitudeDestination, &str[strlen(str)], 10);
				strcat_P(str, PSTR(" ft "));
				LcdTinyString(str, TEXT_NORMAL);	
			}
				
			if (altitudeDestination != 0x7FFF)
			{		
				MakeDataString(str, MENU_DATA_TIME_TO_DESTINATION);		
				LcdGoto(0,5);	
				LcdTinyString(str, TEXT_NORMAL);
			}							
		}
		else
		{
			DrawMenu();
		}
	}	
	else if (mode == MODE_TEMP_INFO)
	{
		LcdDrawHeading("TEMPERATURE", menuLevel == 0);
				
		if (menuLevel == 0)
		{
			LcdGoto(0,1);
			MakeDataString(str, MENU_DATA_TEMP);
			LcdTinyString(str, TEXT_NORMAL);	
			
			LcdGoto(0,2);
			LcdTinyString("Trend ", TEXT_NORMAL);
			long tempTrend = GetTemperatureTrend();
			if (tempTrend == 0x7FFFFFFF)
			{
				strcpy_P(str, PSTR("---"));
			}
			else
			{
				//sprintf(str, "%+ld.%ld", tempTrend/10, labs(tempTrend)%10);
				if (tempTrend >= 0)
				{
					strcpy_P(str, PSTR("+"));
				}
				else
				{
					*str = 0;
				}
				dtostrf((double)tempTrend/10, 1, 1, &str[strlen(str)]);
			}
			LcdTinyString(str, TEXT_NORMAL);
			LcdTinyString(" deg/h", TEXT_NORMAL);
			
			LcdGoto(0,3);
			for (int j=0; j<LCD_WIDTH; j++)
			{
				LcdWrite(LCD_DATA, 0x00);				
			}
			LcdGoto(0,3);	
			MakeDataString(str, MENU_DATA_TEMP_TREND);
			LcdTinyString(str, TEXT_NORMAL);						
		}
		else
		{
			DrawMenu();
		}
	}
	else if (mode == MODE_PRESSURE_INFO)
	{
		LcdDrawHeading("PRESSURE", menuLevel == 0);
				
		if (menuLevel == 0)
		{
			LcdGoto(0,1);
			MakeDataString(str, MENU_DATA_PRESSURE_STATION);
			LcdTinyString(str, TEXT_NORMAL);		
			
			LcdGoto(0,2);
			long pressureTrend1 = GetPressureTrend1(); // units are mb * 100
			if (pressureTrend1 == 0x7FFFFFFF)
			{
				strcpy_P(str, PSTR("----"));
			}
			else
			{
				pressureTrend1 *= 2953;
				pressureTrend1 /= 100000;
				//sprintf(str, "%+ld.%02ld", pressureTrend1/100, labs(pressureTrend1)%100);
				
				if (pressureTrend1 >= 0)
				{
					strcpy_P(str, PSTR("+"));
				}
				else
				{
					*str = 0;
				}			
				dtostrf((double)pressureTrend1/100, 1, 2, &str[strlen(str)]);
			}
			LcdTinyString(str, TEXT_NORMAL);
			LcdTinyString(" in past 1h", TEXT_NORMAL);

			LcdGoto(0,3);
			for (int i=0; i<LCD_WIDTH; i++)
			{
				LcdWrite(LCD_DATA, 0x00);
			}
			LcdGoto(0,3);
			MakeDataString(str, MENU_DATA_PRESSURE_TREND);
			LcdTinyString(str, TEXT_NORMAL);
			
			LcdGoto(0,4);
			MakeDataString(str, MENU_DATA_PRESSURE_SEALEVEL);
			LcdTinyString(str, TEXT_NORMAL);
			
			/*LcdGoto(0,3);
			long pressureTrend5 = GetPressureTrend5();
			if (pressureTrend5 == 0x7FFFFFFF)
			{
				strcpy_P(str, PSTR("----"));
			}
			else
			{
				pressureTrend5 *= 2953;
				pressureTrend5 /= 100000;
				//sprintf(str, "%+ld.%02ld", pressureTrend5/100, labs(pressureTrend5)%100);
				
				if (pressureTrend5 >= 0)
				{
					strcpy_P(str, PSTR("+"));
				}
				else
				{
					*str = 0;
				}			
				dtostrf((double)pressureTrend5/100, 1, 2, &str[strlen(str)]);
			}
			LcdTinyString(str, TEXT_NORMAL);
			LcdTinyString(" in/h past 5h", TEXT_NORMAL);
			*/
							
			LcdGoto(0,5);
			for (int i=0; i<LCD_WIDTH; i++)
			{
				LcdWrite(LCD_DATA, 0x00);
			}
			LcdGoto(0,5);
			MakeDataString(str, MENU_DATA_FORECAST);
			LcdTinyString(str, TEXT_NORMAL);					
		}
		else
		{
			DrawMenu();
		}
	}
	else if (mode == MODE_DATA_TYPE)
	{
		const char** menu = modeMenus[parentMode];
		const char* pMenuItem = (PGM_P)pgm_read_word(&menu[parentSelectedMenuItemIndex]);
		strcpy_P(str, pMenuItem);
		LcdDrawHeading(str, TEXT_NORMAL);
		
		DrawMenu();
	}
}

uint32_t lastButtonTime = 0;

void HandlePrevNext(uint8_t step)
{		
	if (exploringGraph)
	{
		graphCursor += step;
		graphCursor = UINT_MOD(graphCursor, LCD_WIDTH);
	}
	else if ((mode < MODE_TOP_LEVEL_COUNT || mode == MODE_DATA_TYPE || mode == MODE_GRAPH) && menuLevel == 1)
	{
		HandleMenuPrevNext(step);
	}
	
	else if (mode < MODE_TOP_LEVEL_COUNT && menuLevel == 0)
	{			
		// cycle through the available modes
		SpeakerBeep(BEEP_NEXT);
					
		if (mode == MODE_TEMP_GRAPH || mode == MODE_PRESSURE_GRAPH || mode == MODE_ALTITUDE_GRAPH)
		{
			submode += step;
			if (submode == NUM_TIME_SCALES)
			{
				submode = 0;
				mode++;
			}
			else if (submode == 0xFF)
			{
				submode = NUM_TIME_SCALES-1;
				mode--;
			}
		}	
		else
		{
			mode += step;
				
			// moved backwards to a graph?
			if (step == 0xFF && (mode == MODE_TEMP_GRAPH || mode == MODE_PRESSURE_GRAPH || mode == MODE_ALTITUDE_GRAPH))
				submode = NUM_TIME_SCALES-1;
			else
				submode = 0;				
		}	

		mode = UINT_MOD(mode, MODE_TOP_LEVEL_COUNT);
		graphCursor = LCD_WIDTH - 1;
		screenClearNeeded = 1;
	}
	else if (mode == MODE_SET_TIME)
	{
		HandleSetTimePrevNext(step);
	}
	else if (mode == MODE_ENTER_NUMBER)
	{
		HandleEnterNumberPrevNext(step);
	}
	else if (mode == MODE_CHOICE)
	{
		HandleChoicePrevNext(step);
	}
	else if (mode == MODE_SNAPSHOTS)
	{
		HandleSnapshotsPrevNext(step);
	}			
}

void HandleSelect()
{
	if (mode == MODE_PRESSURE_GRAPH || mode == MODE_TEMP_GRAPH || mode == MODE_ALTITUDE_GRAPH)
	{	
		if (!exploringGraph)
		{
			SpeakerBeep(BEEP_ENTER);
			parentMode = mode;
			mode = MODE_GRAPH;
			menuLevel = 1;
			topMenuItemIndex = 0;
			selectedMenuItemIndex = 0;
		}
		else
		{
			SpeakerBeep(BEEP_EXIT);	
			exploringGraph = 0;
		}						
			
		screenClearNeeded = 1;
	}
	else if (mode < MODE_TOP_LEVEL_COUNT && menuLevel == 0)
	{
		if (modeMenus[mode])
		{
			// summon menu
			SpeakerBeep(BEEP_ENTER);
			menuLevel++;
			topMenuItemIndex = 0;
			selectedMenuItemIndex = 0;
			
			screenClearNeeded = 1;			
		}
		else
		{
			SpeakerBeep(BEEP_ERROR);
		}
	}			
	else if ((mode < MODE_TOP_LEVEL_COUNT || mode == MODE_DATA_TYPE || mode == MODE_GRAPH) && menuLevel == 1)
	{
		if (selectedMenuItemIndex == 0)
		{
			// exit menu
			SpeakerBeep(BEEP_EXIT);
			menuLevel--;
			
			if (mode == MODE_DATA_TYPE)
			{
				mode = parentMode;
				selectedMenuItemIndex = parentSelectedMenuItemIndex;
				menuLevel = 1;
			}
			
			if (mode == MODE_GRAPH)
			{
				mode = parentMode;
			}
		}
		else				
		{
			// handle menu item 
			SpeakerBeep(BEEP_ENTER);
			HandleMenuItem();
		}
			
		screenClearNeeded = 1;
	}
	else // navigate/exit special modes
	{
		if (mode == MODE_SET_TIME)
		{
			HandleSetTimeSelect();
		}
		else if (mode == MODE_ENTER_NUMBER)
		{
			HandleEnterNumberSelect();	
		}
		else if (mode == MODE_CHOICE)
		{
			HandleChoiceSelect();
		}
		else if (mode == MODE_SNAPSHOTS)
		{
			HandleSnapshotsSelect();
		}
	}
}

// quarter-second timer interrupt
ISR(TIMER2_OVF_vect)
{
	// NOTE: Note that the Status Register is not automatically stored when entering an interrupt routine, nor
	// restored when returning from an interrupt routine. This must be handled by software.

	ClockTick();
	
	// start of a new second?
	if ((clock_elapsedQuarterSeconds & 0x03) == 0)
	{
		// new second
		if (!hibernating && mode == MODE_SYSTEM && menuLevel == 0) 
		{
			screenUpdateNeeded = 1; // update when seconds change
		}
		
		// new minute?
		if (clock_second == 0)
		{
			newSampleNeeded = 1;
			
			if (!hibernating)
			{
				screenUpdateNeeded = 1; // update graphs when minutes change
			}		
		}
	}	
	
	if (!hibernating)
	{
		uint32_t now = (clock_elapsedQuarterSeconds<<8) | TCNT2;
		uint32_t timeSinceLastButton = now - lastButtonTime;
		
		// NEXT/PREV repeat?
		if (unlockState == 0 && (bit_is_clear(PINB, BUTTON_NEXT) || bit_is_clear(PINB, BUTTON_PREV)))
		{
			if (timeSinceLastButton > 6000)
			{
				HandlePrevNext(bit_is_clear(PINB, BUTTON_NEXT) ? 40 : 0xD8);
				screenUpdateNeeded = 1;
			}
			else if (timeSinceLastButton > 3000)
			{			
				HandlePrevNext(bit_is_clear(PINB, BUTTON_NEXT) ? 12 : 0xF4);
				screenUpdateNeeded = 1;
			}				
			else if (timeSinceLastButton > 500)
			{			
				HandlePrevNext(bit_is_clear(PINB, BUTTON_NEXT) ? 4 : 0xFC);
				screenUpdateNeeded = 1;
			}			
		}
		
		if (timeSinceLastButton > 1024L*sleepDelay)
		{
			// enter hibernation
			hibernating = 1;
			// put LCD in power-down mode	
			LcdPowerSave(1);
			// TODO: if hibernation engages just as a graph is being drawn, it will re-enable the LCD but think it's in hibernation
		}
	}
	
	// If reentering sleep mode within the TOSC1 cycle, the interrupt will immediately occur and the
	// device wake up again. The result is multiple interrupts and wake-ups within one TOSC1 cycle
	// from the first interrupt.
	// Use this method to ensure that one TOSC1 cycle has elapsed:
	TCCR2A = 0; // re-sets the value that is already set
    while (ASSR & (1<<TCR2AUB)) {} // wait until the asynchronous busy flag is zero	
}

// button and serial state change interrupt
ISR(PCINT0_vect) 
{ 
	// start of serial input?
	if (bit_is_clear(PINB, PB4))
	{
		// if hibernating, wake up
		/*if (hibernating)
		{
			lastButtonTime = (clock_elapsedQuarterSeconds<<8) | TCNT2;
			hibernating = 0;
			// put LCD in normal mode
			LcdPowerSave(0);
			screenClearNeeded = 1;
			screenUpdateNeeded = 1;
			unlockState = 1;
		}
		*/
		SerialDoCommand();
		return;
	}
	
	if (hibernating &&
		(bit_is_clear(PINB, BUTTON_NEXT) || bit_is_clear(PINB, BUTTON_PREV) || bit_is_clear(PINB, BUTTON_SELECT)))
	{
		// ignore button press, exit hibernation, wake screen and backlight
		lastButtonTime = (clock_elapsedQuarterSeconds<<8) | TCNT2;
		hibernating = 0;
		// put LCD in normal mode
		LcdPowerSave(0);
		screenClearNeeded = 1;
		screenUpdateNeeded = 1;
		unlockState = 1;
		return;
	}
	
	uint32_t now = (clock_elapsedQuarterSeconds<<8) | TCNT2;
	uint32_t timeSinceLastButton = now - lastButtonTime;
	
	if (unlockState == 1)
	{
		if (bit_is_clear(PINB, BUTTON_NEXT) && bit_is_clear(PINB, BUTTON_PREV) && bit_is_set(PINB, BUTTON_SELECT))
		{
			unlockState = 2;
		}			
	}
	else if (unlockState == 2)
	{
		if (bit_is_set(PINB, BUTTON_NEXT) && bit_is_set(PINB, BUTTON_PREV) && bit_is_set(PINB, BUTTON_SELECT) && timeSinceLastButton > 250)
		{
			unlockState = 0;
			screenClearNeeded = 1;
			screenUpdateNeeded = 1;			
		}			
	}	
	else if (bit_is_clear(PINB, BUTTON_NEXT) && bit_is_clear(PINB, BUTTON_PREV) && bit_is_clear(PINB, BUTTON_SELECT))
	{
		lcdResetNeeded = 1;
	}
	else if ((bit_is_clear(PINB, BUTTON_NEXT) || bit_is_clear(PINB, BUTTON_PREV)) && (timeSinceLastButton > 250))
	{				
		HandlePrevNext(bit_is_clear(PINB, BUTTON_NEXT) ? 1 : 0xFF);
		lastButtonTime = (clock_elapsedQuarterSeconds<<8) | TCNT2;	
		screenUpdateNeeded = 1;
	}	
	else if (bit_is_clear(PINB, BUTTON_SELECT) && (timeSinceLastButton > 250))
	{
		HandleSelect();
		lastButtonTime = (clock_elapsedQuarterSeconds<<8) | TCNT2;	
		screenUpdateNeeded = 1;	
	}
} 
