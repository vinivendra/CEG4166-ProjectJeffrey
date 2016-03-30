#include "FreeRTOS.h"
#include "LEDHandler.h"

/**
 * Loads the micro-controller’s registers with associated
 * input/output ports for each LED. Each bit in the `DDRx` registers
 * can be either `1` or `0` : a bit of `DDRx` at `1` configures the pin
 * as output and putting it at `0` will configure the pin as an input.
 */
void setupLED()
{
    // LED
    DDRE |= _BV(DDE5); // blue
    DDRE |= _BV(DDE3); // green
    DDRH |= _BV(DDH3); // red
}

/**
 * Change LED color to green
 */
void displayGreenLED() {
	PORTE &= ~_BV(PORTE3); // GREEN ON
    PORTE |= _BV(PORTE5);  // BLUE OFF
    PORTH |= _BV(PORTH3);  // RED OFF
}

/**
 * Change LED color to Red
 */
void displayRedLED() {
	PORTE |= _BV(PORTE3);  // GREEN OFF
    PORTE |= _BV(PORTE5);  // BLUE OFF
    PORTH &= ~_BV(PORTH3); // RED ON	
}

/**
 * Change LED color to Blue
 */
void displayBlueLED() {
	PORTE |= _BV(PORTE3);  // GREEN OFF
	PORTE &= ~_BV(PORTE5); // BLUE ON
    PORTH |= _BV(PORTH3);  // RED OFF	
}

/**
 * Change LED color to white
 * All colors must be on to obtain the color white
 */
void displayWhiteLED() {
	PORTE &= ~_BV(PORTE3);  // GREEN ON
	PORTE &= ~_BV(PORTE5); // BLUE ON
    PORTH &= ~_BV(PORTH3);  // RED ON
}

/**
 * Turns off the LED
 */
void displayNoLED() {
	PORTE |= _BV(PORTE3);  // GREEN OFF
	PORTE |= _BV(PORTE5); // BLUE OFF
    PORTH |= _BV(PORTH3);  // RED OFF	
}

