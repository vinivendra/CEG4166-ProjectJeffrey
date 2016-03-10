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

/**
 * Initializes motion
 */
void motionInit()
{
	motion_init();
}

/**
 * Move Chico forwards
 */
void motionForward()
{
	motion_init();
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

/**
 * Move Chico backwards
 */
void motionBackward()
{
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 4800);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 1100);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

/**
 * Spin Chico to the left
 */
void motionSpinLeft()
{
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 1100);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

/**
 * Spin Chico to the right
 */
void motionSpinRight()
{
	motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 4800);
	motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);

	motion_servo_start(MOTION_WHEEL_RIGHT);
	motion_servo_start(MOTION_WHEEL_LEFT);
}

/**
 * Stop Chico from moving
 */
void motionStop()
{
	motion_servo_stop(MOTION_WHEEL_RIGHT);
	motion_servo_stop(MOTION_WHEEL_LEFT);
}

/**
 * Move the Thermo Sensor to the right
 */
void motionThermoSensorRight() {
	motion_servo_set_pulse_width(MOTION_SERVO_CENTER, 1100);
}

/**
 * Move the Thermo Sensor to the left
 */
void motionThermoSensorLeft() {
	motion_servo_set_pulse_width(MOTION_SERVO_CENTER, 4800);
}

/**
 * Stop the Thermo Sensor
 */
void motionThermoSensorStop() {
	motion_servo_set_pulse_width(MOTION_SERVO_CENTER, 2560);
}
