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

#ifndef SSD1306_H_
#define SSD1306_H_

#include <avr/io.h>
#include "sampling.h"

#define OLED_CONTROL_PORT PORTD
#define OLED_CONTROL_DDR DDRD
#define OLED_DATA_PORT PORTB
#define OLED_DATA_DDR DDRB
#define OLED_DC 7 // PD7
#define OLED_CS 4 // PD4
#define OLED_RESET 5 // PD5
#define OLED_CLK 5 // PB5
#define OLED_MOSI 3 // PB3


#define OLED_CMD 0
#define OLED_DATA 1

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_SETPAGESTART 0xB0
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

#define LCD_WIDTH SSD1306_WIDTH
#define LCD_HEIGHT SSD1306_HEIGHT

#define LCD_CMD 0
#define LCD_DATA 1

#define TEXT_NORMAL 0
#define TEXT_INVERSE 1

extern volatile uint8_t lcd_contrast;
extern volatile int16_t graphCurrentYMin;
extern volatile int16_t graphCurrentYMax;
extern volatile int16_t graphYMin[GRAPH_COUNT];
extern volatile int16_t graphYMax[GRAPH_COUNT];
extern volatile uint8_t graphDrawPoints[GRAPH_COUNT];

void ssd1306_init();
void ssd1306_clear();
void ssd1306_goto(uint8_t x, uint8_t y);
void ssd1306_write(uint8_t dc, uint8_t c);
void ssd1306_string(char *c, uint8_t inverse); 
void ssd1306_char(uint8_t c, uint8_t inverse);
 
void LcdReset(void);
void LcdClear(void);
void LcdPowerSave(uint8_t powerSaveOn);
#define LcdGoto(x, y) ssd1306_goto(x, y)
#define LcdWrite(dc, data) ssd1306_write(dc, data)
#define LcdCharacter(character, inverse) ssd1306_char(character, inverse)
#define LcdString(characters) ssd1306_string(characters, TEXT_NORMAL)
#define LcdTinyString(characters, inverse) ssd1306_string(characters, inverse)
#define LcdTinyStringFramed(characters) ssd1306_string(characters, TEXT_NORMAL)
void LcdDrawHeading(char *characters, uint8_t inverse);
void LcdDrawGraph2(uint8_t timescaleNumber, uint8_t type, uint8_t cursorPos, uint8_t showCursor);
void LcdMakeGraphYAxis(uint8_t type, int yminlabel, int ymaxlabel, uint8_t* yMinBUffer, uint8_t* yMaxBuffer, uint8_t* yMinSize, uint8_t* yMaxSize);
void LcdDrawGraphLeftLegend(char *characters);
void LcdDrawGraphRightLegend(char *characters); 

#endif /* SSD1306_H_ */

#endif // SSD1306_LCD