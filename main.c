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

#include "gainspan_gs1011m.h"
#include "usart_serial.h"
#include "web_server.h"

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


void initializeWifi() {
	taskENABLE_INTERRUPTS();
	int terminalUSART = usartOpen(USART_0, BAUD_RATE_115200, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX);
	int wifiUSART = usartOpen(USART_2, BAUD_RATE_9600, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX);
	gs_initialize_module(wifiUSART, BAUD_RATE_9600, terminalUSART, BAUD_RATE_115200);
	gs_set_wireless_ssid("chiiiiiiiiiiico");
	gs_activate_wireless_connection();
}

void initializeWebServer() {
	configure_web_page("Chico: The Robot", "! Control Interface !", HTML_DROPDOWN_LIST);
	add_element_choice('F', "Forward");
	add_element_choice('B', "Backward");
	start_web_server();
}

void vTaskCommandMode(void *pvParameters) {
	if(1) {
		motionForward();
	}else if(0) {
		motionBackward();
	}else if(0) {
		motionSpinLeft();
	}else if(0) {
		motionSpinRight();
	}else {
		motionStop();
	}
}

void vTaskAttachmentMode(void *pvParameters) {
	if(1) {
		motionForward();
	}else if(0) {
		motionBackward();
	}else {
		motionStop();
	}
}

void vTaskWebServer(void *pvParameters)
{
	TickType_t xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();

	while (1)
	{
		vTaskDelayUntil( &xLastWakeTime, (5500 / portTICK_PERIOD_MS));
		process_client_request();
		//char client_request = get_next_client_response();
	}
}


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

void vTaskControl(void *pvParameters) {
	if(1) {
		vTaskSuspend(vTaskAttachmentMode);
		vTaskResume(vTaskCommandMode);
	}else {
		vTaskSuspend(vTaskCommandMode);
		vTaskResume(vTaskAttachmentMode);
	}
}

/**
 * The program's starting point. This funtion schedules the task and then
 * surrenders control of the system to the scheduler.
 */
int main()
{
	initializeWifi();
	initializeWebServer();
	xTaskCreate(vTaskWebServer, (const portCHAR *)"", 1024, NULL, 3, NULL);
//    xTaskCreate(vTaskTemperature, (const portCHAR *)"", 256, NULL, 3, NULL);
//    xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
//    xTaskCreate(
//        vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, NULL);
//    xTaskCreate(vTaskDecoder, (const portCHAR *)"", 256, NULL, 3, NULL);
//    xTaskCreate(vTaskLCD, (const portCHAR *)"", 256, NULL, 3, NULL);

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
