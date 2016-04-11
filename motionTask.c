#include "FreeRTOS.h"
#include "motion.h"
#include "motionTask.h"
#include "task.h"
#include "LEDHandler.h"

/// Flag used to control the movement of the thermo sensor relative to the
/// robot's movement.
bool thermoSensorFlag = false;
/// Variable used to control the thermo sensor's movement.
static int thermoLeft = 1;
/// Value used for calculating the thermo sensor's wave values.
static int pulseWidth = 2800;

/**
 * Initializes motion module and also calls the method to set up the led module
 */
void motionInit()
{
    motion_init();
    setupLED();
}

/**
 * Move Chico backwards by setting the pulse width of the left wheel to 4800
 * and the right wheel to 1100.
 * Also display the green led from the LEDHandler class.
 */
void motionForward()
{
    thermoSensorFlag = true;
    motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
    motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);
    motion_servo_set_pulse_width(MOTION_SERVO_CENTER, 2560);

    motion_servo_start(MOTION_WHEEL_RIGHT);
    motion_servo_start(MOTION_WHEEL_LEFT);
    motion_servo_start(MOTION_SERVO_CENTER);
    displayGreenLED();

}

/**
 * Move Chico backwards by setting the pulse width of the left wheel to 1100
 * and the right wheel to 4800.
 * Also display the red led from the LEDHandler class.
 */
void motionBackward()
{
    thermoSensorFlag = true;
    motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 4800);
    motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 1100);

    motion_servo_start(MOTION_WHEEL_RIGHT);
    motion_servo_start(MOTION_WHEEL_LEFT);
    displayRedLED();
}

/**
 * Spin Chico to the left slowly by setting the pulse width on both wheels to 1100
 * Also display the blue led from the LEDHandler class.
 */
void motionSpinLeft()
{
    thermoSensorFlag = false;
    motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 1100);
    motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 1100);

    motion_servo_start(MOTION_WHEEL_RIGHT);
    motion_servo_start(MOTION_WHEEL_LEFT);
    displayBlueLED();
}

/**
 * Spin Chico to the left slowly by setting the pulse width on both wheels to 2750
 * Also display the blue led from the LEDHandler class.
 */
void motionSpinLeftSlow()
{
    thermoSensorFlag = false;
    motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 2750);
    motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 2750);

    motion_servo_start(MOTION_WHEEL_RIGHT);
    motion_servo_start(MOTION_WHEEL_LEFT);
    displayBlueLED();
}

/**
 * Spin Chico to the right by setting both the right and left wheel pulse width to 4800.
 * Also displays the blue led from the LEDHandler class.
 */
void motionSpinRight()
{
    thermoSensorFlag = false;
    motion_servo_set_pulse_width(MOTION_WHEEL_RIGHT, 4800);
    motion_servo_set_pulse_width(MOTION_WHEEL_LEFT, 4800);

    motion_servo_start(MOTION_WHEEL_RIGHT);
    motion_servo_start(MOTION_WHEEL_LEFT);
    displayBlueLED();
}

/**
 * Stop Chico from moving by stopping both the wheels, also calls the method to display white led
 * from the LEDHandler class.
 */
void motionStop()
{
    thermoSensorFlag = false;
    motion_servo_stop(MOTION_WHEEL_RIGHT);
    motion_servo_stop(MOTION_WHEEL_LEFT);
    displayWhiteLED();
}

/**
 * Move the Thermo Sensor to the left and right while Chico is moving, and
 * return it to the center
 * when Chico is not moving
 */
void motionThermoSensor()
{
    if (thermoLeft == 1)
    {
        pulseWidth -= 100;
        if (pulseWidth <= 1100)
        {
            thermoLeft = 0;
        }
    }
    else
    {
        pulseWidth += 100;
        if (pulseWidth >= 4800)
        {
            thermoLeft = 1;
        }
    }
    motion_servo_set_pulse_width(MOTION_SERVO_CENTER, pulseWidth);
    motion_servo_start(MOTION_SERVO_CENTER);
}

/**
 * Stop the Thermo Sensor by setting the pulse width to 2800.
 */
void motionThermoSensorStop()
{
    motion_servo_set_pulse_width(MOTION_SERVO_CENTER, 2800);
    motion_servo_start(MOTION_SERVO_CENTER);
    pulseWidth = 2800;
}
