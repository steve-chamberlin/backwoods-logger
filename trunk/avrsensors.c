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
#include <util/delay.h>

long readVcc() 
{   
	PRR &= ~(1<<PRADC); // power ADC
	ADCSRA |= (1<<ADEN); // enable ADC
	
	long result;   
	
#ifdef LOGGER_CLASSIC	
	// Read 1.1V reference against AVcc   
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);   
	_delay_ms(2); // Wait for Vref to settle   
	ADCSRA |= _BV(ADSC); // Convert   
	while (bit_is_set(ADCSRA,ADSC));   
	result = ADCL;   
	result |= ADCH<<8;   
	result = 1126400L / result; // Back-calculate AVcc in mV   
#endif

#ifdef LOGGER_MINI
	// Read ADC6 reference against AVcc   
	ADMUX = _BV(REFS0) | _BV(MUX2) | _BV(MUX1);   
	_delay_ms(2); // Wait for Vref to settle   
	ADCSRA |= _BV(ADSC); // Convert   
	while (bit_is_set(ADCSRA,ADSC));   
	result = ADCL;   
	result |= ADCH<<8;   
	result = result * 3260 / 1023; // convert to mV  	
#endif
	
	ADCSRA &= ~(1<<ADEN); // disable ADC
	PRR |= (1<<PRADC); // unpower ADC
		
	return result; 
} 

long readTemp() 
{     
	PRR &= ~(1<<PRADC); // power ADC
	ADCSRA |= (1<<ADEN); // enable ADC
	
	long result; 	
	// Read temperature sensor against 1.1V reference   
	ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);   
	_delay_ms(2); // Wait for Vref to settle   
	ADCSRA |= _BV(ADSC); // Convert   
	while (bit_is_set(ADCSRA,ADSC));   
	result = ADCL;   
	result |= ADCH<<8;   
	result = (result - 125) * 1075;   
	
	ADCSRA &= ~(1<<ADEN); // disable ADC
	PRR |= (1<<PRADC); // unpower ADC
		
	return result; 
} 
