
#ifndef LCDHANDLER_H_
#define LCDHANDLER_H_

void setupLCD();
void writeToLCD(float speed, float distanceTravelled, int averageTemperature, int leftTemperature, int rightTemperature);
void shutdownLCD();

#endif /* LCDHANDLER_H_ */
