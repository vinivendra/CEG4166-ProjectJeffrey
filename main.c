
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

/**
 *  main function of the program here
 *  Creates the task and starts the scheduler
 */

void vTaskMoveChico(void *pvParameters);

int main()
{
	//xTaskCreate(vTaskTemperature, (const portCHAR *)"", 256, NULL, 3, NULL);
	xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
	//xTaskCreate(vTaskDecoder, (const portCHAR *)"", 256, NULL, 3, NULL);
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

void vTaskMoveChico(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	motionInit();

	while (1)
	{
		motionForward();
		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));

//		motionBackward();
//		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));
//
//		motionSpinLeft();
//		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));
//
//		motionSpinRight();
//		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));
//
//		motionStop();
//		vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));
	}
}

void vTaskDecoder(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		decoder();
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
