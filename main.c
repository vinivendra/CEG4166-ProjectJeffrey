
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
 
 static bool thermoSensorFlag = false; 

void vTaskMoveChico(void *pvParameters);
void vTaskMoveThermoSensor(void *pvParameters);
void vTaskDecoder(void *pvParameters);

int main()
{
	//xTaskCreate(vTaskTemperature, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
//	xTaskCreate(vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, NULL);
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
