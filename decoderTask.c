#include "FreeRTOS.h"
#include "task.h"

#include "motion.h"
#include "decoderTask.h"
#include "LCDHandler.h"

void decoderTask(void *pvParameters)
{
    setupLCD();
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    uint32_t ticksLeft;
    uint32_t ticksRight;

    float leftSpeed = 0;
    float rightSpeed = 0;

    while(1)
    {
    	int newDataAvailableLeft = motion_enc_read(MOTION_WHEEL_LEFT, &ticksLeft);
    	int newDataAvailableRight = motion_enc_read(MOTION_WHEEL_RIGHT, &ticksRight);

    	if(newDataAvailableLeft)
    	{
    		float timeLeft = ticksLeft*500*10E-9;
    		leftSpeed = 0.0054/timeLeft; // (m/s)
    	}

    	if(newDataAvailableRight)
    	{
    		float timeRight = ticksRight*500*10E-9;
    		rightSpeed = 0.0054/timeRight; // (m/s)
    	}
    	displaySpeedInLCD(leftSpeed, rightSpeed);
    	vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
    }
}
