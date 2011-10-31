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

#ifndef SAMPLING_H_
#define SAMPLING_H_

#include <inttypes.h>
#include <avr/pgmspace.h>

// temperature, in units of Farenheit * 10
// can store -10 to 117.5F, in 0.5F steps
#define TEMPERATURE_MIN -100
#define TEMPERATURE_SCALE 5
#define TEMPERATURE_BITS 8
// absolute pressure, in units of millibars * 100
// can store 200 to 1223 mb, in 0.5 mb steps
#define PRESSURE_MIN 20000
#define PRESSURE_SCALE 50
#define PRESSURE_BITS 11
// altitude, in units of ft * 100
// can store -1384 to 14999 ft, in 2 ft steps
#define ALTITUDE_MIN -1384
#define ALTITUDE_SCALE 2
#define ALTITUDE_BITS 13

#define SAMPLE_TO_TEMPERATURE(st) ((((long)st*TEMPERATURE_SCALE)+TEMPERATURE_MIN)/TEMPERATURE_SCALE)
#define SAMPLE_TO_PRESSURE(sp) ((((long)sp*PRESSURE_SCALE)+PRESSURE_MIN)/PRESSURE_SCALE)
#define SAMPLE_TO_ALTITUDE(sa) ((((long)sa*ALTITUDE_SCALE)+ALTITUDE_MIN)/ALTITUDE_SCALE)

#ifdef LOGGER_CLASSIC
#define SAMPLES_PER_GRAPH 84
#define NUM_TIME_SCALES 3
#define NUM_SRAM_TIME_SCALES 2
#endif
#ifdef LOGGER_MINI
#define SAMPLES_PER_GRAPH 128
#define NUM_TIME_SCALES 3
#define NUM_SRAM_TIME_SCALES 2
#endif

#define INVALID_SAMPLE_MIN 0x7FFE
#define INVALID_SAMPLE 0x7FFF
#define INVALID_RAW_VALUE 0xFFFF

enum {
	GRAPH_TEMPERATURE = 0,
	GRAPH_PRESSURE,
	GRAPH_ALTITUDE,
	GRAPH_COUNT
};

extern short last_temperature;
extern long last_pressure;
extern float last_altitude;
extern uint16_t minutesPerSample[];
extern volatile short last_calibration_altitude;
extern volatile uint8_t useImperialUnits;
extern const char* unitStrings[];
extern const char* scaleStrings[];
extern int16_t minDataValues[];
extern int16_t maxDataValues[];
	
typedef struct 
{
	unsigned int temperature:TEMPERATURE_BITS;
	unsigned int pressure:PRESSURE_BITS;
	unsigned int altitude:ALTITUDE_BITS;
} Sample;

typedef struct  
{
	uint32_t packedYearMonthDayHourMin;
	Sample sample;	
} Snapshot;

void SamplingInit(uint8_t forceEEpromClear);
void StoreSample(short temperatureRaw, long pressureRaw);
uint8_t GetTimescaleNextSampleIndex(uint8_t timescaleNumber);
Sample* GetSample(uint8_t timescaleNumber, uint8_t index);
void MakePressureString(char* str, int16_t val);
void MakeTemperatureString(char* str, int16_t val);	
void MakeTemperatureDifferenceString(char* str, int16_t val);
void MakeAltitudeString(char* str, int16_t val);	
void MakeSampleValueString(char* str, uint8_t type, int16_t sampleValue);
void AppendSampleUnitsString(char* str, uint8_t type);
void MakeSampleValueAndUnitsString(char* str, uint8_t type, int16_t sampleValue);
void MakeSampleValueAndUnitsStringForGraph(char* str, uint8_t type, int16_t sampleValue);
void StoreSnapshot(short temperatureRaw, long pressureRaw, uint32_t packedYearMonthDayHourMin);
uint8_t GetNewestSnapshotIndex();
Snapshot* GetSnapshot(uint8_t index);
uint8_t GetMaxSnapshots();
uint8_t GetNumSnapshots();

#endif /* SAMPLING_H_ */