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

/*
 * serial.c
 *
 * bit-bang 8N1 serial interface using the MOSI and MISO pins
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "serial.h"
#include "speaker.h"
#include "sampling.h"
#include "clock.h"
#include "hikea.h"

#define SERIAL_IN PB4
#define SERIAL_OUT PB3

#define CMD_VERSION '1'
#define CMD_GETGRAPHS '2'
#define CMD_GETSNAPSHOTS '3'

// determine how many clock cycles in one 26 microsecond bit time at 38400 bps
#ifdef LOGGER_CLASSIC	
// 1000000 MHz / 38400 bps = 26 cycles per bit. Select a timer compare value of 25 to get a 26 cycle period.
// #define BIT_TIME 25
	
/* Experimentally, a value of 25 always seems to be about 3-4% too slow, even with a calibrated oscillator. 
	Why? An off by one error somehow? Using 24 gets much better results in my tests. Is this just a side-effect of 
	my particular ATmega chip and battery voltage, and it will be too fast for someone else's Logger?
*/
#define BIT_TIME 24
#endif

#ifdef LOGGER_MINI	
// 8000000 MHz / 38400 bps = 208 cycles per bit. Select a timer compare value of 207 to get a 208 cycle period.
#define BIT_TIME 207
#endif	

uint8_t checksum;

void SerialInit()
{
	// enable the internal pull-up for serial in
	PORTB |= (1<<SERIAL_IN);	
}

void SerialBegin()
{
	// turn off the speaker beeps, so we can use the timers without interference
	SpeakerOff();
	
	// configure the timer for use with serial communication
	// setup timer 1 for note length of approximately 100 ms
	PRR &= ~(1<<PRTIM1); // turn on the timer hardware
    TCCR1A = 0; // clear timer on compare match (CTC) OCR0A mode, (WGM13 and WGM12 in TCCR1B) 
	TCCR1B = (1<<CS10) | (1<<WGM12) ; // prescale clock/1 (no prescale)
	
	// set the timer compare value. 
	// AVR datasheet section 16.3: To do a 16-bit write, the high byte must be written before the low byte.	
	OCR1AH = 0;
	OCR1AL = BIT_TIME;
	
    TIFR1 = (1 << OCF1A); // clear the timer 1 compare A match interrupt flag 
	
	PORTB |= (1<<SERIAL_OUT);
}

void SerialEnd()
{
	PRR |= (1<<PRTIM1);
}

void SerialDoCommand()
{
	SerialBegin();
	
	uint8_t commandPrefix[] = { 'C', 'M', 'D' };
	uint8_t index = 0;
	uint8_t readCount = 0;
	
	// wait for command prefix "CMD"
	while (readCount < 5)
	{
		uint8_t c = SerialReceiveByte();
		if (c == commandPrefix[index])
		{
			index++;
			if (index == 3)
			{
				uint8_t cmd = SerialReceiveByte();
				if (cmd != 0)
				{
					SerialDispatchCommand(cmd);
				}
				break;
			}
		}
		else if (c == commandPrefix[0])
		{
			index = 1;
		}
		else
		{
			index = 0;
		}
		
		readCount++;
	}	

	SerialEnd();
}

void SerialDispatchCommand(uint8_t cmd)
{
	// send "LOG" prefix
	SerialSendByte('L');
	SerialSendByte('O');
	SerialSendByte('G');
	
	// send the result of the command
	checksum = 0;
	switch (cmd)
	{
		case CMD_VERSION: // version
			for (uint8_t i=0; i<10; i++)
			{
				SerialSendByte(versionStr[i]);
			}
			break;
		
		case CMD_GETGRAPHS:
			SerialSendGraphs();
			break;
				
		case CMD_GETSNAPSHOTS:
			SerialSendSnapshots();
			break;
			
		default:
			// unrecognized command- do nothing
			break;
	}	
	
	// send the checksum
	SerialSendByte(checksum);
}

void SerialSendGraphs()
{	
	// graphs version number
	SerialSendByte(1);
	
	// number of graphs
	SerialSendByte(NUM_TIME_SCALES);
	
	// samples per graph
	SerialSendByte(SAMPLES_PER_GRAPH);
	
	// "now" time reference for the graphs: 
	// graph g is series of samples from the "now" time back to now - SAMPLES_PER_GRAPH * minutesPerSample[g]
	SerialSendByte(clock_second);
	SerialSendByte(clock_minute);
	SerialSendByte(clock_hour);
	SerialSendByte(clock_day);
	SerialSendByte(clock_month);
	SerialSendByte(clock_year); // year - 2000

	// for each graph g, send minutesPerSample[g] followed by the sample data
	for (uint8_t g=0; g<NUM_TIME_SCALES; g++)
	{
		SerialSendByte(minutesPerSample[g] >> 8); // hi byte - TODO: should be big or little endian?
		SerialSendByte(minutesPerSample[g] & 0xFF); // low byte
		
		uint8_t index = GetTimescaleNextSampleIndex(g); 
		for (uint8_t s=0; s<SAMPLES_PER_GRAPH; s++)
		{
			Sample* pSample = GetSample(g, index);
			index++;
			if (index == SAMPLES_PER_GRAPH)
			{
				index = 0;
			}
			
			for (uint8_t i=0; i<sizeof(Sample); i++)
			{
				SerialSendByte(*((uint8_t*)pSample + i));
			}
		}
	}				
}

void SerialSendSnapshots()
{	
	// snapshot version number
	SerialSendByte(1);
	
	// number of snapshots
	uint8_t numSnapshots = GetNumSnapshots();
	SerialSendByte(numSnapshots);

	int index = GetNewestSnapshotIndex() + 1 - numSnapshots;
	if (index < 0)
	{
		index += GetMaxSnapshots();
	}

	for (uint8_t s=0; s<numSnapshots; s++)
	{
		Snapshot* pSnapshot = GetSnapshot(index);
		index++;
		if (index == GetMaxSnapshots())
		{
			index = 0;
		}
							
		for (uint8_t i=0; i<sizeof(Snapshot); i++)
		{
			SerialSendByte(*((uint8_t*)pSnapshot + i));
		}
	}				
}

void SerialSendByte(uint8_t c)
{
	checksum ^= c;
		
	// mark
	PORTB |= (1<<SERIAL_OUT);
	TIFR1 = (1 << OCF1A); // clear the compare match flag
	loop_until_bit_is_set(TIFR1, OCF1A); // wait for the timer - might be just a fractional timer period
	TIFR1 = (1 << OCF1A); // clear the compare match flag
	loop_until_bit_is_set(TIFR1, OCF1A); // wait for the timer
	
	// start bit
	PORTB &= ~(1<<SERIAL_OUT);
	TIFR1 = (1 << OCF1A); // clear the compare match flag
	loop_until_bit_is_set(TIFR1, OCF1A); // wait for the timer
	
	// shift out the data byte, LSB first
	for (uint8_t i=0; i<8; i++)
	{
		if ((c & 0x1) != 0)
			PORTB |= (1<<SERIAL_OUT);
		else
			PORTB &= ~(1<<SERIAL_OUT);
			
		c = c >> 1;
		
		TIFR1 = (1 << OCF1A); // clear the compare match flag
		loop_until_bit_is_set(TIFR1, OCF1A); // wait for the timer
	} 	
	
	// no parity
	
	// stop bit
	PORTB |= (1<<SERIAL_OUT);
	TIFR1 = (1 << OCF1A); // clear the compare match flag
	loop_until_bit_is_set(TIFR1, OCF1A); // wait for the timer
}

uint8_t SerialReceiveByte()
{
	uint8_t c = 0;
	uint16_t try = 0;
	
	// wait for a mark followed by a start bit, up to 19200 bit times (half a second)
	while (bit_is_clear(PINB, SERIAL_IN))
	{
		// timer compare match flag is set?
		if (bit_is_set(TIFR1, OCF1A))
		{
			TIFR1 = (1 << OCF1A); // clear the compare match flag
			try++;
			if (try == 19200) 
				return 0;	
		}		
	}
	
	while (bit_is_set(PINB, SERIAL_IN))
	{
		// timer compare match flag is set?
		if (bit_is_set(TIFR1, OCF1A))
		{
			TIFR1 = (1 << OCF1A); // clear the compare match flag
			try++;
			if (try == 19200) 
				return 0;	
		}		
	}
	
	// delay half a bit time, minus	a few clock cycles to allow for the time needed to respond to the timer 
	__builtin_avr_delay_cycles(((BIT_TIME+1)>>1)-8);
	
	// reset the timer
	TCNT1H = 0;
	TCNT1L = 0;
	
	// shift in the data byte, LSB first
	for (uint8_t i=0; i<8; i++)
	{
		TIFR1 = (1 << OCF1A); // clear the compare match flag
		loop_until_bit_is_set(TIFR1, OCF1A); // wait for the timer
		
		if (bit_is_set(PINB, SERIAL_IN))
			c = (0x80 | (c >> 1));
		else
			c = (c >> 1);
	} 	
	
	return c;
}