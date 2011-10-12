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

#ifndef NOKLCD_H_
#define NOKLCD_H_

#include <inttypes.h>
#include "sampling.h"

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

#define LCD_CMD 0
#define LCD_DATA 1

#define TEXT_NORMAL 0
#define TEXT_INVERSE 1

extern volatile uint8_t lcd_vop;
extern volatile uint8_t lcd_bias;
extern volatile uint8_t lcd_tempCoef;
extern volatile int16_t graphCurrentYMin;
extern volatile int16_t graphCurrentYMax;
extern volatile int16_t graphYMin[GRAPH_COUNT];
extern volatile int16_t graphYMax[GRAPH_COUNT];
extern volatile uint8_t graphDrawPoints[GRAPH_COUNT];

void LcdReset(void);
void LcdClear(void);
void LcdPowerSave(uint8_t powerSaveOn);
void LcdGoto(int x, int y);
void LcdWrite(uint8_t dc, uint8_t data);
void LcdCharacter(char character);
void LcdString(char *characters);
void LcdTinyString(char *characters, uint8_t inverse);
void LcdTinyStringFramed(char *characters);
void LcdDrawHeading(char *characters, uint8_t inverse);
void LcdDrawGraph2(uint8_t timescaleNumber, uint8_t type, uint8_t cursorPos, uint8_t showCursor);
void LcdMakeGraphYAxis(uint8_t type, int yminlabel, int ymaxlabel, uint8_t* yMinBUffer, uint8_t* yMaxBuffer, uint8_t* yMinSize, uint8_t* yMaxSize);
void LcdDrawGraphLeftLegend(char *characters);
void LcdDrawGraphRightLegend(char *characters);


#endif /* NOKLCD_H_ */

#endif // NOKIA_LCD