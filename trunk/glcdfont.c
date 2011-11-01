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

#include <avr/io.h>
#include <avr/pgmspace.h> 
 
#ifndef FONT5X7_H
#define FONT5X7_H

// standard ascii 5x7 font

static unsigned char  font[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, // 20  
	0x00, 0x00, 0x5F, 0x00, 0x00, // 21 !
	0x00, 0x07, 0x00, 0x07, 0x00, // 22 "
	0x14, 0x7F, 0x14, 0x7F, 0x14, // 23 #
	0x24, 0x2A, 0x7F, 0x2A, 0x12, // 24 $
	0x23, 0x13, 0x08, 0x64, 0x62, // 25 %
	0x36, 0x49, 0x56, 0x20, 0x50, // 26 &
	0x00, 0x06, 0x05, 0x03, 0x00, // 27 ' -> degree
	0x00, 0x1C, 0x22, 0x41, 0x00, // 28 (
	0x00, 0x41, 0x22, 0x1C, 0x00, // 29 )
	0x2A, 0x1C, 0x7F, 0x1C, 0x2A, // 2a *
	0x08, 0x08, 0x3E, 0x08, 0x08, // 2b +
	0x00, 0x80, 0x70, 0x30, 0x00, // 2c ,
	0x08, 0x08, 0x08, 0x08, 0x08, // 2d -
	0x00, 0x00, 0x60, 0x60, 0x00, // 2e .
	0x20, 0x10, 0x08, 0x04, 0x02, // 2f /
	0x3E, 0x51, 0x49, 0x45, 0x3E, // 30 0
	0x00, 0x42, 0x7F, 0x40, 0x00, // 31 1
	0x72, 0x49, 0x49, 0x49, 0x46, // 32 2
	0x21, 0x41, 0x49, 0x4D, 0x33, // 33 3
	0x18, 0x14, 0x12, 0x7F, 0x10, // 34 4
	0x27, 0x45, 0x45, 0x45, 0x39, // 35 5
	0x3C, 0x4A, 0x49, 0x49, 0x31, // 36 6
	0x41, 0x21, 0x11, 0x09, 0x07, // 37 7
	0x36, 0x49, 0x49, 0x49, 0x36, // 38 8
	0x46, 0x49, 0x49, 0x29, 0x1E, // 39 9
	0x00, 0x00, 0x14, 0x00, 0x00, // 3a :
	0x10, 0x30, 0x70, 0x30, 0x10, // 3b ;	-> down arrow
	0x00, 0x08, 0x1C, 0x3E, 0x7F, // 3c <
	0x14, 0x14, 0x14, 0x14, 0x14, // 3d =
	0x00, 0x7F, 0x3E, 0x1C, 0x08, // 3e >
	0x02, 0x01, 0x59, 0x09, 0x06, // 3f ?
	0x3E, 0x41, 0x5D, 0x59, 0x4E, // 40 @
	0x7C, 0x12, 0x11, 0x12, 0x7C, // 41 A
	0x7F, 0x49, 0x49, 0x49, 0x36, // 42 B
	0x3E, 0x41, 0x41, 0x41, 0x22, // 43 C
	0x7F, 0x41, 0x41, 0x41, 0x3E, // 44 D
	0x7F, 0x49, 0x49, 0x49, 0x41, // 45 E
	0x7F, 0x09, 0x09, 0x09, 0x01, // 46 F
	0x3E, 0x41, 0x41, 0x51, 0x73, // 47 G
	0x7F, 0x08, 0x08, 0x08, 0x7F, // 48 H
	0x00, 0x41, 0x7F, 0x41, 0x00, // 49 I
	0x20, 0x40, 0x41, 0x3F, 0x01, // 4a J
	0x7F, 0x08, 0x14, 0x22, 0x41, // 4b K
	0x7F, 0x40, 0x40, 0x40, 0x40, // 4c L
	0x7F, 0x02, 0x1C, 0x02, 0x7F, // 4d M
	0x7F, 0x04, 0x08, 0x10, 0x7F, // 4e N
	0x3E, 0x41, 0x41, 0x41, 0x3E, // 4f O
	0x7F, 0x09, 0x09, 0x09, 0x06, // 50 P
	0x3E, 0x41, 0x51, 0x21, 0x5E, // 51 Q
	0x7F, 0x09, 0x19, 0x29, 0x46, // 52 R
	0x26, 0x49, 0x49, 0x49, 0x32, // 53 S
	0x01, 0x01, 0x7F, 0x01, 0x01, // 54 T
	0x3F, 0x40, 0x40, 0x40, 0x3F, // 55 U
	0x1F, 0x20, 0x40, 0x20, 0x1F, // 56 V
	0x3F, 0x40, 0x38, 0x40, 0x3F, // 57 W
	0x63, 0x14, 0x08, 0x14, 0x63, // 58 X
	0x03, 0x04, 0x78, 0x04, 0x03, // 59 Y
	0x61, 0x59, 0x49, 0x4D, 0x43, // 5a Z
	0x00, 0x7F, 0x41, 0x41, 0x41, // 5b [
	0x02, 0x04, 0x08, 0x10, 0x20, // 5c backslash
	0x00, 0x41, 0x41, 0x41, 0x7F, // 5d ]
	0x04, 0x06, 0x07, 0x06, 0x04, // 5e ^
	0x40, 0x40, 0x40, 0x40, 0x40, // 5f _
	0x00, 0x06, 0x05, 0x03, 0x00, // 60 ` -> degree
	0x20, 0x54, 0x54, 0x78, 0x40, // 61 a
	0x7F, 0x28, 0x44, 0x44, 0x38, // 62 b
	0x38, 0x44, 0x44, 0x44, 0x28, // 63 c
	0x38, 0x44, 0x44, 0x28, 0x7F, // 64 d
	0x38, 0x54, 0x54, 0x54, 0x18, // 65 e
	0x00, 0x08, 0x7E, 0x09, 0x02, // 66 f
	0x18, 0xA4, 0xA4, 0x9C, 0x78, // 67 g
	0x7F, 0x08, 0x04, 0x04, 0x78, // 68 h
	0x00, 0x44, 0x7D, 0x40, 0x00, // 69 i
	0x20, 0x40, 0x40, 0x3D, 0x00, // 6a j
	0x7F, 0x10, 0x28, 0x44, 0x00, // 6b k
	0x00, 0x41, 0x7F, 0x40, 0x00, // 6c l
	0x7C, 0x04, 0x78, 0x04, 0x78, // 6d m
	0x7C, 0x08, 0x04, 0x04, 0x78, // 6e n
	0x38, 0x44, 0x44, 0x44, 0x38, // 6f o
	0xFC, 0x18, 0x24, 0x24, 0x18, // 70 p
	0x18, 0x24, 0x24, 0x18, 0xFC, // 71 q
	0x7C, 0x08, 0x04, 0x04, 0x08, // 72 r
	0x48, 0x54, 0x54, 0x54, 0x24, // 73 s
	0x04, 0x04, 0x3F, 0x44, 0x24, // 74 t
	0x3C, 0x40, 0x40, 0x20, 0x7C, // 75 u
	0x1C, 0x20, 0x40, 0x20, 0x1C, // 76 v
	0x3C, 0x40, 0x30, 0x40, 0x3C, // 77 w
	0x44, 0x28, 0x10, 0x28, 0x44, // 78 x
	0x4C, 0x90, 0x90, 0x90, 0x7C, // 79 y
	0x44, 0x64, 0x54, 0x4C, 0x44, // 7a z
};
#endif

#endif // SSD1306_LCD