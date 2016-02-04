
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
 *  <Add description here>
 *
 *  @param pvParameters <#pvParameters description#>
 */
void temperatureTask(void *pvParameters)
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
