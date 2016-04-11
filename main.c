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

#include "wireless_interface.h"
#include "usart_serial.h"
#include "distanceHandler.h"
#include "custom_timer.h"

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

//
char clientRequest = 'F';
char previousClientRequest = 'A';

TaskHandle_t xCommandHandler;
TaskHandle_t xAttachmentHandler;
TaskHandle_t xThermoSensorHandler;
int print_USART;

/**
 * This method initializes the wifi module by using the wireless_interface class.  Opens the
 * usart port for both the terminal (USART_0), and the wifi (USART_2).  Then it sets the wireless SSID.
 * Finally it activates the wireless connection using the wireless_interface class.
 */
void initializeWifi() {
	taskENABLE_INTERRUPTS();
	int terminalUSART = usartOpen(USART_0, BAUD_RATE_115200, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX);
	int wifiUSART = usartOpen(USART_2, BAUD_RATE_9600, portSERIAL_BUFFER_TX, portSERIAL_BUFFER_RX);
	gs_initialize_module(wifiUSART, BAUD_RATE_9600, terminalUSART, BAUD_RATE_115200);
	gs_set_wireless_ssid("TeamJeffChico");
	gs_activate_wireless_connection();
}

/**
 * This method initializes the web server by using the wireless_interface class.  It first
 * configure the web page by setting a page title, and a type of component into it (dropdown list)
 * Then it adds the choices in that dropdown list. After this, it calls the method start_web_server from
 * the wireless_interface class so the server will be able to process the client request and responses.
 */
void initializeWebServer() {
	configure_web_page("Chico: The Robot", "! Control Interface !", HTML_DROPDOWN_LIST);
	add_element_choice('F', "Forward");
	add_element_choice('B', "Backward");
	add_element_choice('L', "Spin Left");
	add_element_choice('R', "Spin Right");
	add_element_choice('S', "Stop");
	add_element_choice('A', "Attachment");
	start_web_server();
}

/**
 * The task handles the command mode of chico, it initializes the motion module and
 * the thermoSensor module to mode the head when going forward or backward. Then
 * according to the client request set in the web server task, chico will either go
 * forward (F), backward (B), spin left (L), spin right (R) or stop (S).  This task uses the
 * motion module to move the robot.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
void vTaskCommandMode(void *pvParameters) {
	motionInit();
	vTaskResume(xThermoSensorHandler);
	while (1) {
		//usart_fprintf_P(USART_0, PSTR("COMMAND set too: %c"), clientRequest);
		// Move forward
		if(clientRequest == 'F') {
			motionForward();
		}
		// Move backward
		else if(clientRequest == 'B') {
			motionBackward();
		}
		// Spin Left
		else if(clientRequest == 'L') {
			motionSpinLeft();
		}
		// Spin Right
		else if(clientRequest == 'R') {
			motionSpinRight();
		}
		// Stop moving
		else if(clientRequest == 'S') {
			motionStop();
		}

		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

/**
 * The task handles the attachment mode of chico, it initializes the motion module and then
 * handles the three modes possible in this mode (Searching, Panic, Attached). These three mode
 * analyzes the tempreature, move the robot according to the mode it is in.  In attached, chico
 * follows an heat source that he found.  In the searching modes it searches an heatsource by turning slowly.
 * In the panic mode, it spins fast to then come back in searching mode.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
void vTaskAttachmentMode(void *pvParameters) {

	typedef enum {
		Attached,
		Searching,
		Panic
	} State;
	State state = Searching;
	int count = 0;
	int INCREMENT = 100;
	int MAX_COUNT = 10000;

	motionInit();
	vTaskSuspend(xThermoSensorHandler);
	while (1)
	{
		int left = getLeft3AvgTemperatures();
		int center = getCenter4AvgTemperatures();
		int right = getRight3AvgTemperatures();
		//int ambient = getAmbientTemperature();

//		motionSpinLeftSlow();

		if (state == Searching) {
			motionSpinLeftSlow();
			// ensure significant heat source
			//if (left > ambient + 3 || right > ambient + 3 || center > ambient + 3) {
			if(getSignificantTemperature()) {
				state = Attached;
			}
		}

		else if(state == Attached) {
			// ensure significant heat source
			//if (left > ambient + 3 || right > ambient + 3 || center > ambient + 3) {
			if(getSignificantTemperature()){
				count = 0;
				// Heat source is to the left
				int distance = getDistance();

				if(distance > 30) {
					if (left > center) {
						motionSpinLeft();
					}
					// Heat source is to the right
					else if (right > center) {
						motionSpinRight();
					}
					// Heat source is in the center, follow it
					else {
						motionForward();
					}
				}else if(distance > 0){
					motionStop();
				}
			}
			else {
				motionStop();
				count += INCREMENT;
				if (count >= MAX_COUNT) {
					count = 0;
					state = Panic;
				}
			}

		} else if(state == Panic) {
			motionSpinRight();
			count += INCREMENT;
			if (count == MAX_COUNT) {
				count = 0;
				state = Searching;
			}
		}

		vTaskDelay(INCREMENT / portTICK_PERIOD_MS);
	}

}

/**
 * The task handles the webserver and is responsible for handling both the process request from the client and to get the
 * client response to the suspend and resume either the command mode or task mode depending on
 * the client response.
 *
 * @param pvParameters Used only for function definition compatibility.
 */
void vTaskWebServer(void *pvParameters)
{
	//vTaskSuspend(vTaskAttachmentMode);
	//vTaskSuspend(vTaskCommandMode);

	vTaskDelay(5500 / portTICK_PERIOD_MS);
	while (1)
	{
		process_client_request();
		clientRequest = get_next_client_response();
		if(clientRequest != previousClientRequest){
			if (clientRequest == 'A') {
				vTaskDelay(100 / portTICK_PERIOD_MS);
				// Delay to ensure the variable is updated
				//vTaskSuspend(xCommandHandler);
				//vTaskResume(xAttachmentHandler);
			}
			else if (clientRequest == 'F' ||
				clientRequest == 'B' || clientRequest == 'L' ||
				clientRequest == 'R' || clientRequest == 'S') {
				vTaskDelay(100 / portTICK_PERIOD_MS);
				// Delay to ensure the variable is updated
				//vTaskSuspend(xAttachmentHandler);
				vTaskResume(xCommandHandler);
			}
		}
		previousClientRequest = clientRequest;
		vTaskDelay((5500 / portTICK_PERIOD_MS));
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
	initialize_module_timer0();
	initializeWifi();
	initializeWebServer();
	//xTaskCreate(vTaskWebServer, (const portCHAR *)"", 1024, NULL, 1, NULL);
    xTaskCreate(vTaskTemperature, (const portCHAR *)"", 128, NULL, 3, NULL);
//    xTaskCreate(vTaskMoveChico, (const portCHAR *)"", 256, NULL, 3, NULL);
    xTaskCreate(vTaskMoveThermoSensor, (const portCHAR *)"", 256, NULL, 3, &xThermoSensorHandler);
    //vTaskSuspend(xThermoSensorHandler);
    xTaskCreate(vTaskDecoder, (const portCHAR *)"", 128, NULL, 3, NULL);
    xTaskCreate(vTaskLCD, (const portCHAR *)"", 128, NULL, 3, NULL);
	xTaskCreate(vTaskCommandMode, (const portCHAR *)"", 128, NULL, 3, &xCommandHandler);
	//vTaskSuspend(xCommandHandler);
	//xTaskCreate(vTaskAttachmentMode, (const portCHAR *)"", 256, NULL, 3, &xAttachmentHandler);
	//vTaskSuspend(xAttachmentHandler);
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
