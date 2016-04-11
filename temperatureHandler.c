
#include "temperatureHandler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i2cMultiMaster.h"
#include <avr/io.h>

/**
 * Private array that store temperature information to be shared by the
 * temperature handlers.
 */
int pixelTemperatures[8];


void setupTemperature()
{
    I2C_Master_Initialise(0xBA);
}


/**
 * @param index The index of pixel to read the temperature from
 * @return int The temperature value of the pixel in degrees Celcius
 *
 * Send a message to the thermal sensor using `I2C` to read from the register
 * corresponding to the pixel index. Return the value that is read as an
 * integer.
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
 * @return int The temperature average value of the last 3 right pixel in degrees Celcius
 * This method uses the getPixelTemperature() to get the 3 last right pixel temperatures
 * Then calculated the average and returns it.
 */
int getRight3AvgTemperatures ()
{
	int pixelTemperatureSum = 0;

	    for (int pixel = 0; pixel < 3; pixel++)
	    {
	        int pixelTemp = getPixelTemperature(pixel);
	        pixelTemperatures[pixel] = pixelTemp;
	        pixelTemperatureSum += pixelTemp;
	    }

	    return pixelTemperatureSum / 3;
}

/**
 * @return int The temperature average value of the 4 center pixels in degrees Celcius
 * This method uses the getPixelTemperature() to get the 4 center pixel temperatures
 * Then calculated the average and returns it.
 */
int getCenter4AvgTemperatures ()
{
	int pixelTemperatureSum = 0;

	    for (int pixel = 2; pixel < 6; pixel++)
	    {
	        int pixelTemp = getPixelTemperature(pixel);
	        pixelTemperatures[pixel] = pixelTemp;
	        pixelTemperatureSum += pixelTemp;
	    }

	    return pixelTemperatureSum / 4;
}

/**
 * @return int The temperature average value of the last 3 left pixels in degrees Celcius
 * This method uses the getPixelTemperature() to get the last 3 left pixel temperatures
 * Then calculated the average and returns it.
 */
int getLeft3AvgTemperatures ()
{
	int pixelTemperatureSum = 0;

	    for (int pixel = 5; pixel < 8; pixel++)
	    {
	        int pixelTemp = getPixelTemperature(pixel);
	        pixelTemperatures[pixel] = pixelTemp;
	        pixelTemperatureSum += pixelTemp;
	    }

	    return pixelTemperatureSum / 3;
}

/**
 * @return int returns 1 if a temperature with 3 over the ambient temperature is found. Else return 0
 * This method uses the getPixelTemperature() to get all the pixel temperatures
 * If one of them is 3 over the ambient temperature, returns 1.
 */
int getSignificantTemperature() {
	int ambientTemperature = getAmbientTemperature();

    for (int pixel = 0; pixel < 8; pixel++)
    {
        int pixelTemp = getPixelTemperature(pixel);

        if(pixelTemp > ambientTemperature + 3) {
        	return 1;
        }
    }

    return 0;
}

/**
 * Calculates the average temperature of the 4 pixels on the left based on the
 * values in the shared `pixelTemperatures` array.
 *
 * @return The average temperature of the 4 pixels on the left.
 */
int getLeftAverageTemperature()
{
    int pixelTemperatureSum = 0;

    for (int pixel = 0; pixel < 4; pixel++)
    {
        int pixelTemp = getPixelTemperature(pixel);
        pixelTemperatures[pixel] = pixelTemp;
        pixelTemperatureSum += pixelTemp;
    }

    return pixelTemperatureSum / 4;
}

/**
 * Calculates the average temperature of the 4 pixels on the right based on the
 * values in the shared `pixelTemperatures` array.
 *
 * @return The average temperature of the 4 pixels on the right.
 */
int getRightAverageTemperature()
{
    int pixelTemperatureSum = 0;

    for (int pixel = 4; pixel < 8; pixel++)
    {
        int pixelTemp = getPixelTemperature(pixel);
        pixelTemperatures[pixel] = pixelTemp;
        pixelTemperatureSum += pixelTemp;
    }

    return pixelTemperatureSum / 4;
}

/**
 *  Reads the ambient temperature.
 *
 *  @return The current ambient temperature.
 */
int getAmbientTemperature()
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

    return result[1];
}

/**
 * Updates the temperature readings and stores the appropriate values in the
 * parameters.
 *
 * @param ambientTemperature      A pointer to an integer in which the ambient
 * temperature will be stored.
 * @param leftAverageTemperature  A pointer to an integer in which the average
 * left temperature will be stored.
 * @param rightAverageTemperature A pointer to an integer in which the average
 * right temperature will be stored.
 */
void updateTemperatures(int *ambientTemperature,
                        int *leftAverageTemperature,
                        int *rightAverageTemperature)
{
    *leftAverageTemperature = getLeftAverageTemperature();
    *rightAverageTemperature = getRightAverageTemperature();
    *ambientTemperature = getAmbientTemperature();
}
