
#include "LCDHandler.h"
#include "i2cMultiMaster.h"
#include "serial.h"
#include "temperatureHandler.h"

/**
 *  Port being used to communicate with the LCD screen
 */
extern xComPortHandle xSerial1Port;

/**
 * setupLCD method 
 * @return void
 * @params none
 *
 * Initialise the LCD screen
 * Set Baud rate to the default (9600bps)
 * Setup the necessary ports 
 */
void setupLCD()
{
    xSerial1Port = xSerialPortInitMinimal(USART1,
                                          9600,
                                          portSERIAL_BUFFER_TX,
                                          portSERIAL_BUFFER_RX);
}

/**
 * displayTemperatureInLCD method
 * @return void
 * @params none
 *
 * Refresh the LCD screen, then make sure it is clear
 * Print the necessary temperatures to the LCD
 * Flush the commands to the LCD
 */
void displayTemperatureInLCD()
{
    avrSerialxPrintf_P(&xSerial1Port, PSTR("%c"), 0xFE); // Refresh display
    avrSerialxPrintf_P(&xSerial1Port, PSTR("%c"), 0x01); // Clear display

    // Print the temperature
    avrSerialxPrintf_P(&xSerial1Port, PSTR("Amb:%2d\r"), ambiantTemperature);
    for (int i = 0; i < 8; i++)
    {
        avrSerialxPrintf_P(&xSerial1Port, PSTR("%2d,"), pixelTemperatures[i]);
    }


    xSerialFlush(&xSerial1Port); // Flush commands to LCD
}

/**
 * shutDownLCD method
 * @return void
 * @params none
 *
 * Shuts down the LCD screen and frees the allocated memory 
 */
void shutdownLCD()
{
    vSerialClose(&xSerial1Port); // Shutdown and free allocated memory
}
