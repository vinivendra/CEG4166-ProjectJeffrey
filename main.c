
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


int main(void)
{
    xTaskCreate(temperatureTask, (const portCHAR *)"", 256, NULL, 3, NULL);

    vTaskStartScheduler();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, portCHAR *pcTaskName)
{

    DDRB |= _BV(DDB7);
    PORTB |= _BV(PORTB7); // main (red PB7) LED on. Mega main LED on and die.

    while (1)
        ;
}
