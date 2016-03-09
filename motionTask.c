#include "FreeRTOS.h"
#include "task.h"

#include "motion.h"
#include "motionTask.h"

//void motionTask(void *pvParameters)
//{
//    TickType_t xLastWakeTime;
//    xLastWakeTime = xTaskGetTickCount();
//
//    motion_init();
//    motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
//    motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);
//
//    motion_servo_start(MOTION_WHEEL_RIGHT);
//    motion_servo_start(MOTION_WHEEL_LEFT);
//
//
//    vTaskDelayUntil(&xLastWakeTime, (2000 / portTICK_PERIOD_MS));
//
//    motion_servo_stop(MOTION_WHEEL_RIGHT);
//    motion_servo_stop(MOTION_WHEEL_LEFT);
//}

void motionForward(void *pvParameters)
{
	motion_init();
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

void motionBackward(void *pvParameters)
{
	motion_init();
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 4800);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 1100);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

void motionSpinLeft(void *pvParameters)
{
	motion_init();
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 1100);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

void motionSpinRight(void *pvParameters)
{

	motion_init();
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 4800);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

void motionStop(void *parameters)
{
	motion_servo_stop(MOTION_WHEEL_RIGHT);
	motion_servo_stop(MOTION_WHEEL_LEFT);
}
