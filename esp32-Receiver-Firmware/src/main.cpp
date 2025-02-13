// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#include <Arduino.h>
#include <receiver_espnow.h>

#define LED 2 // Builtin LED (could be different depending on your board)

// Enter any ID e.g. "Livingroom lights", but make sure it is unique.
// It will appear on the settings page at any "select esp" dropdown
String EspID = "ENTER_ID_HERE";

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
}