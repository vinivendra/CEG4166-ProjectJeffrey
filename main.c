
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
//	xTaskCreate(vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, NULL);
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

	while (1)
	{
		updateTemperatures(&ambientTemperature, &leftTemperature, &rightTemperature);
	}

	shutdownLCD();
}

//void vTaskMoveThermoSensor(void *pvParameters)
//{
//
//	TickType_t xLastWakeTime;
//	xLastWakeTime = xTaskGetTickCount();
//
//	motionInit();
//
//	while(thermoSensorFlag) {
//		motionThermoSensorRight();
//		//vTaskDelayUntil(&xLastWakeTime, (200 / portTICK_PERIOD_MS));
//		motionThermoSensorLeft();
//	}
//	motionThermoSensorStop();
//
//}

void vTaskMoveChico(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	
	setupLED();

	motionInit();

	while (1)
	{
		motionForward();
//		thermoSensorFlag = true;
		displayGreenLED();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

		motionBackward();
		displayRedLED();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

//		thermoSensorFlag = false;
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

	int period = 100;

	while (1)
	{
		decoderTask(period, &speed, &distanceTravelled);

		vTaskDelayUntil(&xLastWakeTime, (period / portTICK_PERIOD_MS));
	}
}

void vTaskLCD(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	int period = 1000;

	setupLCD();

	while (1)
	{
		writeToLCD(speed, distanceTravelled, ambientTemperature, leftTemperature, rightTemperature);

		vTaskDelayUntil(&xLastWakeTime, (period / portTICK_PERIOD_MS));
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
