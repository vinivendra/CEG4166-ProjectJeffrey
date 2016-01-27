
#include "FreeRTOS.h"

#include "temperatureTask.h"

#include "LEDHandler.h"
#include "LCDHandler.h"
#include "temperatureHandler.h"

void TemperatureTask(void *pvParameters)
{
	TickType_t xLastWakeTime = xTaskGetTickCount();

	while(1)
	{
		uint8_t temperature = readTemperature();

		displayTemperatureInLCD(temperature);
		displayTemperatureInLED(temperature);

		vTaskDelayUntil( &xLastWakeTime, ( 1000 / portTICK_PERIOD_MS ) );
	}

}
