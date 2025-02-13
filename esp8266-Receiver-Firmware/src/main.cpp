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
String EspID = "ESP8266";

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); // Ensure LED is off, this particular board has active-low logic
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_channel(1);

  if (esp_now_init() != 0)
  {
    Serial.println("ESP-NOW Initialization Failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
}

void loop()
{
}