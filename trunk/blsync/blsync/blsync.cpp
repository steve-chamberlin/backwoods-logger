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
  THE PRODUCT OR THE USE OR OTHER DEALINGS IN THE PRODUCT.\

  blsync - Sync application for the Backwoods Logger.

*/

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include "blsync.h"

using namespace std;

#define CMD_VERSION '1'
#define CMD_GETGRAPHS '2'
#define CMD_GETSNAPSHOTS '3'

int _tmain(int argc, _TCHAR* argv[])
{
    _TCHAR* portName = 0;
    _TCHAR* graphFilename = 0;
    _TCHAR* snapshotFilename = 0;
    unsigned long baudRate = 38400;
    bool userDefinedRate = false;
    bool reportVersion = false;
    bool saveAsCSV = true;
    
    if (argc < 2)
    {
        Usage();
        return 0;
    }

    for (int i = 1; i < argc; i++)
    {
        _TCHAR* arg = argv[i];
        if (arg[0] == _T('-'))
        {
            if (arg[1] == _T('p') && i+1 != argc) 
            {
                // port
                portName = argv[i + 1];
                i++;
            } 
            else if (arg[1] == _T('b') && i+1 != argc) 
            {
                // baud rate override
                userDefinedRate = true;
                baudRate = _wtoi(argv[i + 1]);
                i++;
            } 
            else if (arg[1] == _T('g') && i+1 != argc) 
            {
                // graph filename
                graphFilename = argv[i + 1];
                i++;
            } 
            else if (arg[1] == _T('s') && i+1 != argc) 
            {
                // snapshot filename
                snapshotFilename = argv[i + 1];
                i++;
            }
            else if (arg[1] == _T('v')) 
            {
                // report firmware version number
                reportVersion = true;
            } 
            else if (arg[1] == _T('c')) 
            {
                // save as CSV
                saveAsCSV = true;
            } 
            else if (arg[1] == _T('r')) 
            {
                // save as raw binary
                saveAsCSV = false;
            } 
            else 
            {
                wcout << "Unknown option " << arg << "." << endl << endl;
                Usage();
                return 0;
            }
        }
        else
        {
            wcout << "Unknown option " << arg << "." << endl << endl;
            Usage();
            return 0;
        }
    }

    if (portName == 0)
    {
        wcout << "Error: communication port was not specified." << endl << endl;
        Usage();
        return 0;
    }

    HANDLE hSerial = InitSerialPort(portName, baudRate);

    if (!userDefinedRate)
    {
        if (!AdjustBitRate(hSerial, portName, baudRate))
            return 0;
    }
    if (reportVersion)
    {
        GetFirmwareVersion(hSerial, true);
    }

    if (graphFilename != 0)
    {
        GetGraphs(hSerial, graphFilename, saveAsCSV);
    }

    if (snapshotFilename != 0)
    {
        GetSnapshots(hSerial, snapshotFilename, saveAsCSV);
    }

    if (hSerial != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hSerial);
    }

    return 0;
}

/* 
    SendCommand
    Sends the "CMD" prefix to the Logger, followed by the command ID.
    A short delay is inserted between each byte sent, to give the Logger enough
    time to process it before the next one arrives.
    Two zeroes precede the "CMD" prefix, to give the Logger enough time to
    enter the serial read interrupt routine.
    Returns true if successful, false if an error occured.
*/
bool SendCommand(HANDLE hSerial, char cmd)
{
    char szBuff[7] = { 0, 0, 'C', 'M', 'D', 0, 0};
    szBuff[5] = cmd; 
    DWORD dwBytesSent = 0;

    for (int i=0; i<6; i++)
    {
        if (!WriteFile(hSerial, &szBuff[i], 1, &dwBytesSent, NULL))
        {
            ReportError();
            return false;
        }
        else if (dwBytesSent != 1)
        {
            wcout << "Error: failed to send the command to the Logger." << endl;
            return false;
        }

        Sleep(20);
    }

    return true;
}

/* 
    GetBytes
    Attempts to read a user-defined number of bytes into a user-supplied buffer.
    Updates the checksum, which is passed by reference.
    Returns true if successful, false if an error occurred.
*/
bool GetBytes(HANDLE hSerial, char* pBuffer, int bytesToRead, unsigned char& checksum, bool showOutput)
{
    if (bytesToRead == 0)
        return true;

    DWORD dwBytesRead = 0;
    if (!ReadFile(hSerial, pBuffer, bytesToRead, &dwBytesRead, NULL))
    {
        ReportError();
    }
    else if (dwBytesRead == 0)
    {
         if (showOutput)
            wcout << "Error: no response was received from the Logger." << endl;
    }
    else if (dwBytesRead != bytesToRead)
    {
         if (showOutput)
             wcout << "Error: an incomplete response was received from the Logger." << endl;
    }        
    else
    {
        for (int i=0; i<bytesToRead; i++)
        {
            checksum ^= (unsigned char)pBuffer[i];
        }
        return true;
    }

    return false;
}

/* 
    GetResponseHeader
    Gets up to 300 bytes from the Logger, and stopping after the first occurrence of the string "LOG".
    Returns true if successful, false if an error occurred.
*/
bool GetResponseHeader(HANDLE hSerial, bool showOutput)
{
    char* prefix = "LOG";
    char index = 0;
    char readCount = 0;

    char szBuff[2] = {0};
    unsigned char checksum = 0;
    
    while (readCount < 300)
    {
        if (GetBytes(hSerial, szBuff, 1, checksum, showOutput))
        {
            readCount++;
           
            if (szBuff[0] == prefix[index])
            {
                index++;
                if (index == 3)
                {
                    return true;
                }
            }
            else if (szBuff[0] == prefix[0])
            {
                index = 1;
            }
            else
            {
                index = 0;
            }   
        }
        else
        {
            // GetBytes() failed
            break;
        }
    }

    if (readCount > 0)
    {
        if (showOutput)
            wcout << "Error: an incorrect response was received from the Logger." << endl;
    }

    return false;
}

/* 
    VerifyChecksum
    Gets a checksum byte from the Logger, and verifies it matches the passed-in checksum value.
    Returns true if successful, false if an error occurred.
*/
bool VerifyChecksum(HANDLE hSerial, unsigned char checksum, bool showOutput)
{
    char szBuff[2] = {0};
    unsigned char dummy = 0;
    
    // read the checksum byte
    if (GetBytes(hSerial, szBuff, 1, dummy, showOutput))
    {
        return (unsigned char)szBuff[0] == checksum;
    }

    return false;
}

/* 
    GetFirmwareVersion
    Get the firmware version number from the Logger, and optionally print it to the console
    Returns true if successful, false if an error occurred.
*/
bool GetFirmwareVersion(HANDLE hSerial, bool showOutput)
{
    if (!SendCommand(hSerial, CMD_VERSION))
        return false;

    if (!GetResponseHeader(hSerial, showOutput))
        return false;

    // get the version string
    char szBuff[32] = {0};
    unsigned char checksum = 0;
    char* p = szBuff;

    do
    {
        if (!GetBytes(hSerial, p, 1, checksum, showOutput))
            return false;
    } while (*p++ != 0);

    if (VerifyChecksum(hSerial, checksum, showOutput))
    {
        if (showOutput)
            wcout << "The Logger's firmware version number is " << szBuff << endl;
        return true;
    }

    return false;
}

/* 
    AdjustBitRate
    Silently attempts to retrieve the firmware version number using several different bit
    rates. The bitRate parameter (passed by reference) is set to the selected bit rate.
    Returns true if successful, false if an error occurred.
*/
bool AdjustBitRate(HANDLE& hSerial, _TCHAR* portName, unsigned long& bitRate)
{
    unsigned long defaultRate = bitRate;
    
    // try the default rate first
    if (GetFirmwareVersion(hSerial, false))
        return true;

    CloseHandle(hSerial);

    unsigned long alternateRates[6] = { 38000, 38800, 37600, 39200, 37200, 39600 };
    for (int i=0; i<6; i++)
    {
        bitRate = alternateRates[i];
        if (hSerial = InitSerialPort(portName, bitRate))
        {
            if (GetFirmwareVersion(hSerial, false))
            {
                wcout << "Adjusted the communication bit rate to  " << bitRate << endl;
                return true;
            }
            else
            {
                CloseHandle(hSerial);
            }
        }
    }

    // try the default rate once more, with error messages enabled
    bitRate = defaultRate;
    hSerial = InitSerialPort(portName, bitRate);
    if (GetFirmwareVersion(hSerial, true))
        return true;

    CloseHandle(hSerial);
    return false;
}

/* 
    AdjustTime
    Adjusts the given SYSTEMTIME by adding or subtracting a user-defined number of minutes.
*/
void AdjustTime(SYSTEMTIME& st, int minutesToSubtract)
{
    // convert SYSTEMTIME to FILETIME to LARGE_INTEGER
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);

    LARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    // adjust the time
    li.QuadPart += (LONGLONG)10000000 * 60 * minutesToSubtract;

    // convert LARGE_INTEGER to FILETIME to SYSTEMTIME
    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;

    FileTimeToSystemTime(&ft, &st);
}

/* 
    WriteFileString
    Writes a null-terminated string to file with the given handle.
    Returns true if successful, false if an error occurred.
*/
bool WriteFileString(HANDLE hFile, char* str)
{
    DWORD dwBytesSent = 0;
    DWORD bytesToSend = strlen(str);

    if (!WriteFile(hFile, str, bytesToSend, &dwBytesSent, NULL))
    {
        ReportError();
    }
    else if (dwBytesSent != bytesToSend)
    {
        wcout << "Error: Failed to save data to the file." << endl;
    }
    else
    {
        return true;
    }

    return false;
}

/* 
    GetGraphs
    Syncs the graph data from the Logger, and saves it to the specified file as CSV or raw binary.
*/
void GetGraphs(HANDLE hSerial, _TCHAR* filename, bool saveAsCSV)
{
    if (!SendCommand(hSerial, CMD_GETGRAPHS))
        return;

    if (!GetResponseHeader(hSerial, true))
        return;

    // get the graph data header
    char header[10] = {0};
    unsigned char checksum = 0;
    if (!GetBytes(hSerial, header, 9, checksum, true))
        return;

    unsigned char versionNumber = (unsigned char)header[0];
    if (versionNumber != 1)
    {
        wcout << "Error: Unsupported graph version number: " << versionNumber << endl;
        return;
    }

    unsigned char numberOfGraphs = (unsigned char)header[1];
    unsigned char samplesPerGraph = (unsigned char)header[2];

    // "now" time reference for the graphs
    // graph g is series of samples from the "now" time back to now - samplesPerGraph * minutesPerSample[g]
    unsigned char nowSecond = (unsigned char)header[3];
    unsigned char nowMinute = (unsigned char)header[4];
    unsigned char nowHour = (unsigned char)header[5];
    unsigned char nowDay = (unsigned char)header[6];
    unsigned char nowMonth = (unsigned char)header[7];
    unsigned char nowYear = (unsigned char)header[8];

    int graphDataBytes = numberOfGraphs * (sizeof(Sample) * samplesPerGraph + 2);

    unsigned char* pGraphData = (unsigned char*)malloc(graphDataBytes);
    if (!pGraphData)
    {
        wcout << "Error: Failed to allocate memory for the graph data buffer." << endl;
        return;
    }

    if (!GetBytes(hSerial, (char*)pGraphData, graphDataBytes, checksum, true))
        return;

    if (VerifyChecksum(hSerial, checksum, true))
    {
        // save the data to the file
        HANDLE hFile = CreateFile(filename,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                0,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                0);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            ReportError();
        }
        else
        {
            if (saveAsCSV)
            {
                // construct a system time for the "now" time reference
                SYSTEMTIME timeRef;
                GetSystemTime(&timeRef);
                timeRef.wYear = 2000 + nowYear;
                timeRef.wMonth = nowMonth;
                timeRef.wDay = nowDay;
                timeRef.wHour = nowHour;
                timeRef.wMinute = nowMinute;
                timeRef.wSecond = nowSecond;

                unsigned int graphSize = sizeof(Sample) * samplesPerGraph + 2;

                char buf[512];

                for (int g=0; g<numberOfGraphs; g++)
                {
                    sprintf_s(buf, 512, "Graph %d\n", g+1);
                    if (!WriteFileString(hFile, buf))
                    {
                        break;
                    }
                    WriteFileString(hFile, "Time, Temperature (deg F), Altitude (ft), Pressure (in)\n");

                    unsigned int sampleInterval = (unsigned int)pGraphData[graphSize * g]*256 + pGraphData[graphSize * g + 1];

                    // Samples are taken when the number of minutes since midnight is a multiple of the sampleInterval.
                    // Walk the reference time backwards to the first such occurrence to get the correct sample times.
                    SYSTEMTIME st = timeRef;
                    unsigned int minutesSinceMidnight = st.wHour * 60 + st.wMinute;
                    int minutesToAdjust = minutesSinceMidnight % sampleInterval;
                    AdjustTime(st, -minutesToAdjust);

                    // Set time back to the first sample
                    AdjustTime(st, -(int)sampleInterval * (samplesPerGraph-1));

                    for (int s=0; s<samplesPerGraph; s++)
                    {
                        Sample* pSample = (Sample*)&pGraphData[graphSize * g + 2 + sizeof(Sample) * s];
                        long temperature = SAMPLE_TO_TEMPERATURE(pSample->temperature); // degrees F * 2
                        long pressure = SAMPLE_TO_PRESSURE(pSample->pressure); // millibars * 2
                        long altitude = SAMPLE_TO_ALTITUDE(pSample->altitude); // feet / 2

                        sprintf_s(buf, 512, "%d/%d/%d %d:%02d %s,%.1f,%d,%.2f\n",
                                st.wMonth,
                                st.wDay,
                                st.wYear - 2000,
                                (st.wHour % 12) == 0 ? 12 : (st.wHour % 12),
                                st.wMinute,
                                st.wHour > 11 ? "PM" : "AM",
                                (float)temperature/2,
                                altitude*2,
                                (float)pressure/2*0.0295333727f);
                        WriteFileString(hFile, buf);
                        AdjustTime(st, sampleInterval);
                    }

                    WriteFileString(hFile, "\n");
                }

                wcout << "Saved CSV format graph data to " << filename << endl;
            }
            else
            {
                // save raw binary data      
                DWORD dwBytesSent = 0;
                if (!WriteFile(hFile, header, 9, &dwBytesSent, NULL))
                {
                    ReportError();
                }
                else if (dwBytesSent != 9)
                {
                    wcout << "Error: Failed to save data to the file." << endl;
                }
                else
                {
                    if (!WriteFile(hFile, pGraphData, graphDataBytes, &dwBytesSent, NULL))
                    {
                        ReportError();
                    }
                    else if (dwBytesSent != graphDataBytes)
                    {
                        wcout << "Error: Failed to save data to the file." << endl;
                    }
                    else
                    {
                        wcout << "Saved binary format graph data to " << filename << endl;
                    }
                }
            }

            CloseHandle(hFile);
        }
    }

    free(pGraphData);
}


/* 
    GetSnapshots
    Syncs the snapshot data from the Logger, and saves it to the specified file as CSV or raw binary.
*/
void GetSnapshots(HANDLE hSerial, _TCHAR* filename, bool saveAsCSV)
{
    if (!SendCommand(hSerial, CMD_GETSNAPSHOTS))
        return;

    if (!GetResponseHeader(hSerial, true))
        return;

    // get the snapshot data header
    char header[3] = {0};
    unsigned char checksum = 0;
    if (!GetBytes(hSerial, header, 2, checksum, true))
        return;

    unsigned char versionNumber = (unsigned char)header[0];
    if (versionNumber != 1)
    {
        wcout << "Error: Unsupported snapshot version number: " << versionNumber << endl;
        return;
    }

    unsigned char numberOfSnapshots = (unsigned char)header[1];

    int snapshotDataBytes = numberOfSnapshots * sizeof(Snapshot);

    unsigned char* pSnapshotData = (unsigned char*)malloc(snapshotDataBytes);
    if (!pSnapshotData)
    {
        wcout << "Error: Failed to allocate memory for the snapshot data buffer." << endl;
        return;
    }

    if (!GetBytes(hSerial, (char*)pSnapshotData, snapshotDataBytes, checksum, true))
        return;

    if (VerifyChecksum(hSerial, checksum, true))
    {
        // save the data to the file
        HANDLE hFile = CreateFile(filename,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                0,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                0);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            ReportError();
        }
        else
        {
            if (saveAsCSV)
            {
                char buf[512];

                WriteFileString(hFile, "Time, Temperature (deg F), Altitude (ft), Pressure (in)\n");

                for (int s=0; s<numberOfSnapshots; s++)
                {
                    Snapshot* pSnapshot = (Snapshot*)&pSnapshotData[sizeof(Snapshot) * s];
                    unsigned long packedTime = pSnapshot->packedYearMonthDayHourMin;

                    unsigned char snap_minute = packedTime % 60;
                    packedTime /= 60;
                    unsigned char snap_hour = packedTime % 24;
                    packedTime /= 24;
                    unsigned char snap_day = packedTime % 32;
                    packedTime /= 32;
                    unsigned char snap_month = packedTime % 13;
                    if (snap_month > 12)
                        snap_month = 12;
                    packedTime /= 13;
                    unsigned char snap_year = (unsigned char)packedTime;

                    Sample* pSample = &pSnapshot->sample;
                    long temperature = SAMPLE_TO_TEMPERATURE(pSample->temperature); // degrees F * 2
                    long pressure = SAMPLE_TO_PRESSURE(pSample->pressure); // millibars * 2
                    long altitude = SAMPLE_TO_ALTITUDE(pSample->altitude); // feet / 2

                    sprintf_s(buf, 512, "%d/%d/%d %d:%02d %s,%.1f,%d,%.2f\n",
                            snap_month,
                            snap_day,
                            snap_year,
                            (snap_hour % 12) == 0 ? 12 : (snap_hour % 12),
                            snap_minute,
                            snap_hour > 11 ? "PM" : "AM",
                            (float)temperature/2,
                            altitude*2,
                            (float)pressure/2*0.0295333727f);
                    WriteFileString(hFile, buf);
                }                

                wcout << "Saved CSV format snapshot data to " << filename << endl;
            }
            else
            {
                // save raw binary data      
                DWORD dwBytesSent = 0;
                if (!WriteFile(hFile, header, 2, &dwBytesSent, NULL))
                {
                    ReportError();
                }
                else if (dwBytesSent != 2)
                {
                    wcout << "Error: Failed to save data to the file." << endl;
                }
                else
                {
                    if (!WriteFile(hFile, pSnapshotData, snapshotDataBytes, &dwBytesSent, NULL))
                    {
                        ReportError();
                    }
                    else if (dwBytesSent != snapshotDataBytes)
                    {
                        wcout << "Error: Failed to save data to the file." << endl;
                    }
                    else
                    {
                        wcout << "Saved binary format snapshot data to " << filename << endl;
                    }
                }
            }

            CloseHandle(hFile);
        }
    }

    free(pSnapshotData);
}

/* 
    Usage
    Print program usage instructions to the console
*/
void Usage()
{
    wcout << "Backwoods Logger Sync Utility" << endl;
    wcout << "Usage: blsync -p port [-b speed] [-v] [-c] [-r] [-g filename] [-s filename]" << endl;
    wcout << "    -p port       Port to use for Logger communication, such as COM1." << endl;
    wcout << "    -b speed      Bit rate for communication. Default is 38400." << endl;
    wcout << "    -v            Display the Logger firmware version number." << endl;
    wcout << "    -c            Save files in CSV format. This is the default." << endl;
    wcout << "    -r            Save files in raw binary format instead of CSV." << endl;
    wcout << "    -g filename   Sync the graph data, and save it to the named file." << endl;
    wcout << "    -s filename   Sync the snapshot data, and save it to the named file." << endl;
}

/* 
    InitSerialPort
    Open the requested serial port, and set the speed and data/parity/stop bits
*/
HANDLE InitSerialPort(_TCHAR* portName, unsigned long speed)
{
    HANDLE hSerial;
    hSerial = CreateFile(portName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0);

    if (hSerial == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            wcout << "Error: Serial port " << portName << " does not exist." << endl;
        }
        else
        {
            ReportError();
        }
    }
    else
    {
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        
        if (!GetCommState(hSerial, &dcbSerialParams)) 
        {
            ReportError();
        }
        else
        {
            dcbSerialParams.BaudRate = speed;
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.Parity = NOPARITY;
            dcbSerialParams.StopBits = ONESTOPBIT;
            
            if (!SetCommState(hSerial, &dcbSerialParams))
            {
                ReportError();
            }
            else
            {
                COMMTIMEOUTS timeouts={0};
                timeouts.ReadIntervalTimeout=50;
                timeouts.ReadTotalTimeoutConstant=50;
                timeouts.ReadTotalTimeoutMultiplier=10;
                timeouts.WriteTotalTimeoutConstant=50;
                timeouts.WriteTotalTimeoutMultiplier=10;
                
                if (!SetCommTimeouts(hSerial, &timeouts))
                {
                    ReportError();
                }
            }
        }
    }

    return hSerial;
}

/* 
    ReportError
    Print a descriptive error string for the most recent error.
*/
void ReportError()
{
    _TCHAR lastError[1024];
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        lastError,
        1024,
        NULL);

    wcout << "Error: " << lastError << endl;
}
