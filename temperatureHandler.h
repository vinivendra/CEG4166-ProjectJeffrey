
#ifndef TEMPERATUREHANDLER_H_
#define TEMPERATUREHANDLER_H_

/**
 * Stores the 8 pixel temperatures as read by the last call to the
 * `updateTemperatures` function.
 */
int pixelTemperatures[8];
/**
 *  Stores the ambient temperature as read by the last call to the
 * `updateTemperatures` function.
 */
int ambiantTemperature;
/**
 *  Stores the average of the 8 pixel temperatures, as read by the last call to
 * the `updateTemperatures` function.
 */
int averageTemperature;

void updateTemperatures();

#endif /* TEMPERATUREHANDLER_H_ */
