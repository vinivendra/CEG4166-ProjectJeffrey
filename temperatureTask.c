
#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "i2cMultiMaster.h"
#include "queue.h"
#include "semphr.h"
#include "serial.h"
#include "task.h"

/* Local files */
#include "LCDHandler.h"
#include "LEDHandler.h"
#include "temperatureHandler.h"
#include "temperatureTask.h"

/**
 * Function to be given to the program's scheduler to handle monitoring thermal
 * sensors and displaying temperature in the LCD and LED displays. This task
 * handles all relevant setup and shutdown procedures as well as calling for
 * regular updates. The task sleeps for 1 second between each update in order to
 * avoid flickering in the output and give the user time to read the displayed
 * information.
 *
 * @param pvParameters This parameter is present only so the function conforms
 * with the required signature. It is ignored.
 */
void temperatureTask()
{
    I2C_Master_Initialise(0xBA);
    TickType_t xLastWakeTime;

    setupLED();
    setupLCD();

    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // Clear and refresh display of LCD
        // int temperature = getAverageTemperature();
        updateTemperatures();

        displayTemperatureInLED(averageTemperature);
        displayTemperatureInLCD();

        vTaskDelayUntil(&xLastWakeTime, (1000 / portTICK_PERIOD_MS));
    }

    shutdownLCD();
}
