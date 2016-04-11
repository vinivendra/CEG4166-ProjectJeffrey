#include "FreeRTOS.h"
#include "task.h"

#include "motion.h"
#include "decoderTask.h"
#include "LCDHandler.h"


#define distancePerRisingEdge 0.0054


float distanceTravelled = 0;

/**
 * Calculates the speed and the distance travelled of chico. It calculates the
 * speed of both the right and left wheel.  Then does the average to return the speed.
 * Then it calculates the distance travelled using the left wheel speed and the period.
 *
 * @param period      Represents the period of that the calculation is done.
 * @param returnSpeed  A pointer to a float in which the speed will be stored.
 * @param returnDistanceTravelled A pointer to a float in which the distance
 * travelled will be stored.
 */
void decoderTask(int period, float *returnSpeed, float *returnDistanceTravelled)
{
	uint32_t ticksLeft;
	uint32_t ticksRight;

	float leftSpeed = 0;
	float rightSpeed = 0;

	int newDataAvailableLeft = motion_enc_read(MOTION_WHEEL_LEFT, &ticksLeft);
	int newDataAvailableRight = motion_enc_read(MOTION_WHEEL_RIGHT, &ticksRight);

	if(newDataAvailableLeft)
	{
		float timeLeft = ticksLeft*500E-9;
		leftSpeed = distancePerRisingEdge /timeLeft; // (m/s)
	}
	else
	{
		leftSpeed = 0;
	}

	if(newDataAvailableRight)
	{
		float timeRight = ticksRight*500E-9;
		rightSpeed = distancePerRisingEdge /timeRight; // (m/s)
	}
	else
	{
		rightSpeed = 0;
	}

	*returnSpeed = (leftSpeed + rightSpeed) / 2;

	*returnDistanceTravelled += leftSpeed * (period / (float)1000);
}

