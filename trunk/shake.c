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

#ifdef SHAKE_SENSOR

#include <avr/io.h>
#include <avr/interrupt.h>
#include "shake.h"
#include "speaker.h"

// port C
#define SHAKE_PORT PORTC
#define SHAKE_DDR DDRC
#define SHAKE_PIN_IN PC0
#define SHAKE_PIN_OUT PC1

#define SHAKE_TRIP_START_TIMEOUT 5
#define SHAKE_TRIP_STOP_TIMEOUT 5

static volatile uint8_t ignoreOneShake = 0;
static volatile uint8_t isMoving = 0;
static volatile uint8_t shakeDetected = 0;
static volatile uint8_t consecutiveMinutesShaking = 0;
static volatile uint8_t consecutiveMinutesStopped = 0;
static volatile uint16_t tripMinutes = 0;

void ShakeInit()
{
	// enable the internal pull-up resistors for shake-in	
	SHAKE_PORT |= (1<<SHAKE_PIN_IN); 
	
	// set shake-out as output
	SHAKE_DDR |= (1<<SHAKE_PIN_OUT);
		
	// set the pin change interrupt mask		
	PCMSK1 |= (1<<PCINT8);
	
	ShakeEnable(1);
}

void ShakeEnable(uint8_t enable)
{
	if (enable)
	{
		ignoreOneShake = 1;
		
		// drive 0
		SHAKE_PORT &= ~(1<<SHAKE_PIN_OUT);
		// clear interrupt flag
		PCIFR &= ~(1<<PCIF1);
		// enable the pin change interrupts for shake sensor	
		PCICR |= (1<<PCIE1);
	}
	else
	{
		// disable the pin change interrupts for shake sensor		
		PCICR &= ~(1<<PCIE1);
		// drive 1 - uses no power regardless of sensor state
		SHAKE_PORT |= (1<<SHAKE_PIN_OUT);			
	}
}

uint16_t ShakeGetTripTime()
{
	return tripMinutes;
}

uint8_t ShakeIsMoving()
{
	return isMoving;
}

void ShakeUpdate()
{
	// called once per minute
	if (shakeDetected)
	{
		consecutiveMinutesShaking++;
		consecutiveMinutesStopped = 0;
	}
	else
	{
		consecutiveMinutesShaking = 0;
		consecutiveMinutesStopped++;
	}
	
	if (!isMoving)
	{
		if (consecutiveMinutesShaking == SHAKE_TRIP_START_TIMEOUT)
		{
			isMoving = 1;
			tripMinutes = consecutiveMinutesShaking;
			SpeakerBeep(BEEP_TIMER_START);
		}
	}
	else
	{
		// isMoving
		if (consecutiveMinutesStopped == SHAKE_TRIP_STOP_TIMEOUT)
		{
			isMoving = 0;
			SpeakerBeep(BEEP_TIMER_END);
		}
		else
		{
			tripMinutes++;
		}
	}
	
	ShakeEnable(1); // re-enable sensor at the start of each minute
	shakeDetected = 0;
}

// shake sensor state change interrupt
ISR(PCINT1_vect) 
{ 
	if (ignoreOneShake)
	{
		ignoreOneShake = 0;
		return;
	}
	
	shakeDetected = 1;
	
	// disable sensor until explicitly re-enabled, to save power
	ShakeEnable(0);
}	

#endif
