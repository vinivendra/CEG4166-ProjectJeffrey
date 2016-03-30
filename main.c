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

#include "LCDHandler.h"
#include "LEDHandler.h"
#include "decoderTask.h"
#include "motionTask.h"
#include "temperatureHandler.h"

/// Global variable that stores the ambient temperature. Created for sharing
/// information between tasks.
int ambientTemperature;
/// Global variable that stores the left pixels' temperature. Created for
/// sharing information between tasks.
int leftTemperature;
/// Global variable that stores the right pixels' temperature. Created for
/// sharing information between tasks.
int rightTemperature;
/// Global variable that stores the robot's speed. Created for sharing
/// information between tasks.
float speed;
/// Global variable that stores the distance travelled so far. Created for
/// sharing information between tasks.
float distanceTravelled;

/**
 * The task responsible for reading temperature values and updating them
 * whenever possible in the `ambientTemperature`, `rightTemperature` and
 * `leftTemperature` variables.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
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
        updateTemperatures(&ambientTemperature,
                           &leftTemperature,
                           &rightTemperature);
        vTaskDelay((period / portTICK_PERIOD_MS));
    }

    shutdownLCD();
}

/**
 * The task responsible for moving the thermoSensor.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
void vTaskMoveThermoSensor(void *pvParameters)
{
    const TickType_t xDelay = (50 / portTICK_PERIOD_MS);

    motionInit();

    while (1)
    {
        while (thermoSensorFlag)
        {
            motionThermoSensor();
            vTaskDelay(xDelay);
        }
        motionThermoSensorStop();
    }
}

/**
 * The task responsible for moving the robot.
 *
 * @param pvParameters Used only for function definition compatibility.
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

/**
 * The task responsible for reading the robot's movement information and
 * updating it in the `speed` and `distanceTravelled` variables.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
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

/**
 * The task responsible for updating the temperature and movement information
 * periodically in the LCD.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
void vTaskLCD(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    int period = 500;

    setupLCD();

    while (1)
    {
        writeToLCD(speed,
                   distanceTravelled,
                   ambientTemperature,
                   leftTemperature,
                   rightTemperature);

        vTaskDelay((period / portTICK_PERIOD_MS));
    }

    shutdownLCD();
}

/**
 * The program's starting point. This funtion schedules the task and then
 * surrenders control of the system to the scheduler.
 */
int main()
{
    xTaskCreate(vTaskTemperature, (const portCHAR *)"", 256, NULL, 3, NULL);
    xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
    xTaskCreate(
        vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, NULL);
    xTaskCreate(vTaskDecoder, (const portCHAR *)"", 256, NULL, 3, NULL);
    xTaskCreate(vTaskLCD, (const portCHAR *)"", 256, NULL, 3, NULL);

    vTaskStartScheduler();
}

/**
 *  Empty function, used only for compatibility with the OS.
 *
 *  @param xTask Parameter present for compatibility in the function definition.
 *  @param pcTaskName Parameter present for compatibility in the function
 * definition.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, portCHAR *pcTaskName)
{
    while (1)
        ;
}
