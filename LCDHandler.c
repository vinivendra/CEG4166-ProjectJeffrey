
#include "LCDHandler.h"
#include "i2cMultiMaster.h"
#include "serial.h"
#include "temperatureHandler.h"

/**
 *  Port being used to communicate with the LCD screen
 */
extern xComPortHandle xSerial1Port;

/**
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

void writeToLCD(float speed, float distanceTravelled, int averageTemperature, int leftTemperature, int rightTemperature) {
    avrSerialxPrintf_P(&xSerial1Port, PSTR("%c"), 0xFE); // Refresh display
    avrSerialxPrintf_P(&xSerial1Port, PSTR("%c"), 0x01); // Clear display

    // Print the temperature
    avrSerialxPrintf_P(&xSerial1Port, PSTR("S: %2.2f D: %2.2f     "), speed, distanceTravelled);
    avrSerialxPrintf_P(&xSerial1Port, PSTR("A:%2d L:%2d R:%2d"), averageTemperature, leftTemperature, rightTemperature);

    xSerialFlush(&xSerial1Port); // Flush commands to LCD
}

/**
 * Shuts down the LCD screen and frees the allocated memory
 */
void shutdownLCD()
{
    vSerialClose(&xSerial1Port); // Shutdown and free allocated memory
}
