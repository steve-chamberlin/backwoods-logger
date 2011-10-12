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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "speaker.h"

#define SPEAKER_PIN PD6
#define SPEAKER_GROUND_PIN PD0

uint8_t speaker_beepType;
volatile uint8_t speaker_toneCount;
volatile uint8_t speaker_in_use = 0;
volatile uint8_t soundEnable = 1;

void SpeakerInit()
{
	// set the speaker as output
	DDRD |= (1<<SPEAKER_PIN);	
	DDRD |= (1<<SPEAKER_GROUND_PIN);
	SpeakerOff();
}

void SpeakerOff()
{
	speaker_in_use = 0;
	PORTD &= ~((1<<SPEAKER_PIN) | (1<<SPEAKER_GROUND_PIN));
    TCCR0A = 0; // disable OCR0A pin 	
    TIMSK1 = 0; // disable timer 1 interrupts
	TIFR1 = 0; // clear timer 1 interrupt flags 
	PRR |= (1<<PRTIM1) | (1<<PRTIM0);
}

#ifdef LOGGER_CLASSIC
#define BEEP_DURATION 100
#define BEEP_DURATION_LONG 300
#endif
#ifdef LOGGER_MINI
#define BEEP_DURATION 800
#define BEEP_DURATION_LONG 2400
#endif

void SpeakerNextTone()
{
	uint8_t tone0 = 64;
	
	if (speaker_toneCount == 0)
	{
		if (speaker_beepType == BEEP_NEXT || speaker_beepType == BEEP_ENTER || speaker_beepType == BEEP_TIMER_END)
		{
			tone0 = 64;
		}
		else
		{
			tone0 = 96;
		}
	}	
	else if (speaker_toneCount == 1)
	{
		if (speaker_beepType == BEEP_NEXT)
		{
			SpeakerOff();
			return;
		}
		else if (speaker_beepType == BEEP_EXIT || speaker_beepType == BEEP_TIMER_START)
		{
			tone0 = 64;
		}
		else
		{
			tone0 = 96;
		}
	}
	else if (speaker_toneCount == 2 && speaker_beepType == BEEP_TIMER_START)
	{
		tone0 = 64;
	}
	else if (speaker_toneCount == 2 && speaker_beepType == BEEP_TIMER_END)
	{
		tone0 = 96;
	}
	else
	{
		SpeakerOff();
		return;
	}
	
	// setup timer 0 for tone PWM
	PRR &= ~(1<<PRTIM0);
    TCCR0A = (1<<COM0A0) | (1<<WGM01); // Toggle OC0A on Compare Match, CTC OCR0A mode (WGM02 in TCCR0B)
#ifdef LOGGER_CLASSIC	
	TCCR0B = (1<<CS01) ; // clk/8 prescale
#endif
#ifdef LOGGER_MINI	
	TCCR0B = (1<<CS01) | (1<<CS00); // clk/64 prescale
#endif
	OCR0A = tone0;
	TCNT0 = 0;	

	// setup timer 1 for note length of approximately 100 ms
	PRR &= ~(1<<PRTIM1);
    TCCR1A = 0; // CTC OCR0A mode, (WGM13 and WGM12 in TCCR1B) 
	TCCR1B = (1<<CS12) | (1<<CS10) | (1<<WGM12) ; // prescale clock/1024
	// datasheet section 16.3: To do a 16-bit write, the high byte must be written before the low byte.
		
	if (speaker_beepType == BEEP_TIMER_START || speaker_beepType == BEEP_TIMER_END)
	{
		OCR1AH = (BEEP_DURATION_LONG >> 8);
		OCR1AL = (BEEP_DURATION_LONG & 0xFF);
	}
	else
	{
		OCR1AH = (BEEP_DURATION >> 8);
		OCR1AL = (BEEP_DURATION & 0xFF);		
	}
    TIFR1 = (1 << OCF1A); // clear the timer 1 compare A match interrupt flag 
    TIMSK1 = (1 << OCIE1A); // enable timer 1 compare A match interrupt	
}

void SpeakerBeep(uint8_t beepType)
{
	if (!soundEnable)
		return;
		
	speaker_beepType = beepType;
	speaker_toneCount = 0;
	speaker_in_use = 1;
	
	SpeakerNextTone();
}

ISR(TIMER1_COMPA_vect)
{
	speaker_toneCount++;
	SpeakerNextTone();	
}