
#include "LEDHandler.h"

void displayTemperatureInLED(uint8_t temperature)
{
	usart_printf_P("Pretending to display temperature in LED: %d", temperature);
}
