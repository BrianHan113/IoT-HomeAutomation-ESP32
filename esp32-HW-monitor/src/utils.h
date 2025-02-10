#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "addresses_helper.h"
void queueNextionCommand(String command);
uint32_t nextionNumConvert(int start, int end, String command);
float calculateAverage(const float arr[], int size);
int weatherCodeToNextionPicID(int code, bool isDay);
int windSpeedToNextionPicID(float windSpeed);
int windToNextionWindBarbID(float windSpeed, int windDir);
void nextionWaveformYAxisScale(int min, int max, String waveformID);
int rainToNextionRainID(float prec, int prec_probability);
void sendCommandToDevice(String command, String device);
void sendNextionCommand(String command);

#endif