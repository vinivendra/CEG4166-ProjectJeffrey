#ifndef MOTIONTASK_H_
#define MOTIONTASK_H_
#include <stdbool.h>

extern bool thermoSensorFlag;

void motionInit();
void motionForward();
void motionBackward();
void motionSpinLeft();
void motionSpinLeftSlow();
void motionSpinRight();
void motionStop();
void motionThermoSensor();
void motionThermoSensorStop();

#endif /* MOTIONTASK_H_ */
