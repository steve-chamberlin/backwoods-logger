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

HANDLE InitSerialPort(_TCHAR* portName, unsigned long speed);
bool SendCommand(HANDLE hSerial, char cmd);
bool GetBytes(HANDLE hSerial, char* pBuffer, int bytesToRead, unsigned char& checksum);
bool GetResponseHeader(HANDLE hSerial);
bool VerifyChecksum(HANDLE hSerial, unsigned char checksum);
void GetFirmwareVersion(HANDLE hSerial);
void GetGraphs(HANDLE hSerial, _TCHAR* filename, bool saveAsCSV);
void GetSnapshots(HANDLE hSerial, _TCHAR* filename, bool saveAsCSV);
void AdjustTime(SYSTEMTIME& st, int minutesToSubtract);
bool WriteFileString(HANDLE hFile, char* str);
void ReportError();
void Usage();

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

typedef struct 
{
    unsigned int temperature:TEMPERATURE_BITS;
    unsigned int pressure:PRESSURE_BITS;
    unsigned int altitude:ALTITUDE_BITS;
} Sample;

typedef struct  
{
    unsigned long packedYearMonthDayHourMin;
    Sample sample;	
} Snapshot;
