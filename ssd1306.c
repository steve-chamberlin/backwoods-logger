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

#ifdef SSD1306_LCD

#include <string.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "ssd1306.h"
#include "glcdfont.c"
#include "clock.h"

volatile uint8_t lcd_contrast;
volatile int16_t graphCurrentYMin;
volatile int16_t graphCurrentYMax;
volatile int16_t graphYMin[GRAPH_COUNT];
volatile int16_t graphYMax[GRAPH_COUNT];
volatile uint8_t graphDrawPoints[GRAPH_COUNT];

inline void ssd1306_write(uint8_t dc, uint8_t c) 
{  	
	if (dc)
		OLED_CONTROL_PORT |= (1<<OLED_DC);
	else
		OLED_CONTROL_PORT &= ~(1<<OLED_DC);
		
	OLED_CONTROL_PORT &= ~(1<<OLED_CS);
  		  
	for (uint8_t i = 0; i < 8; i++)  
	{						
		OLED_DATA_PORT &= ~(1<<OLED_CLK); 
		
		if (c & 0x80)
			OLED_DATA_PORT |= (1<<OLED_MOSI);
		else
			OLED_DATA_PORT &= ~(1<<OLED_MOSI);
			
		c <<= 1;
					
		OLED_DATA_PORT |= (1<<OLED_CLK); 			
	}
		
	OLED_CONTROL_PORT |= (1<<OLED_CS);
	
	// leave data pin high
	OLED_DATA_PORT |= (1<<OLED_MOSI);
}

void ssd1306_init() 
{
	// set pin directions
	OLED_CONTROL_DDR |= (1<<OLED_DC) | (1<<OLED_CS) | (1<<OLED_RESET);
	OLED_DATA_DDR |= (1<<OLED_CLK) | (1<<OLED_MOSI);
	
	OLED_CONTROL_PORT |= (1<<OLED_CS);
	OLED_CONTROL_PORT |= (1<<OLED_RESET);
	
	 // VDD (3.3V) goes high at start, lets just chill for a ms
	_delay_ms(1);
	// bring reset low
	OLED_CONTROL_PORT &= ~(1<<OLED_RESET);
	// wait 10ms
	_delay_ms(10);
	// bring out of reset
	OLED_CONTROL_PORT |= (1<<OLED_RESET);
	// turn on VCC (9V?)

	ssd1306_write(OLED_CMD, SSD1306_DISPLAYOFF);  // 0xAE
	ssd1306_write(OLED_CMD, SSD1306_SETLOWCOLUMN | 0x0);  // low col = 0
	ssd1306_write(OLED_CMD, SSD1306_SETHIGHCOLUMN | 0x0);  // hi col = 0
  
	ssd1306_write(OLED_CMD, SSD1306_SETSTARTLINE | 0x0); // line #0

	ssd1306_write(OLED_CMD, SSD1306_SETCONTRAST);  // 0x81
	ssd1306_write(OLED_CMD, lcd_contrast);  // contrast
    
	ssd1306_write(OLED_CMD, 0xa1);  // segment remap 95 to 0 (?)

	ssd1306_write(OLED_CMD, SSD1306_NORMALDISPLAY); // 0xA6

	ssd1306_write(OLED_CMD, SSD1306_DISPLAYALLON_RESUME); // 0xA4

	ssd1306_write(OLED_CMD, SSD1306_SETMULTIPLEX); // 0xA8
	ssd1306_write(OLED_CMD, 0x3F);  // 0x3F 1/64 duty
  
	ssd1306_write(OLED_CMD, SSD1306_SETDISPLAYOFFSET); // 0xD3
	ssd1306_write(OLED_CMD, 0x0); // no offset
  
	ssd1306_write(OLED_CMD, SSD1306_SETDISPLAYCLOCKDIV);  // 0xD5
	ssd1306_write(OLED_CMD, 0x80);  // the suggested ratio 0x80
  
	ssd1306_write(OLED_CMD, SSD1306_SETPRECHARGE); // 0xd9
	ssd1306_write(OLED_CMD, 0xF1); // DC/DC
  
	ssd1306_write(OLED_CMD, SSD1306_SETCOMPINS); // 0xDA
	ssd1306_write(OLED_CMD, 0x12); // disable COM left/right remap
  
	ssd1306_write(OLED_CMD, SSD1306_SETVCOMDETECT); // 0xDB
	ssd1306_write(OLED_CMD, 0x40); // 0x20 is default?

	ssd1306_write(OLED_CMD, SSD1306_MEMORYMODE); // 0x20
	ssd1306_write(OLED_CMD, 0x00); // 0x0 act like ks0108
  
	// left to right scan
	ssd1306_write(OLED_CMD, SSD1306_SEGREMAP | 0x1);

	ssd1306_write(OLED_CMD, SSD1306_COMSCANDEC);

	ssd1306_write(OLED_CMD, SSD1306_CHARGEPUMP); //0x8D
	ssd1306_write(OLED_CMD, 0x14);  // enable    

	ssd1306_write(OLED_CMD, SSD1306_DISPLAYON);//--turn on oled panel
}

void ssd1306_char(uint8_t c, uint8_t inverse) 
{
	prog_uint8_t* pChar = font + ((c - 32) * 5);
	
	for (uint8_t i=0; i<5; i++ ) 
	{
		uint8_t pixels = pgm_read_byte(pChar);
		ssd1306_write(OLED_DATA, inverse ? pixels ^ 0x7F : pixels);
		pChar++;
	}
	ssd1306_write(OLED_DATA, inverse ? 0x7F : 0x00);
}

void ssd1306_string(char *c, uint8_t inverse) 
{
	while (*c != 0) 
	{
		ssd1306_char(*c, inverse);
		c++;
	}
}

void ssd1306_clear()
{
	ssd1306_goto(0,0);
	
	OLED_CONTROL_PORT |= (1<<OLED_DC);	
	OLED_CONTROL_PORT &= ~(1<<OLED_CS);
	OLED_DATA_PORT &= ~(1<<OLED_MOSI);
		
	for (uint16_t i=0; i<SSD1306_WIDTH*SSD1306_HEIGHT; i++)
	{		
		OLED_DATA_PORT &= ~(1<<OLED_CLK);
		OLED_DATA_PORT |= (1<<OLED_CLK); 	
	}				
			
	OLED_CONTROL_PORT |= (1<<OLED_CS);
	
	// leave data pin high
	OLED_DATA_PORT |= (1<<OLED_MOSI);
}

void ssd1306_goto(uint8_t x, uint8_t y)
{
	ssd1306_write(OLED_CMD, SSD1306_SETLOWCOLUMN | (x & 0xF));
	ssd1306_write(OLED_CMD, SSD1306_SETHIGHCOLUMN | ((x & 0xF0) >> 4));  
	ssd1306_write(OLED_CMD, SSD1306_SETPAGESTART | (y & 0x7));  
}


void LcdReset(void)
{
	for (uint8_t i=0; i<GRAPH_COUNT; i++)
	{
		graphYMin[i] = INVALID_SAMPLE;
		graphYMax[i] = INVALID_SAMPLE;
		graphDrawPoints[i] = 0;
	}
	
	lcd_contrast = 0xFF;
	
	ssd1306_init();
}

void LcdClear(void)
{
	ssd1306_clear();
}

void LcdPowerSave(uint8_t powerSaveOn)
{
	if (powerSaveOn)
	{
		ssd1306_write(OLED_CMD, SSD1306_DISPLAYOFF);
	}	
	else
	{
		ssd1306_write(OLED_CMD, SSD1306_DISPLAYON);
	}
}

void LcdSetBacklight(uint8_t on)
{
	// nop
}

void LcdDrawHeading(char *characters, uint8_t inverse)
{
	uint8_t len = strlen(characters);
	uint8_t lpad = (19 - len) >> 1;
	uint8_t rpad = 19 - len - lpad;
	
	LcdGoto(0,0);
	LcdWrite(LCD_DATA, inverse ? 0x7F : 0x00);
	LcdTinyString(inverse ? "<" : "[", inverse);
	if (!inverse)
	{
		LcdGoto(6,0);
		LcdWrite(LCD_DATA, 0x40);
	}
	for (uint8_t i=1; i<lpad; i++)
	{
		if (inverse)
			LcdTinyString(" ", TEXT_INVERSE);
		else
		{
			for (uint8_t j=0; j<6; j++)
			{
				LcdWrite(LCD_DATA, 0x40);
			}
		}
	}
	LcdTinyString(" ", inverse);
	LcdTinyString(characters, inverse);
	LcdTinyString(" ", inverse);
	for (uint8_t i=1; i<rpad; i++)
	{
		if (inverse)
			LcdTinyString(" ", TEXT_INVERSE);
		else
		{
			for (uint8_t j=0; j<6; j++)
			{
				LcdWrite(LCD_DATA, 0x40);
			}
		}
	}
	LcdWrite(LCD_DATA, inverse ? 0x7F : 0x40);
	LcdGoto(128-6,0);	
	LcdTinyString(inverse ? ">" : "]", inverse);	
	
}

void LcdDrawGraph2(uint8_t timescaleNumber, uint8_t type, uint8_t cursorPos, uint8_t showCursor)
{
	uint8_t xphase = GetTimescaleNextSampleIndex(timescaleNumber);
	Sample* pSample;
	
	// find the min and max values
	uint16_t rawValue;
	uint16_t minRawValue = INVALID_RAW_VALUE;
	uint16_t maxRawValue = 0;
	uint16_t rawCursorValue = INVALID_RAW_VALUE;
	short cursorValue;
	short minValue;
	short maxValue;
	
	for (uint8_t i=0; i<SAMPLES_PER_GRAPH; i++)
	{
		pSample = GetSample(timescaleNumber, i);
		
		// ignore unfilled samples
		if (pSample->temperature == 0 && pSample->pressure == 0 && pSample->altitude == 0)
			continue;
			
		if (type == GRAPH_TEMPERATURE)
		{
			rawValue = pSample->temperature;
		}
		else if (type == GRAPH_PRESSURE)
		{
			rawValue = pSample->pressure;
		}
		else
		{
			rawValue = pSample->altitude;
		}
		
		if (rawValue > maxRawValue)
			maxRawValue = rawValue;
		if (rawValue < minRawValue)
			minRawValue = rawValue;
			
		if (i == (cursorPos+xphase)%SAMPLES_PER_GRAPH)
		{
			rawCursorValue = rawValue;
		}
	}
	
	uint8_t customAxes = 0;
	
	if (type == GRAPH_TEMPERATURE)
	{
		cursorValue = SAMPLE_TO_TEMPERATURE(rawCursorValue);
		minValue = graphYMin[GRAPH_TEMPERATURE] == INVALID_SAMPLE ? SAMPLE_TO_TEMPERATURE(minRawValue) : graphYMin[GRAPH_TEMPERATURE];
		maxValue = graphYMax[GRAPH_TEMPERATURE] == INVALID_SAMPLE ? SAMPLE_TO_TEMPERATURE(maxRawValue) : graphYMax[GRAPH_TEMPERATURE];
		graphCurrentYMin = minValue / 2;
		graphCurrentYMax = maxValue / 2;
		customAxes = graphYMin[GRAPH_TEMPERATURE] != INVALID_SAMPLE || graphYMax[GRAPH_TEMPERATURE] != INVALID_SAMPLE;
	}
	else if (type == GRAPH_PRESSURE)
	{
		cursorValue = SAMPLE_TO_PRESSURE(rawCursorValue);
		minValue = graphYMin[GRAPH_PRESSURE] == INVALID_SAMPLE ? SAMPLE_TO_PRESSURE(minRawValue) : graphYMin[GRAPH_PRESSURE];  
		maxValue = graphYMax[GRAPH_PRESSURE] == INVALID_SAMPLE ? SAMPLE_TO_PRESSURE(maxRawValue) : graphYMax[GRAPH_PRESSURE];
		graphCurrentYMin = minValue / (2*33.86389f);
		graphCurrentYMax = maxValue / (2*33.86389f);
		customAxes = graphYMin[GRAPH_PRESSURE] != INVALID_SAMPLE || graphYMax[GRAPH_PRESSURE] != INVALID_SAMPLE;
	}
	else
	{
		cursorValue = SAMPLE_TO_ALTITUDE(rawCursorValue);
		minValue = graphYMin[GRAPH_ALTITUDE] == INVALID_SAMPLE ? SAMPLE_TO_ALTITUDE(minRawValue) : graphYMin[GRAPH_ALTITUDE]/2;
		maxValue = graphYMax[GRAPH_ALTITUDE] == INVALID_SAMPLE ? SAMPLE_TO_ALTITUDE(maxRawValue) : graphYMax[GRAPH_ALTITUDE]/2;
		graphCurrentYMin = minValue * 2;
		graphCurrentYMax = maxValue * 2;
		customAxes = graphYMin[GRAPH_ALTITUDE] != INVALID_SAMPLE || graphYMax[GRAPH_ALTITUDE] != INVALID_SAMPLE;
	}
	
	if (rawCursorValue == INVALID_RAW_VALUE)
	{
		cursorValue = INVALID_SAMPLE;
	}

	const int graphLastPixel = 55; // graphHeight - 1
	const int minGraphSampleRange = (graphLastPixel+1)/2; // don't allow very small vertical ranges, to prevent super-jaggies
		
	if (minRawValue == INVALID_RAW_VALUE)
	{
		// no filled samples
		minValue = INVALID_SAMPLE_MIN;
		maxValue = INVALID_SAMPLE;
		graphCurrentYMin = graphCurrentYMax = 0;
	}	
	else if (maxValue == minValue)
	{
		maxValue = minValue + 1;
	}	
	else if (maxValue - minValue < minGraphSampleRange)
	{
		// require at least 1 sample step per 2 Y pixels
		short avgValue = (minValue + maxValue) / 2;
		minValue = avgValue - minGraphSampleRange/2;
		maxValue = avgValue + minGraphSampleRange/2;
	}
	
	ssd1306_write(OLED_CMD, SSD1306_MEMORYMODE); // 0x20
	ssd1306_write(OLED_CMD, 0x01); // vertical
	
	// start at the right side of the graph	
	xphase += SAMPLES_PER_GRAPH - 1;
	xphase %= SAMPLES_PER_GRAPH;	
	
	short sampleValue;
	uint8_t prevysample = 0xFF;
	
	uint8_t yMinLabelBuffer[6*5];
	uint8_t yMaxLabelBuffer[6*5];
	
	uint8_t yMinSize;
	uint8_t yMaxSize;
	
	LcdMakeGraphYAxis(type, minValue, maxValue, yMinLabelBuffer, yMaxLabelBuffer, &yMinSize, &yMaxSize);
		
	for (uint8_t x=SAMPLES_PER_GRAPH-1; x<SAMPLES_PER_GRAPH; x--)
	{
		LcdGoto(x,1);
		
		uint8_t ormask[7];
		uint8_t andmask[7];
		
		for (uint8_t y=0; y<7; y++)
		{
			ormask[y] = (showCursor && (x == cursorPos)) ? 0x55 : 0;	
			andmask[y] = 0xFF;
			
			if (x < yMaxSize && !showCursor)
			{
				ormask[0] |= yMaxLabelBuffer[x];
				andmask[0] = 0xC0;
			}
			if (x < yMinSize && !showCursor)
			{		
				ormask[6] |= yMinLabelBuffer[x];
				andmask[6] = 0x03;
			}
		}
			
		pSample = GetSample(timescaleNumber, xphase);
		
		xphase--;
		if (xphase == 0xFF)
			xphase = SAMPLES_PER_GRAPH-1;
		
		if (type == GRAPH_TEMPERATURE)
		{
			sampleValue = SAMPLE_TO_TEMPERATURE(pSample->temperature);
		}
		else if (type == GRAPH_PRESSURE)
		{
			sampleValue = SAMPLE_TO_PRESSURE(pSample->pressure);
		}
		else
		{
			sampleValue = SAMPLE_TO_ALTITUDE(pSample->altitude);
		}
				
		uint8_t pixeldata[7];
		
		for (uint8_t y=0; y<7; y++)
		{
			pixeldata[y] = 0;
		}			
		
		/* this doesn't work right, and doesn't look very good either
		// show timescale tickmarks for major time boundaries
		uint8_t samplePeriod = minutesPerSample[timescaleNumber];
		int16_t sampleHourMin = hourMin - (LCD_WIDTH-1-x)*samplePeriod;
		
		if (((samplePeriod < 3) && (sampleHourMin % 30 == 0)) ||
			((samplePeriod < 8) && (sampleHourMin % 60 == 0)) ||
			((samplePeriod < 30) && (sampleHourMin % 240 == 0)) ||
			(sampleHourMin % 1440 == 0))
		{
			pixeldata[0] |= 0x03;
			pixeldata[4] |= 0xC0;
		}	
		*/
		
		if (sampleValue < minValue || sampleValue > maxValue ||
			(pSample->temperature == 0 && pSample->pressure == 0 && pSample->altitude == 0))
		{
			// sample is out of range or unfilled
			prevysample = 0xFF;		
		}
		else
		{
			uint8_t ysample = graphLastPixel - (long) graphLastPixel * (sampleValue - minValue) / (maxValue - minValue);
			uint8_t yrow = ysample >> 3;
			uint8_t ybit = ysample & 0x7; // ysample % 8
			uint8_t pixel = 0x01;
			
			while (ybit)
			{
				pixel = pixel << 1;
				ybit--;
			}		
			
			pixeldata[yrow] = pixel;
			
			if (ysample != prevysample && prevysample != 0xFF && !graphDrawPoints[type])
			{
				// modify the pixel data to extend a line towards the previous pixel
				if (prevysample < ysample)
				{
					// previous sample is above new one
					while (1)
					{
						while (ysample != prevysample && (pixeldata[yrow] & 0x1) == 0)
						{
							pixeldata[yrow] = pixeldata[yrow] | (pixeldata[yrow]>>1);
							prevysample++;
						}
						
						if (ysample == prevysample)
							break;
							
						yrow--;
						pixeldata[yrow] = 0x80;
						prevysample++;
					}															
				}
				else
				{
					// previous sample is equal or below new one
					while (1)
					{
						while (ysample != prevysample && (pixeldata[yrow] & 0x80) == 0)
						{
							pixeldata[yrow] = pixeldata[yrow] | (pixeldata[yrow]<<1);
							prevysample--;						
						}
						
						if (ysample == prevysample)
							break;
							
						yrow++;
						pixeldata[yrow] = 0x01;
						prevysample--;
					}			
				}
			}
			
			prevysample = ysample;
		}	
		
		for (uint8_t y=0; y<7; y++)
		{
			LcdWrite(LCD_DATA, (pixeldata[y] & andmask[y]) | ormask[y]);				
		}		
	}	
	
	ssd1306_write(OLED_CMD, SSD1306_MEMORYMODE); // 0x20
	ssd1306_write(OLED_CMD, 0x00); // horizontal
	
	char str[21];
	if (showCursor)
	{
		uint16_t minutesOfDay;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			minutesOfDay = clock_hour * 60 + clock_minute;
		}
	
		MakePastTimeString(str, minutesOfDay % minutesPerSample[timescaleNumber] + (SAMPLES_PER_GRAPH - 1 - cursorPos) * minutesPerSample[timescaleNumber]);
		LcdDrawGraphLeftLegend(str);
		
		for (uint8_t xclear = 1+strlen(str)*6; xclear < LCD_WIDTH; xclear++)
		{
			LcdWrite(LCD_DATA, 0x7F);
		}
		
		// convert altitude into the units expected by the formatting functions
		// TODO: harmonize all the units-related functions so this isn't necessary
		if (type == GRAPH_ALTITUDE)
		{
			cursorValue *= 2;
		}		
		MakeSampleValueAndUnitsStringForGraph(str, type, cursorValue);
		LcdDrawGraphRightLegend(str);
	}
	else
	{
		char timeStr[6];
		if (type == GRAPH_TEMPERATURE)
		{
			strcpy_P(str, PSTR("TEMP"));
		}
		else if (type == GRAPH_PRESSURE)
		{
			strcpy_P(str, PSTR("PRESSURE"));
		}
		else
		{
			strcpy_P(str, PSTR("ALTITUDE"));	
		}
		
		strcat_P(str, PSTR(" PAST "));	
		strcat(str, MakeApproxTimeString(timeStr, minutesPerSample[timescaleNumber] * SAMPLES_PER_GRAPH));		
		LcdDrawHeading(str, TEXT_INVERSE);
	}	
}

void LcdMakeGraphYAxis(uint8_t type, int yminlabel, int ymaxlabel, uint8_t* yMinBuffer, uint8_t* yMaxBuffer, uint8_t* yMinSize, uint8_t* yMaxSize)
{
	// convert altitude into the units expected by the formatting functions
	// TODO: harmonize all the units-related functions so this isn't necessary
	if (type == GRAPH_ALTITUDE)
	{
		yminlabel *= 2;
		ymaxlabel *= 2;
	}
		
	// clear frame buffers
	for (uint8_t i=0; i<4*5; i++)
	{
		yMinBuffer[i] = 0;
		yMaxBuffer[i] = 0;
	}	
	
	*yMaxSize=0;
	
	char str[7];
	//LcdGoto(0,1);
	MakeSampleValueString(str, type, ymaxlabel);
	
	// up-shift beyond the default
	char* characters = str;
	while (*characters)
	{
		unsigned short charbase = (*characters++ - 0x20) * 5;
		
		for (uint8_t index = 0; index < 5; index++)
		{
			uint8_t pixels = pgm_read_byte((unsigned char*)font + charbase + index);
			//LcdWrite(LCD_DATA, pixels);
			yMaxBuffer[(*yMaxSize)++] = pixels;
		}
		
		//LcdWrite(LCD_DATA, 0x00);
		yMaxBuffer[(*yMaxSize)++] = 0x00;
	}	
	
	//LcdGoto(0,5);
	*yMinSize = 0;
	MakeSampleValueString(str, type, yminlabel);
	
	// down-shift beyond the default
	characters = str;
	while (*characters)
	{
		unsigned short charbase = (*characters++ - 0x20) * 5;
		
		for (uint8_t index = 0; index < 5; index++)
		{
			uint8_t pixels = pgm_read_byte((unsigned char*)font + charbase + index);
			pixels = pixels << 1;
			//LcdWrite(LCD_DATA, pixels);
			yMinBuffer[(*yMinSize)++] = pixels;
		}			
		
		//LcdWrite(LCD_DATA, 0x00);
		yMinBuffer[(*yMinSize)++] = 0x00;
	}
}

void LcdDrawGraphLeftLegend(char *characters)
{
	LcdGoto(0,0);
	LcdWrite(LCD_DATA, 0x7F);
	LcdTinyString(characters, TEXT_INVERSE);	
}

void LcdDrawGraphRightLegend(char *characters)
{
	LcdGoto(LCD_WIDTH-(strlen(characters)*6),0);
	LcdTinyString(characters, TEXT_INVERSE);	
}

#endif // SSD1306_LCD