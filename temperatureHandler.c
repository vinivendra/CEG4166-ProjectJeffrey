
#include "temperatureHandler.h"

uint8_t readTemperature()
{
	usart_printf_P("Pretending to read temperature.");

	return 10;
}



#include "temperatureHandler.h"


#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <avr/io.h>
#include "i2cMultiMaster.h"


int getPixelTemperature(int index)
{
    uint8_t pixelIndex = (uint8_t)index;
    pixelIndex += 2; //add 2 so we can index pixels beginning at 0

    uint8_t msg1[2] = {0xD0, pixelIndex}; //1. write address (address of thermal sensor with 0 bit), read register (pixel to read from 2-9)
    uint8_t msg2[2] = {0xD1, 0x00}; //2. read address (address of thermal sensor with 1 bit), space for storing read data

    I2C_Master_Start_Transceiver_With_Data(msg1, sizeof(msg1)); //first message
    I2C_Master_Start_Transceiver_With_Data(msg2, sizeof(msg2)); //second message
    uint8_t result[2];
    I2C_Master_Get_Data_From_Transceiver(result, sizeof(result)); //read result

    return result[1];
}

int getAverageTemperature()
{

    int pixelTemperatureSum = 0;

    for(int pixel=0;pixel<8;pixel++)
    {
        pixelTemperatureSum += getPixelTemperature(pixel);
    }

    return pixelTemperatureSum/8;
}
