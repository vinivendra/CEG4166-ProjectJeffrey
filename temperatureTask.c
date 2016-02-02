
#include "FreeRTOS.h"

#include "LCDHandler.h"
#include "LEDHandler.h"
#include "temperatureHandler.h"
#include "temperatureTask.h"

void temperatureTask(void *pvParameters)
{
  TickType_t xLastWakeTime;

  setupLED();
  setupLCD();

  xLastWakeTime = xTaskGetTickCount();

  while (1)
  {
    // Clear and refresh display of LCD
    int temperature = getAverageTemperature();

    displayTemperatureInLED(temperature);
    displayTemperatureInLCD(temperature);

    vTaskDelayUntil(&xLastWakeTime, (40 / portTICK_PERIOD_MS));
  }

  shutdownLCD();
}
