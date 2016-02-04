
#include "LCDHandler.h"
#include "i2cMultiMaster.h"
#include "serial.h"
#include "temperatureHandler.h"

/**
 *  <Add description here>
 */
extern xComPortHandle xSerial1Port;

/**
 *  <Add description here>
 */
void setupLCD()
{
    xSerial1Port = xSerialPortInitMinimal(USART1,
                                          9600,
                                          portSERIAL_BUFFER_TX,
                                          portSERIAL_BUFFER_RX);
}

/**
 *  <Add description here>
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
 *  <Add description here>
 */
void shutdownLCD()
{
    vSerialClose(&xSerial1Port); // Shutdown and free allocated memory
}
