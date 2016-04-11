#include "custom_timer.h"
#include "FreeRTOS.h"
#include "usart_serial.h"
#include <util/delay.h>

#define CHECK_BIT_STATUS(variable, position) 							( (variable) & (1 << (position)) )

/**
 * Gets the distance that chico has from an heat source.  It sends a sonar signal and calculates
 * the elapsed time between the send of the signal and the time when the signal comes back to chico.
 * It then converts the time into centimeters.
 *
 * Returns the distance in centimeters.
 */
int getDistance() {
	DDRA |= 0b00000001; //output
	PORTA &= 0b11111110;  // make sure pin A0 is LOW
	_delay_us(2);
	PORTA |= 0b00000001; // pulse HIGH
	_delay_us(5);
	PORTA &= 0b11111110; //pin A0 LOW
	DDRA &= 0b11111110; //input

	usart_fprintf_P(USART_0, PSTR("1\n"));

	unsigned long count = 0;

	while(!bit_is_set(PINA, PA0)) {
		_delay_us(5);
		count++;
		if(count > 200000){
			usart_fprintf_P(USART_0, PSTR("NOOOO CHICO!!!!!\n"));
			return -1;
		}
	}

	unsigned long start = time_in_microseconds();
	usart_fprintf_P(USART_0, PSTR("2\n"));
	loop_until_bit_is_clear(PINA, PA0);
	unsigned long end = time_in_microseconds();
	usart_fprintf_P(USART_0, PSTR("3\n"));
	long elapsedTime = end - start;

	int distance = elapsedTime/29/2;
	usart_fprintf_P(USART_0, PSTR("Dist: %lu \n"), end);
	return distance;
}
