// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#ifndef SECURITY_CAMERA_H
#define SECURITY_CAMERA_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include "utils.h"
#include "tasks.h"

extern const String cam1ServerUrl;
extern const String cam2ServerUrl;

void uploadCamCaptureToNextion(String camServer, String cameraId);

#endif