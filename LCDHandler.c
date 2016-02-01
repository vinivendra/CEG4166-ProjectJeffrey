
#include "LCDHandler.h"

#include "serial.h"
#include "i2cMultiMaster.h"

extern xComPortHandle xSerial1Port;

void setupLCD()
{
    xSerial1Port = xSerialPortInitMinimal( USART1, 9600, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX); //  serial port: WantedBaud, TxQueueLength, RxQueueLength (8n1)
}

void displayTemperatureInLCD(int temperature)
{
    avrSerialxPrintf_P(&xSerial1Port, PSTR("%c"), 0xFE); // Refresh display
    avrSerialxPrintf_P(&xSerial1Port, PSTR("%c"), 0x01); // Clear display

    avrSerialxPrintf_P(&xSerial1Port, PSTR("%2d "), temperature); // Print the temperature

    xSerialFlush(&xSerial1Port); // Flush commands to LCD
}

void shutdownLCD()
{
    vSerialClose(&xSerial1Port); // Shutdown and free allocated memory
}

