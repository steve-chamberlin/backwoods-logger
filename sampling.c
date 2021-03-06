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

#include <avr/eeprom.h>
#include <util/atomic.h>
#include <stdlib.h>
#include "config.h"

#include "sampling.h"
#include "bmp085.h"
#include "clock.h"

#ifdef NOKIA_LCD
#include "noklcd.h"
#endif
#ifdef SSD1306_LCD
#include "ssd1306.h"
#endif

// EEPROM header format
// page 0
// 0-1: version (2 bytes)
// 2: reserved (1 byte)
// 3: first timescale next sample index (1 byte)
// page 1
// 4-5: reserved (3 bytes)
// 7: second timescale next sample index (1 byte)

#define EEPROM_HEADER_BASE 0
#define EEPROM_SIGNATURE 0xBEB3

#define EEPROM_SAMPLES_BASE 16

#define EEPROM_SNAPSHOTS_BASE (EEPROM_SAMPLES_BASE+(NUM_TIME_SCALES-NUM_SRAM_TIME_SCALES)*(SAMPLES_PER_GRAPH*sizeof(Sample)))
#define EEPROM_SNAPSHOTS_MAX ((1024-EEPROM_SNAPSHOTS_BASE)/sizeof(Snapshot))

short last_temperature; // units of 2 * degrees F (halves of a degree)
long last_pressure; // units of 100 * millibars (hundredths of a millibar)
float last_altitude; // units of feet
volatile short last_calibration_altitude;
volatile uint8_t useImperialUnits = 3;

Sample sampleData[NUM_SRAM_TIME_SCALES][SAMPLES_PER_GRAPH]; 
uint8_t nextSampleIndex[NUM_SRAM_TIME_SCALES]; 

#ifdef LOGGER_CLASSIC
// classic: 90m, 8h, 1.75d
uint16_t minutesPerSample[NUM_TIME_SCALES] = {1, 6, 30}; // must evenly divide 1440 for correct sample time detection
const char scaleShort[] PROGMEM = "90m";
const char scaleMed[] PROGMEM = "8h";
const char scaleLong[] PROGMEM = "2d";	
#endif
#ifdef LOGGER_MINI
// mini: 2h, 10h, 2.5d
uint16_t minutesPerSample[NUM_TIME_SCALES] = {1, 5, 30}; // must evenly divide 1440 for correct sample time detection
const char scaleShort[] PROGMEM = "2h";
const char scaleMed[] PROGMEM = "10h";
const char scaleLong[] PROGMEM = "2d";	
#endif

const char* scaleStrings[] = { 
	scaleShort, scaleMed, scaleLong
};
	
const char tempMetric[] PROGMEM = "`C";
const char pressureMetric[] PROGMEM = " mb";
const char altitudeMetric[] PROGMEM = " m";
const char tempEnglish[] PROGMEM = "`F";
const char pressureEnglish[] PROGMEM = " in";
const char altitudeEnglish[] PROGMEM = " ft";

const char* unitStrings[] = { 
	tempMetric, pressureMetric, altitudeMetric, tempEnglish, pressureEnglish, altitudeEnglish
};

int16_t minDataValues[] = {
	-23,  // tempMetric
	170,  // pressureMetric
	-305, // altitudeMetric
	-10,  // tempEnglish
	5,    // pressureEnglish
	-1000 // altitude English
};

int16_t maxDataValues[] = {
	48,   // tempMetric
	1253, // pressureMetric
	4572, // altitudeMetric
	118,  // tempEnglish
	37,   // pressureEnglish
	15000 // altitude English
};

uint32_t* GetSampleEEpromAddress(uint8_t timescaleNumber, uint8_t index)
{
	return (uint32_t*)EEPROM_SAMPLES_BASE + (timescaleNumber-NUM_SRAM_TIME_SCALES)*SAMPLES_PER_GRAPH + index;
}	

uint8_t GetTimescaleNextSampleIndex(uint8_t timescaleNumber)
{
	if (timescaleNumber < NUM_SRAM_TIME_SCALES)
		return nextSampleIndex[timescaleNumber];
	else 
	{
		// EEPROM
		uint8_t* eepromIndexAddress = (uint8_t*)EEPROM_HEADER_BASE + 3 + (timescaleNumber-NUM_SRAM_TIME_SCALES)*4;
		return eeprom_read_byte(eepromIndexAddress);
	}	
}

uint32_t sample_eeprom_dword;

Sample* GetSample(uint8_t timescaleNumber, uint8_t index)
{
	if (timescaleNumber < NUM_SRAM_TIME_SCALES)
		return &sampleData[timescaleNumber][index];
	else
	{
		// EEPROM
		sample_eeprom_dword = eeprom_read_dword(GetSampleEEpromAddress(timescaleNumber, index));
		return (Sample*)&sample_eeprom_dword;
	}
}

void FillSample(Sample* pSample, short temperatureRaw, long pressureRaw)
{
	last_pressure = pressureRaw;
	
	// temperature (F)
	long tempFSample = (temperatureRaw * 9) / 5 + 320; // convert to F tenths
	last_temperature = tempFSample / 5;
	if (tempFSample < TEMPERATURE_MIN)
	{
		tempFSample = TEMPERATURE_MIN;			 // cap at min value
	}
	tempFSample -= TEMPERATURE_MIN;
	tempFSample = (tempFSample + (TEMPERATURE_SCALE>>1)) / TEMPERATURE_SCALE;
	if (tempFSample > (1L<<TEMPERATURE_BITS)-1)
	{
		tempFSample = (1L<<TEMPERATURE_BITS)-1; // cap at max value
	}
	
	// pressure (MB)
	long pressureSample = pressureRaw;
	if (pressureSample < PRESSURE_MIN)
	{
		pressureSample = PRESSURE_MIN;			// cap at min value
	}
	pressureSample -= PRESSURE_MIN;
	pressureSample = (pressureSample + (PRESSURE_SCALE>>1)) / PRESSURE_SCALE;
	if (pressureSample > (1L<<PRESSURE_BITS)-1)
	{
		pressureSample = (1L<<PRESSURE_BITS)-1; // cap at max value
	}
	
	// altitude (FT) 
	last_altitude = 3.28f * bmp085GetAltitude((float)pressureRaw); 
	long altitudeSample = (long) (last_altitude + 0.5f);	
	if (altitudeSample < ALTITUDE_MIN)
	{
		altitudeSample = ALTITUDE_MIN;			 // cap at min value
	}
	altitudeSample -= ALTITUDE_MIN;
	altitudeSample = (altitudeSample + (ALTITUDE_SCALE>>1)) / ALTITUDE_SCALE;
	if (altitudeSample > (1L<<ALTITUDE_BITS)-1)
	{
		altitudeSample = (1L<<ALTITUDE_BITS)-1; // cap at max value
	}
	
	pSample->temperature = tempFSample;
	pSample->pressure = pressureSample;
	pSample->altitude = altitudeSample;
}

#if TRACK_DAILYHIGHLOW
static Sample _DailyHigh;
static Sample _DailyLow;
void ResetHighLow()
{
	// Samples store data in Degrees F, Inches and Feet
	// Sample values are unsigned bitfield vars so use 0 as the minimum and xFFFF as the maximum
	// Set the High sample initial value to the minimum value so the first sample will be higher
	// Set the Low sample initial value to the maximum value so the first sample will be lower
	_DailyHigh.temperature = 0;
	_DailyHigh.pressure = 0; 
	_DailyHigh.altitude = 0; 
	
	// calculate the max based on the number of bits available to each member
	_DailyLow.temperature = (1L<<TEMPERATURE_BITS)-1;
	_DailyLow.pressure = (1L<<PRESSURE_BITS)-1;
	_DailyLow.altitude = (1L<<ALTITUDE_BITS)-1;
}
void GetHighLow( Sample *high, Sample *low )
{
	if( NULL != high )
	{
		high->altitude = _DailyHigh.altitude;
		high->pressure = _DailyHigh.pressure;
		high->temperature = _DailyHigh.temperature;
	}	
	if( NULL != low )
	{
		low->altitude = _DailyLow.altitude;
		low->pressure = _DailyLow.pressure;
		low->temperature = _DailyLow.temperature;
	}	
}
void _UpdateHighLow( Sample *newSample )
{
	/* 
	 * This is a "private" function called only from StoreSample()
	 * so we wont bother to make sure the pointer is valid
	 */
	if( newSample->altitude > _DailyHigh.altitude )		
	{
		_DailyHigh.altitude = newSample->altitude;
	}
	if( newSample->altitude < _DailyLow.altitude )		
	{
		_DailyLow.altitude = newSample->altitude;
	}
	
	if( newSample->pressure > _DailyHigh.pressure )		
	{
		_DailyHigh.pressure = newSample->pressure;
	}
	if( newSample->pressure < _DailyLow.pressure )		
	{
		_DailyLow.pressure = newSample->pressure;
	}
	
	if( newSample->temperature > _DailyHigh.temperature )		
	{
		_DailyHigh.temperature = newSample->temperature;
	}
	if( newSample->temperature < _DailyLow.temperature )		
	{
		_DailyLow.temperature = newSample->temperature;
	}
}

#endif


// store a raw sample into one or more graphs
// temperatureRaw: tenths of degrees C
// pressureRaw: hundredths of millibars
void StoreSample(short temperatureRaw, long pressureRaw)
{
	Sample newSample;
	
	FillSample(&newSample, temperatureRaw, pressureRaw);

	#if TRACK_DAILYHIGHLOW
	_UpdateHighLow( &newSample );
	#endif
	
	for (uint8_t i=0; i<NUM_TIME_SCALES; i++)
	{
		if (i == 0 || (((int)clock_hour * 60 + clock_minute) % minutesPerSample[i]) == 0)
		{
			if (i<NUM_SRAM_TIME_SCALES)
			{		
				uint8_t index = nextSampleIndex[i];
				Sample* pSample = &sampleData[i][index];
			
				*pSample = newSample;
			
				index++;
				index %= SAMPLES_PER_GRAPH;
				nextSampleIndex[i] = index;
			}		
			else
			{
				// EEPROM			
				uint8_t* eepromIndexAddress = (uint8_t*)EEPROM_HEADER_BASE + 3 + (i-NUM_SRAM_TIME_SCALES)*4;
				uint8_t index = eeprom_read_byte(eepromIndexAddress);
				
				uint32_t* pDword = (uint32_t*)&newSample; // treat sample as a generic dword
				eeprom_update_dword(GetSampleEEpromAddress(i, index), *pDword);
				
				index++;
				index %= SAMPLES_PER_GRAPH;			
				eeprom_update_byte(eepromIndexAddress, index);					
			}	
		}
	}
}

void StoreSnapshot(short temperatureRaw, long pressureRaw, uint32_t packedYearMonthDayHourMin)
{
	Sample newSample;
	
	FillSample(&newSample, temperatureRaw, pressureRaw);
	
	// find the oldest snapshot
	uint8_t oldestIndex = 0;
	uint32_t oldestTime = 0xFFFFFFFF;
	for (uint8_t i=0; i<EEPROM_SNAPSHOTS_MAX; i++)
	{
		uint32_t* eepromAddress = (uint32_t*)(EEPROM_SNAPSHOTS_BASE + i*sizeof(Snapshot));
		uint32_t time = eeprom_read_dword(eepromAddress);
		if (time < oldestTime)
		{
			oldestTime = time;
			oldestIndex = i;
		}
	}
	
	// overwrite the oldest snapshot
	uint32_t* eepromAddress = (uint32_t*)(EEPROM_SNAPSHOTS_BASE + oldestIndex*sizeof(Snapshot));
	eeprom_update_dword(eepromAddress, packedYearMonthDayHourMin);
	uint32_t* pDword = (uint32_t*)&newSample; // treat sample as a generic dword
	eeprom_update_dword(eepromAddress+1, *pDword);
}

uint8_t GetNewestSnapshotIndex()
{
	// find the newest snapshot
	uint8_t newestIndex = 0;
	uint32_t newestTime = 0;
	for (uint8_t i=0; i<EEPROM_SNAPSHOTS_MAX; i++)
	{
		uint32_t* eepromAddress = (uint32_t*)(EEPROM_SNAPSHOTS_BASE + i*sizeof(Snapshot));
		uint32_t time = eeprom_read_dword(eepromAddress);
		if (time >= newestTime)
		{
			newestTime = time;
			newestIndex = i;
		}
	}
	
	return newestIndex;
}

Snapshot eepromSnapshot;

Snapshot* GetSnapshot(uint8_t index)
{
	uint32_t* eepromAddress = (uint32_t*)(EEPROM_SNAPSHOTS_BASE + index*sizeof(Snapshot));
	uint32_t packedTime = eeprom_read_dword(eepromAddress);
	
	eepromSnapshot.packedYearMonthDayHourMin = packedTime;
	
	uint32_t sampleDword = eeprom_read_dword(eepromAddress+1);
	Sample* pSample = (Sample*)&sampleDword;
	eepromSnapshot.sample = *pSample;
	
	return &eepromSnapshot;
}

uint8_t GetMaxSnapshots()
{
	return EEPROM_SNAPSHOTS_MAX;
}

uint8_t GetNumSnapshots()
{
	uint8_t count = 0;
	
	for (uint8_t i=0; i<EEPROM_SNAPSHOTS_MAX; i++)
	{
		uint32_t* eepromAddress = (uint32_t*)(EEPROM_SNAPSHOTS_BASE + i*sizeof(Snapshot));
		uint32_t time = eeprom_read_dword(eepromAddress);
		if (time == 0)
			break;
		
		count++;
	}
	
	return count;	
}

void SamplingInit(uint8_t forceEEpromClear)
{
	last_calibration_altitude = 0;
	
	for (uint8_t i=0; i<NUM_SRAM_TIME_SCALES; i++)
	{
		nextSampleIndex[i] = 0;
		
		for (uint8_t j=0; j<SAMPLES_PER_GRAPH; j++)
		{
			Sample* pSample = &sampleData[i][j];
			pSample->temperature = 0;
			pSample->pressure = 0;
			pSample->altitude = 0;
		}
	}
	
	// check EEPROM signature
	uint16_t signature = eeprom_read_word((uint16_t*)EEPROM_HEADER_BASE);
	if (signature == EEPROM_SIGNATURE && !forceEEpromClear)
	{
		//LcdString("EEPROM sig OK");
	}
	else
	{	
		LcdString("EEPROM init...");
		
		// write the header
		eeprom_update_dword((uint32_t*)EEPROM_HEADER_BASE, EEPROM_SIGNATURE); // signature, and first timescale index	
		eeprom_update_dword((uint32_t*)EEPROM_HEADER_BASE + 1, 0);
		
		// clear all the samples
		for (uint8_t scale=NUM_SRAM_TIME_SCALES; scale < NUM_TIME_SCALES; scale++)
		{
			for (uint8_t i=0; i<SAMPLES_PER_GRAPH; i++)
			{
				eeprom_update_dword(GetSampleEEpromAddress(scale, i), 0);
			}
		}
		
		// clear all the snapshots
		for (uint8_t i=0; i<EEPROM_SNAPSHOTS_MAX; i++)
		{
			uint32_t* eepromAddress = (uint32_t*)(EEPROM_SNAPSHOTS_BASE + i*sizeof(Snapshot));
			eeprom_update_dword(eepromAddress, 0);
			eeprom_update_dword(eepromAddress+1, 0);
		}
	}		
}

void MakeTemperatureString(char* str, int16_t val)
{
	// val is temperature in units of 2 * degrees F
	double fVal = val;
	if (!useImperialUnits)
	{
		fVal = ((fVal - 64) * 5) / 9;
	}
	dtostrf(fVal/2, 1, 1, str);	
}
	
void MakeTemperatureDifferenceString(char* str, int16_t val)
{
	// val is temperature difference in units of 2 * degrees F (half degree per unit)
	double fVal = val;
	if (!useImperialUnits)
	{
		fVal = (fVal * 5) / 9;
	}
	dtostrf(fVal/2, 1, 1, str);	
}
	
void MakePressureString(char* str, int16_t val)
{
	// val is pressure in units of 2 * millibars (half mb per unit)
	if (useImperialUnits)
	{
		int32_t longValue;		
		longValue = 2953L * val; // 100000 * 0.0295301
		val = longValue / 100000;
	}			
	dtostrf((double)val/2, 1, useImperialUnits?2:0, str);	
}	

void MakeAltitudeString(char* str, int16_t val)		
{
	// val is altitude in units of feet
	if (!useImperialUnits)
	{
		val = ((long)val * 100) / 328;
	}
	itoa(val, str, 10);	
}

void MakeSampleValueString(char* str, uint8_t type, int16_t sampleValue)
{
	if (type == GRAPH_TEMPERATURE)
	{
		MakeTemperatureString(str, sampleValue);
	}
	else if (type == GRAPH_PRESSURE)
	{
		MakePressureString(str, sampleValue);
	}
	else // altitude
	{
		MakeAltitudeString(str, sampleValue);
	}
}

void AppendSampleUnitsString(char* str, uint8_t type)
{	
	if (type == GRAPH_TEMPERATURE)
	{
		strcat_P(str, unitStrings[useImperialUnits + GRAPH_TEMPERATURE]);
	}
	else if (type == GRAPH_PRESSURE)
	{
		strcat_P(str, &unitStrings[useImperialUnits + GRAPH_PRESSURE][1]);
	}
	else
	{
		strcat_P(str, &unitStrings[useImperialUnits + GRAPH_ALTITUDE][1]);
	}
}

void MakeSampleValueAndUnitsString(char* str, uint8_t type, int16_t sampleValue)
{
	// concatenates the value and the unit string, with a space between them
	if (sampleValue >= INVALID_SAMPLE_MIN)
	{
		*str = 0;
		return;
	}
	
	MakeSampleValueString(str, type, sampleValue);
	strcat_P(str, PSTR(" "));
	AppendSampleUnitsString(str, type);
}

void MakeSampleValueAndUnitsStringForGraph(char* str, uint8_t type, int16_t sampleValue)
{
	// concatenates the value and the unit string, with no space between them
	if (sampleValue >= INVALID_SAMPLE_MIN)
	{
		*str = 0;
		return;
	}
	
	MakeSampleValueString(str, type, sampleValue);
	AppendSampleUnitsString(str, type);
}