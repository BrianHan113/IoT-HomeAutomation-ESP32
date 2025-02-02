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