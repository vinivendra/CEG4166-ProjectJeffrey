
#include "temperatureHandler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i2cMultiMaster.h"
#include <avr/io.h>

/**
 *  getPixelTemperature function
 *  @param index - The index of pixel to read the temperature from
 *  @return int - The temperature value of the pixel in degrees celcius
 *
 *  Send a message to the thermal sensor using I2C to read from the register
 *  corresponding to the pixel index. Return the value that is read as an integer.
 */
int getPixelTemperature(int index)
{
    uint8_t pixelIndex = (uint8_t)index;
    pixelIndex += 2; // add 2 so we can index pixels beginning at 0

    // 1. write address (address of thermal sensor with 0 bit), read register
    // (pixel to read from 2-9)
    uint8_t msg1[2] = {0xD0, pixelIndex};
    // 2. read address (address of thermal sensor with 1 bit), space for storing
    // read data
    uint8_t msg2[2] = {0xD1, 0x00};

    I2C_Master_Start_Transceiver_With_Data(msg1, sizeof(msg1)); // first message
    I2C_Master_Start_Transceiver_With_Data(msg2, sizeof(msg2)); // second
                                                                // message
    uint8_t result[2];
    I2C_Master_Get_Data_From_Transceiver(result, sizeof(result)); // read result

    return result[1];
}

/**
 *  getPixelAverageTemperature function
 *  @params none
 *  @return void
 *
 *  Calls the getPixelTemperature method for each pixel, then calculates the
 *  average of those values and stores the result in the averageTemperature variable.
 *  The pixelTemperatures array is also populated with the values returned from the
 *  getPixelTemperature function calls.
 */
void getPixelAverageTemperature()
{
    int pixelTemperatureSum = 0;

    for (int pixel = 0; pixel < 8; pixel++)
    {
        int pixelTemp = getPixelTemperature(pixel);
        pixelTemperatures[pixel] = pixelTemp;
        pixelTemperatureSum += pixelTemp;
    }

    averageTemperature = (pixelTemperatureSum / 8);
}

/**
 *  getAmbientTemperature function
 *  @params none
 *  @return void
 *
 *  Send a message to the thermal sensor using I2C to read from register 1
 *  which contains the abient temperature from the thermal sensor. Then the
 *  result is stored in the ambiantTemperature variable.
 */
void getAmbientTemperature()
{
    // 1. write address (address of thermal sensor with 0 bit), read register
    // ambient pixel register
    uint8_t msg1[2] = {0xD0, 0x01};
    // 2. read address (address of thermal sensor with 1 bit), space for storing
    // read data
    uint8_t msg2[2] = {0xD1, 0x00};

    I2C_Master_Start_Transceiver_With_Data(msg1, sizeof(msg1)); // first message
    I2C_Master_Start_Transceiver_With_Data(msg2, sizeof(msg2)); // second
                                                                // message
    uint8_t result[2];
    I2C_Master_Get_Data_From_Transceiver(result, sizeof(result)); // read result

    ambiantTemperature = result[1];
}

/**
 *  updateTemperatures function
 *  @params none
 *  @return void
 *
 *  Calls the getPixelAverageTemperature and getAmbientTemperature functions.
 *  This updates the values stored in the ambiantTemperature and averageTemperature
 *  variables, and the individual pixel values stored in the pixelTemperatures array.
 */
void updateTemperatures()
{
    getPixelAverageTemperature();
    getAmbientTemperature();
}
