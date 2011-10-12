/*
  TwoWire.h - TWI/I2C library for Arduino & Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Wire_h
#define Wire_h

#include <inttypes.h>

#define BUFFER_LENGTH 8

void Wire_onRequestService(void);
void Wire_onReceiveService(uint8_t*, int);
  
void Wire_begin();
void Wire_begin_addr(uint8_t);
void Wire_beginTransmission(uint8_t);
uint8_t Wire_endTransmission(void);
uint8_t Wire_requestFrom(uint8_t, uint8_t);
void Wire_send(uint8_t);
void Wire_send_buffer(uint8_t*, uint8_t);
void Wire_send_string(char*);
uint8_t Wire_available(void);
uint8_t Wire_receive(void);
void Wire_onReceive( void (*)(int) );
void Wire_onRequest( void (*)(void) );

#endif

