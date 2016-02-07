
#include "FreeRTOS.h"
#include "LEDHandler.h"
#include "serial.h"

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
 * Changes the color of the LED to reflect the given temperature value.
 * Temperatures less then `30` cause the LED to be blue; from `30` to `40`,
 * green; and from `40` up, red.
 * @param temperature The temperature value to use for changing the LED
 * color, in Celsius.
 */
void displayTemperatureInLED(int temperature)
{
    if (temperature < 30)
    {
        PORTE |= _BV(PORTE3);  // GREEN OFF
        PORTE &= ~_BV(PORTE5); // BLUE ON
        PORTH |= _BV(PORTH3);  // RED OFF
    }
    else if (temperature >= 30 && temperature < 40)
    {
        PORTE &= ~_BV(PORTE3); // GREEN ON
        PORTE |= _BV(PORTE5);  // BLUE OFF
        PORTH |= _BV(PORTH3);  // RED OFF
    }
    // If temperature is equals or higher than 40 degrees
    else
    {
        PORTE |= _BV(PORTE3);  // GREEN OFF
        PORTE |= _BV(PORTE5);  // BLUE OFF
        PORTH &= ~_BV(PORTH3); // RED ON
    }
}
