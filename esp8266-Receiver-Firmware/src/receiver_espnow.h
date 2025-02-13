// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#ifndef RECEIVER_ESPNOW_H
#define RECEIVER_ESPNOW_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
void espNowAddReceiver(const uint8_t *receiver);
void OnDataRecv(u8 *mac, u8 *data, u8 len);

#endif