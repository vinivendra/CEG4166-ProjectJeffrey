
#include <stdio.h>
#include "LCDHandler.h"
#include "i2cMultiMaster.h"
#include "usart_serial.h"
#include "temperatureHandler.h"

/**
 *  Port being used to communicate with the LCD screen
 */
//extern xComPortHandle xSerial1Port;

int LCD_USART;

/**
 * Initialise the LCD screen
 * Set Baud rate to the default (9600bps)
 * Setup the necessary ports
 */
void setupLCD()
{
//    xSerial1Port = xSerialPortInitMinimal(USART1,
//                                          9600,
//                                          portSERIAL_BUFFER_TX,
//                                          portSERIAL_BUFFER_RX);

	LCD_USART = usartOpen(USART_1, 9600, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX);
}

/**
 *  Updates the LCD with the appropriate values.
 *
 *  @param speed              The speed at which the robot's moving, in `m/s`.
 *  @param distanceTravelled  The distance the robot's travelled so far, in `m`.
 *  @param averageTemperature The ambient temperature, in `Celsius`.
 *  @param leftTemperature    The average temperature of the left pixels, in
 * `Celsius`.
 *  @param rightTemperature   The average temperature of the right pixels, in
 * `Celsius`.
 */
void writeToLCD(float speed,
                float distanceTravelled,
                int ambientTemperature,
                int leftTemperature,
                int rightTemperature)
{
	usart_fprintf_P(LCD_USART, PSTR("%c"), 0xFE);
	usart_fprintf_P(LCD_USART, PSTR("%c"), 0x01);


	//usart_fprintf_P(LCD_USART, PSTR("Testing"));
	usart_fprintf_P(LCD_USART, PSTR("S: %2.2f D: %2.2f "), speed, distanceTravelled);
	usart_fprintf_P(LCD_USART, PSTR("A:%2d L:%2d R:%2d"), ambientTemperature, leftTemperature, rightTemperature);

//	usart_xflushRx(LCD_USART);
//    xSerialFlush(&xSerial1Port); // Flush commands to LCD
}

/**
 * Shuts down the LCD screen and frees the allocated memory.
 */
void shutdownLCD()
{
    usartClose(LCD_USART);
//	vSerialClose(&xSerial1Port); // Shutdown and free allocated memory
}
