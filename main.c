#include <avr/io.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "temperatureTask.h"
#include "motionTask.h"
#include "decoderTask.h"
#include "LEDHandler.h"

/**
 *  main function of the program here
 *  Creates the task and starts the scheduler
 */
 
void vTaskMoveChico(void *pvParameters);
void vTaskMoveThermoSensor(void *pvParameters);
void vTaskDecoder(void *pvParameters);

int main()
{
	//xTaskCreate(vTaskTemperature, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskDecoder, (const portCHAR *)"", 256, NULL, 3, NULL);
    vTaskStartScheduler();
}

void vTaskTemperature(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		temperatureTask();
	}
}

/**
 * Task to move the Thermo Sensor to the left and right while Chico is moving
 */
void vTaskMoveThermoSensor(void *pvParameters)
{
	const TickType_t xDelay = (50 / portTICK_PERIOD_MS);

	motionInit();

	while(1) {
		while(thermoSensorFlag){
			motionThermoSensor();
			vTaskDelay(xDelay);
		}
		motionThermoSensorStop();
	}
}

/**
 * Task to move Chico foward, backwards, spin right and spin left
 */
void vTaskMoveChico(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	
	setupLED();

	motionInit();

	while (1)
	{
		motionForward();
		displayGreenLED();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

		motionBackward();
		displayRedLED();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

		motionSpinLeft();
		displayBlueLED();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

		motionSpinRight();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

		motionStop();
		displayWhiteLED();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));
	}
}

void vTaskDecoder(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		decoderTask();
	}
}

/**
 *  This function needs to be here.
 *
 *  @param xTask
 *  @param pcTaskName
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, portCHAR *pcTaskName)
{
    while (1)
        ;
}