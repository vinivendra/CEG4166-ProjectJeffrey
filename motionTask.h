
#ifndef MOTIONTASK_H_
#define MOTIONTASK_H_

void motionForward(void *pvParameters);
void motionBackward(void *pvParameters);
void motionSpinLeft(void *pvParameters);
void motionSpinRight(void *pvParameters);
void motionStop(void *pvParameters);

#endif /* MOTIONTASK_H_ */
