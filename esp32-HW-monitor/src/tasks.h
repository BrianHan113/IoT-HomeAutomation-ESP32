#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include "ArduinoJson.h"
#include <esp_now.h>
#include <HardwareSerial.h>
#include <string>
#include "addresses_helper.h"
#include "utils.h"
#include "security_camera.h"

void receiveHardwareData(void *params);
void sendHardwareData(void *params);
void sendWeatherData(void *params);
void sendTempData(void *params);
void receiveNextionSerial(void *params);
void executeCommands(void *params);
void sendTideData(void *params);

#endif