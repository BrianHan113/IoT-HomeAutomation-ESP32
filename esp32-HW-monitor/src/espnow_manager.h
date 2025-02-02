#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <esp_now.h>
#include <Arduino.h>
#include "addresses_helper.h"
#include "utils.h"

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void espNowAddReceiver(const uint8_t *receiver);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

#endif