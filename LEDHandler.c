
#include "LEDHandler.h"

#include "serial.h"

void setupLED()
{
  // LED
  DDRE |= _BV(DDE5); // blue
  DDRE |= _BV(DDE3); // green
  DDRH |= _BV(DDH3); // red
}

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
    PORTH |= _BV(PORTH3);  // RED OFF*/
  }
  else
  {
    PORTE |= _BV(PORTE3);  // GREEN OFF
    PORTE |= _BV(PORTE5);  // BLUE ON
    PORTH &= ~_BV(PORTH3); // RED OFF
  }
}
