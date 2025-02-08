#ifndef RECEIVER_ESPNOW_H
#define RECEIVER_ESPNOW_H

#include <esp_now.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include "WiFi.h"
#include <FastLED.h>

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void espNowAddReceiver(const uint8_t *receiver);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

#endif