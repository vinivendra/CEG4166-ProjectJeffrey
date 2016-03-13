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

#include "temperatureHandler.h"
#include "motionTask.h"
#include "decoderTask.h"
#include "LEDHandler.h"
#include "LCDHandler.h"

/**
 *  main function of the program here
 *  Creates the task and starts the scheduler
 */
int ambientTemperature, leftTemperature, rightTemperature;
float speed, distanceTravelled;


void vTaskTemperature(void *pvParameters);
void vTaskMoveChico(void *pvParameters);
void vTaskMoveThermoSensor(void *pvParameters);
void vTaskDecoder(void *pvParameters);
void vTaskLCD(void *pvParameters);

int main()
{
	xTaskCreate(vTaskTemperature, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskDecoder, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskLCD, (const portCHAR *)"", 256, NULL, 3, NULL);

    vTaskStartScheduler();
}

void vTaskTemperature(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	setupTemperature();
	setupLED();
	setupLCD();

	int period = 100;

	while (1)
	{
		updateTemperatures(&ambientTemperature, &leftTemperature, &rightTemperature);
		vTaskDelay((period / portTICK_PERIOD_MS));
	}

	shutdownLCD();
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
		vTaskDelay((2000 / portTICK_PERIOD_MS));

		motionBackward();
		displayRedLED();
		vTaskDelay((2000 / portTICK_PERIOD_MS));

		motionSpinLeft();
		displayBlueLED();
		vTaskDelay((2000 / portTICK_PERIOD_MS));

		motionSpinRight();
		vTaskDelay((2000 / portTICK_PERIOD_MS));

		motionStop();
		displayWhiteLED();
		vTaskDelay((2000 / portTICK_PERIOD_MS));
	}
}

void vTaskDecoder(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	int period = 100;

	while (1)
	{
		decoderTask(period, &speed, &distanceTravelled);

		vTaskDelay((period / portTICK_PERIOD_MS));
	}
}

void vTaskLCD(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	int period = 500;

	setupLCD();

	while (1)
	{
		writeToLCD(speed, distanceTravelled, ambientTemperature, leftTemperature, rightTemperature);

		vTaskDelay((period / portTICK_PERIOD_MS));
	}

	shutdownLCD();
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
