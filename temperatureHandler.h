
#ifndef TEMPERATUREHANDLER_H_
#define TEMPERATUREHANDLER_H_

int getLeft3AvgTemperatures();
int getRight3AvgTemperatures();
int getCenter4AvgTemperatures();
int getAmbientTemperature();
int getSignificantTemperature();

void setupTemperature();
void updateTemperatures(int *ambientTemperature, int *leftAverageTemperature, int *rightAverageTemperature);

#endif /* TEMPERATUREHANDLER_H_ */
