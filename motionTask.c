#include "FreeRTOS.h"
#include "task.h"
#include "motion.h"
#include "motionTask.h"

bool thermoSensorFlag = false;
static int thermoLeft = 1;
static int pulseWidth = 2800;

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
	thermoSensorFlag = true;
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
	thermoSensorFlag = true;
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
	thermoSensorFlag = false;
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
	thermoSensorFlag = false;
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
	thermoSensorFlag = false;
	motion_servo_stop(MOTION_WHEEL_RIGHT);
	motion_servo_stop(MOTION_WHEEL_LEFT);
}

/**
 * Move the Thermo Sensor to the left and right while Chico is moving, and return it to the center
 * when Chico is not moving
 */
void motionThermoSensor() {
	if(thermoLeft == 1){
		pulseWidth -= 100;
		if(pulseWidth <= 1100){
			thermoLeft = 0;
		}
	}else{
		pulseWidth += 100;
		if(pulseWidth >= 4800){
			thermoLeft = 1;
		}
	}
	motion_servo_set_pulse_width(MOTION_SERVO_CENTER, pulseWidth);
	motion_servo_start(MOTION_SERVO_CENTER);
}

/**
 * Stop the Thermo Sensor
 */
void motionThermoSensorStop() {
	motion_servo_set_pulse_width(MOTION_SERVO_CENTER, 2800);
	motion_servo_start(MOTION_SERVO_CENTER);
	pulseWidth = 2800;
}
