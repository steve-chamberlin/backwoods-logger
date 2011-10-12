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

#ifdef NOKIA_LCD

// Nokia 5110 monochrome LCD 84x46

#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <string.h>

#include "noklcd.h"
#include "sampling.h"
#include "clock.h"

// port B
#define LCD_PIN_SDIN PB3
#define LCD_PIN_SCLK PB5

// port D
#define LCD_PIN_DC PD7
#define LCD_PIN_SCE PD4
#define LCD_PIN_RESET PD5
#define LCD_PIN_BACKLIGHT PD3

volatile uint8_t lcd_vop;
volatile uint8_t lcd_bias;
volatile uint8_t lcd_tempCoef;

volatile int16_t graphCurrentYMin;
volatile int16_t graphCurrentYMax;
volatile int16_t graphYMin[GRAPH_COUNT];
volatile int16_t graphYMax[GRAPH_COUNT];
volatile uint8_t graphDrawPoints[GRAPH_COUNT];

static const prog_uint8_t tiny_font[][3] = {

	{0x00,0x00,0x00}, // 20  
	{0x00,0x17,0x00}, // 21 !
	{0x03,0x00,0x03}, // 22 "
	{0x1f,0x0a,0x1f}, // 23 #
	{0x0a,0x1f,0x05}, // 24 $
	{0x09,0x04,0x12}, // 25 %
	{0x0f,0x17,0x1c}, // 26 &
	{0x00,0x03,0x03}, // 27 ' -> degree
	{0x00,0x0e,0x11}, // 28 (
	{0x11,0x0e,0x00}, // 29 )
	{0x05,0x02,0x05}, // 2a *
	{0x04,0x0e,0x04}, // 2b +
	{0x10,0x08,0x00}, // 2c ,
	{0x04,0x04,0x04}, // 2d -		
	{0x00,0x10,0x00}, // 2e .	
	{0x18,0x04,0x03}, // 2f /				
	{0x1e,0x11,0x0f}, // 30 0
	{0x02,0x1f,0x00}, // 31 1
	{0x19,0x15,0x12}, // 32 2
	{0x15,0x15,0x0a}, // 33 3
	{0x07,0x04,0x1f}, // 34 4
	{0x17,0x15,0x09}, // 35 5		
	{0x1e,0x15,0x1d}, // 36 6
	{0x19,0x05,0x03}, // 37 7
	{0x1f,0x15,0x1f}, // 38 8
	{0x17,0x15,0x0f}, // 39 9	
	{0x00,0x0a,0x00}, // 3a :
	{0x04,0x0c,0x04}, // 3b ;	-> down arrow	
	{0x04,0x0e,0x1f}, // 3c <
	{0x0a,0x0a,0x0a}, // 3d =
	{0x1f,0x0e,0x04}, // 3e >
	{0x01,0x15,0x03}, // 3f ?			
	{0x0e,0x15,0x16}, // 40 @
	{0x1e,0x05,0x1e}, // 41 A
	{0x1f,0x15,0x0a}, // 42 B
	{0x0e,0x11,0x11}, // 43 C
	{0x1f,0x11,0x0e}, // 44 D
	{0x1f,0x15,0x15}, // 45 E
	{0x1f,0x05,0x05}, // 46 F
	{0x0e,0x15,0x1d}, // 47 G
	{0x1f,0x04,0x1f}, // 48 H
	{0x11,0x1f,0x11}, // 49 I
	{0x08,0x10,0x0f}, // 4a J
	{0x1f,0x04,0x1b}, // 4b K
	{0x1f,0x10,0x10}, // 4c L
	{0x1f,0x06,0x1f}, // 4d M
	{0x1f,0x0e,0x1f}, // 4e N		
	{0x0e,0x11,0x0e}, // 4f O	
	{0x1f,0x05,0x02}, // 50 P				
	{0x0e,0x19,0x1e}, // 51 Q
	{0x1f,0x0d,0x16}, // 52 R
	{0x12,0x15,0x09}, // 53 S
	{0x01,0x1f,0x01}, // 54 T
	{0x0f,0x10,0x1f}, // 55 U
	{0x07,0x18,0x07}, // 56 V		
	{0x1f,0x0c,0x1f}, // 57 W
	{0x1b,0x04,0x1b}, // 58 X
	{0x03,0x1c,0x03}, // 59 Y
	{0x19,0x15,0x13}, // 5a Z	
	{0x1f,0x10,0x10}, // 5b [
	{0x02,0x04,0x08}, // 5c backslash
	{0x10,0x10,0x1f}, // 5d ]
	{0x04,0x06,0x04}, // 5e ^
	{0x10,0x10,0x10}, // 5f _
	{0x00,0x03,0x03}, // 60 ` -> degree
	{0x1a,0x16,0x1c}, // 61 a
	{0x1f,0x12,0x0c}, // 62 b
	{0x0c,0x12,0x12}, // 63 c
	{0x0c,0x12,0x1f}, // 64 d
	{0x0c,0x1a,0x16}, // 65 e
	{0x04,0x1e,0x05}, // 66 f
	{0x0c,0x2a,0x1e}, // 67 g
	{0x1f,0x02,0x1c}, // 68 h
	{0x00,0x1d,0x00}, // 69 i
	{0x10,0x20,0x1d}, // 6a j 
	{0x1f,0x0c,0x12}, // 6b k
	{0x11,0x1f,0x10}, // 6c l
	{0x1e,0x0e,0x1e}, // 6d m
	{0x1e,0x02,0x1c}, // 6e n
	{0x0c,0x12,0x0c}, // 6f o
	{0x3e,0x12,0x0c}, // 70 p
	{0x0c,0x12,0x3e}, // 71 q
	{0x1c,0x02,0x02}, // 72 r
	{0x14,0x1e,0x0a}, // 73 s
	{0x02,0x1f,0x12}, // 74 t
	{0x0e,0x10,0x1e}, // 75 u
	{0x0e,0x18,0x0e}, // 76 v
	{0x1e,0x1c,0x1e}, // 77 w
	{0x12,0x0c,0x12}, // 78 x
	{0x06,0x28,0x1e}, // 79 y
	{0x1a,0x1f,0x16}, // 7a z															
};
	
static const prog_uint8_t ASCII[][5] = {
	 {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
	,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
	,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
	,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
	,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
	,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
	,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
	,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
	,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
	,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
	,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
	,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
	,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
	,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
	,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
	,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
	,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
	,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
	,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
	,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
	,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
	,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
	,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
	,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
	,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
	,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
	,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
	,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
	,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
	,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
	,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
	,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
	,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
	,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
	,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
	,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
	,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
	,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
	,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
	,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
	,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
	,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
	,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
	,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
	,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
	,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
	,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
	,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
	,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
	,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
	,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
	,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
	,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
	,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
	,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
	,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
	,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
	,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
	,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
	,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
	,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c backslash
	,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
	,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
	,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
	,{0x00, 0x06, 0x05, 0x03, 0x00} // 60 ` -> degree
	,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
	,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
	,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
	,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
	,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
	,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
	,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
	,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
	,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
	,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
	,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
	,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
	,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
	,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
	,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
	,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
	,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
	,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
	,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
	,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
	,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
	,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
	,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
	,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
	,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
	,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
};

void LcdWrite(uint8_t dc, uint8_t data)
{
	if (dc)
	{
		PORTD |= (1<<LCD_PIN_DC); 
	}
	else
	{
		PORTD &= ~(1<<LCD_PIN_DC);
	}
			
	PORTD &= ~(1<<LCD_PIN_SCE);

	for (int i = 0; i < 8; i++)  
	{						
		if (data & 0x80)
			PORTB |= (1<<LCD_PIN_SDIN);
		else
			PORTB &= ~(1<<LCD_PIN_SDIN);
			
		data <<= 1;
					
		PORTB |= (1<<LCD_PIN_SCLK); 		
		PORTB &= ~(1<<LCD_PIN_SCLK); 			
	}

	PORTD |= (1<<LCD_PIN_SCE);
}

void LcdGoto(int x, int y)
{
	LcdWrite(LCD_CMD, 0x80 | x);  
	LcdWrite(LCD_CMD, 0x40 | y);

}

void LcdCharacter(char character)
{
	unsigned short charbase = (character - 0x20) * 5;
		
	for (int index = 0; index < 5; index++)
	{
		LcdWrite(LCD_DATA, pgm_read_byte((unsigned char*)ASCII + charbase + index));
	}
	
	LcdWrite(LCD_DATA, 0x00);
}

void LcdString(char *characters)
{
	while (*characters)
	{
		LcdCharacter(*characters++);
	}
}

void LcdTinyString(char *characters, uint8_t inverse)
{
	while (*characters)
	{
		if (*characters == 'm')
		{
			// special case 'm'
			characters++;
			LcdWrite(LCD_DATA, inverse ? 0x3c ^ 0x7F : 0x3c);
			LcdWrite(LCD_DATA, inverse ? 0x04 ^ 0x7F : 0x04);
			LcdWrite(LCD_DATA, inverse ? 0x18 ^ 0x7F : 0x18);
			LcdWrite(LCD_DATA, inverse ? 0x04 ^ 0x7F : 0x04);
			LcdWrite(LCD_DATA, inverse ? 0x38 ^ 0x7F : 0x38);
			LcdWrite(LCD_DATA, inverse ? 0x7F : 0x00);
		}
		else
		{	
			unsigned short charbase = (*characters++ - 0x20) * 3;
		
			for (int index = 0; index < 3; index++)
			{
				uint8_t pixels = pgm_read_byte((unsigned char*)tiny_font + charbase + index);
				pixels = pixels << 1;
				LcdWrite(LCD_DATA, inverse ? pixels ^ 0x7F : pixels);
			}
		
			LcdWrite(LCD_DATA, inverse ? 0x7F : 0x00);
		}		
	}	
}

void LcdTinyStringFramed(char *characters)
{
	while (*characters)
	{
		if (*characters == 'm')
		{
			// special case 'm'
			characters++;
			LcdWrite(LCD_DATA, (0x3c << 1) | 0x01);
			LcdWrite(LCD_DATA, (0x04 << 1) | 0x01);
			LcdWrite(LCD_DATA, (0x18 << 1) | 0x01);
			LcdWrite(LCD_DATA, (0x04 << 1) | 0x01);
			LcdWrite(LCD_DATA, (0x38 << 1) | 0x01);
			LcdWrite(LCD_DATA, (0x00 << 1) | 0x01);
		}
		else
		{	
			unsigned short charbase = (*characters++ - 0x20) * 3;
		
			for (int index = 0; index < 3; index++)
			{
				uint8_t pixels = pgm_read_byte((unsigned char*)tiny_font + charbase + index);
				pixels = pixels << 2;
				pixels |= 0x01;
				LcdWrite(LCD_DATA, pixels);
			}
		
			LcdWrite(LCD_DATA, 0x01);
		}		
	}	
}

void LcdSetBacklight(uint8_t on)
{
	if (on)
	{
		PORTD |= (1<<LCD_PIN_BACKLIGHT);
	}
	else
	{
		PORTD &= ~(1<<LCD_PIN_BACKLIGHT);
	}
}

void LcdPowerSave(uint8_t powerSaveOn)
{
	if (powerSaveOn)
	{
		LcdWrite(LCD_CMD, 0x24); // put LCD in power-down mode	
		LcdSetBacklight(0);
	}
	else
	{
		LcdWrite(LCD_CMD, 0x20); // put LCD in normal mode
		LcdSetBacklight(1);
	}
}

void LcdClear(void)
{
	for (int index = 0; index < LCD_WIDTH * LCD_HEIGHT / 8; index++)
	{
		LcdWrite(LCD_DATA, 0x00);
	}
}

void LcdReset(void)
{
	lcd_vop = 0xB1;
	lcd_bias = 0x15;
	lcd_tempCoef = 0x04;

	for (int i=0; i<GRAPH_COUNT; i++)
	{
		graphYMin[i] = 0x7FFF;
		graphYMax[i] = 0x7FFF;
		graphDrawPoints[i] = 0;
	}
	
	DDRD |= (1<<LCD_PIN_SCE);
	DDRD |= (1<<LCD_PIN_RESET);
	DDRD |= (1<<LCD_PIN_DC);
	DDRD |= (1<<LCD_PIN_BACKLIGHT);
	DDRB |= (1<<LCD_PIN_SDIN);
	DDRB |= (1<<LCD_PIN_SCLK);
	
	PORTD &= ~(1<<LCD_PIN_RESET); 
	PORTD |= (1<<LCD_PIN_RESET);

	LcdWrite(LCD_CMD, 0x21); // LCD Extended Commands.
	LcdWrite(LCD_CMD, lcd_vop); // Set LCD Vop (Contrast). 
	LcdWrite(LCD_CMD, lcd_tempCoef); // Set Temp coefficent.
	LcdWrite(LCD_CMD, lcd_bias); // LCD bias mode 
	LcdWrite(LCD_CMD, 0x20);
	LcdWrite(LCD_CMD, 0x0C); // LCD in normal mode. 0x0d for inverse
}

void LcdDrawGraph2(uint8_t timescaleNumber, uint8_t type, uint8_t cursorPos, uint8_t showCursor)
{
	uint8_t xphase = GetTimescaleNextSampleIndex(timescaleNumber);
	Sample* pSample;
	
	// find the min and max values
	uint16_t rawValue;
	uint16_t minRawValue = 0xFFFF;
	uint16_t maxRawValue = 0;
	uint16_t rawCursorValue = 0xFFFF;
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
		minValue = graphYMin[GRAPH_TEMPERATURE] == 0x7FFF ? SAMPLE_TO_TEMPERATURE(minRawValue) : graphYMin[GRAPH_TEMPERATURE]*2;
		maxValue = graphYMax[GRAPH_TEMPERATURE] == 0x7FFF ? SAMPLE_TO_TEMPERATURE(maxRawValue) : graphYMax[GRAPH_TEMPERATURE]*2;
		graphCurrentYMin = minValue / 2;
		graphCurrentYMax = maxValue / 2;
		customAxes = graphYMin[GRAPH_TEMPERATURE] != 0x7FFF || graphYMax[GRAPH_TEMPERATURE] != 0x7FFF;
	}
	else if (type == GRAPH_PRESSURE)
	{
		cursorValue = SAMPLE_TO_PRESSURE(rawCursorValue);
		minValue = graphYMin[GRAPH_PRESSURE] == 0x7FFF ? SAMPLE_TO_PRESSURE(minRawValue) : graphYMin[GRAPH_PRESSURE]*2*33.86389f;  
		maxValue = graphYMax[GRAPH_PRESSURE] == 0x7FFF ? SAMPLE_TO_PRESSURE(maxRawValue) : graphYMax[GRAPH_PRESSURE]*2*33.86389f;
		graphCurrentYMin = minValue / (2*33.86389f);
		graphCurrentYMax = maxValue / (2*33.86389f);
		customAxes = graphYMin[GRAPH_PRESSURE] != 0x7FFF || graphYMax[GRAPH_PRESSURE] != 0x7FFF;
	}
	else
	{
		cursorValue = SAMPLE_TO_ALTITUDE(rawCursorValue);
		minValue = graphYMin[GRAPH_ALTITUDE] == 0x7FFF ? SAMPLE_TO_ALTITUDE(minRawValue) : graphYMin[GRAPH_ALTITUDE]/2;
		maxValue = graphYMax[GRAPH_ALTITUDE] == 0x7FFF ? SAMPLE_TO_ALTITUDE(maxRawValue) : graphYMax[GRAPH_ALTITUDE]/2;
		graphCurrentYMin = minValue * 2;
		graphCurrentYMax = maxValue * 2;
		customAxes = graphYMin[GRAPH_ALTITUDE] != 0x7FFF || graphYMax[GRAPH_ALTITUDE] != 0x7FFF;
	}
	
	if (rawCursorValue == 0xFFFF)
	{
		cursorValue = 0x7FFF;
	}

	const int graphLastPixel = 39; // graphHeight - 1
	const int minGraphSampleRange = (graphLastPixel+1)/2; // don't allow very small vertical ranges, to prevent super-jaggies
		
	if (minRawValue == 0xFFFF)
	{
		// no filled samples
		minValue = 0x7FFE;
		maxValue = 0x7FFF;
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
	
	LcdWrite(LCD_CMD, 0x22); // switch to vertical addressing
	
	// NOTE: assumption here that SAMPLES_PER_GRAPH equals LCD_WIDTH
	// start at the right side of the graph	
	xphase += LCD_WIDTH - 1;
	xphase %= LCD_WIDTH;	
	
	short sampleValue;
	uint8_t prevysample = 0xFF;
	
	uint8_t yMinLabelBuffer[4*5];
	uint8_t yMaxLabelBuffer[4*5];
	
	uint8_t yMinSize;
	uint8_t yMaxSize;
	
	LcdMakeGraphYAxis(type, minValue, maxValue, yMinLabelBuffer, yMaxLabelBuffer, &yMinSize, &yMaxSize);
		
	for (uint8_t x=LCD_WIDTH-1; x<LCD_WIDTH; x--)
	{
		LcdGoto(x,1);
		
		uint8_t ormask[5];
		uint8_t andmask[5];
		
		for (int y=0; y<5; y++)
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
				ormask[4] |= yMinLabelBuffer[x];
				andmask[4] = 0x03;
			}
		}
			
		pSample = GetSample(timescaleNumber, xphase);
		
		xphase--;
		if (xphase == 0xFF)
			xphase = LCD_WIDTH-1;
		
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
				
		uint8_t pixeldata[5];
		
		for (int y=0; y<5; y++)
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
		
		for (int y=0; y<5; y++)
		{
			LcdWrite(LCD_DATA, (pixeldata[y] & andmask[y]) | ormask[y]);				
		}		
	}	
	
	LcdWrite(LCD_CMD, 0x20); // switch to horizontal addressing
	
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
		
		for (uint8_t xclear = 1+strlen(str)*4; xclear < LCD_WIDTH; xclear++)
		{
			LcdWrite(LCD_DATA, 0x7F);
		}
		
		char valueStr[8];
		char unitsStr[3];
		strcpy(str, MakeSampleValueString(valueStr, type, cursorValue));
		strcat(str, MakeSampleUnitsString(unitsStr, type, cursorValue));
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
	
	// up-shift one pixels beyond the default
	char* characters = str;
	while (*characters)
	{
		unsigned short charbase = (*characters++ - 0x20) * 3;
		
		for (int index = 0; index < 3; index++)
		{
			uint8_t pixels = pgm_read_byte((unsigned char*)tiny_font + charbase + index);
			//LcdWrite(LCD_DATA, pixels);
			yMaxBuffer[(*yMaxSize)++] = pixels;
		}
		
		//LcdWrite(LCD_DATA, 0x00);
		yMaxBuffer[(*yMaxSize)++] = 0x00;
	}	
	
	//LcdGoto(0,5);
	*yMinSize = 0;
	MakeSampleValueString(str, type, yminlabel);
	
	// down-shift two pixels beyond the default
	characters = str;
	while (*characters)
	{
		unsigned short charbase = (*characters++ - 0x20) * 3;
		
		for (int index = 0; index < 3; index++)
		{
			uint8_t pixels = pgm_read_byte((unsigned char*)tiny_font + charbase + index);
			pixels = pixels << 3;
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
	uint8_t mcount = 0;
	uint8_t len = 0;
	char* p = characters;
	while (*p)
	{
		len++;
		if (*p == 'm')
			mcount++;
		p++;
	}
	
	LcdGoto(LCD_WIDTH-(len<<2)-2*mcount,0);
	LcdTinyString(characters, TEXT_INVERSE);
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
		LcdGoto(3,0);
		LcdWrite(LCD_DATA, 0x20);
	}
	for (uint8_t i=1; i<lpad; i++)
	{
		if (inverse)
			LcdTinyString(" ", TEXT_INVERSE);
		else
		{
			for (uint8_t j=0; j<4; j++)
			{
				LcdWrite(LCD_DATA, 0x20);
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
			for (uint8_t j=0; j<4; j++)
			{
				LcdWrite(LCD_DATA, 0x20);
			}
		}
	}
	LcdGoto(80,0);	
	LcdTinyString(inverse ? ">" : "]", inverse);	
}

// ----------- SPI
/*
byte SpiTransfer(uint8_t _data) 
{  
	SPDR = _data;  
	while (!(SPSR & (1<<SPIF))) {}  
	return SPDR;
}

void SPIAttachInterrupt() {  SPCR |= 1<<SPIE;}
void SPIDetachInterrupt() {  SPCR &= ~(1<<SPIE);}

void SpiBegin() {  
	// Set direction register for SCK and MOSI pin.  
	// MISO pin automatically overrides to INPUT.  
	// When the SS pin is set as OUTPUT, it can be used as  
	// a general purpose output port (it doesn't influence  
	// SPI operations).  
	pinMode(SCK, OUTPUT);  
	pinMode(MOSI, OUTPUT);  
	pinMode(SS, OUTPUT);    
	digitalWrite(SCK, 0);  
	digitalWrite(MOSI, 0);  
	digitalWrite(SS, 1);  
	// Warning: if the SS pin ever becomes a LOW INPUT then SPI   
	// automatically switches to Slave, so the data direction of   
	// the SS pin MUST be kept as OUTPUT.  
	SPCR |= (1<<MSTR);  
	SPCR |= (1<<SPE);
}
	
void SpiEnd() 
{  
	SPCR &= ~(1<<SPE);
}

void SpiSetBitOrder(bool lsbFirst)
{  
	if(lsbFirst) 
	{    
		SPCR |= (1<<DORD);  
	} 
	else 
	{    
		SPCR &= ~((1<<DORD));  
	}
}

void SpiSetDataMode(uint8_t mode)
{  
	SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}

void SpiSetClockDivider(uint8_t rate)
{  
	SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);  
	SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}
*/

#endif // NOKIA_LCD
