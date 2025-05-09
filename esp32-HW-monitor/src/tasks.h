// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

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
void sendNextionSerial(void *pvParameters);

#endif