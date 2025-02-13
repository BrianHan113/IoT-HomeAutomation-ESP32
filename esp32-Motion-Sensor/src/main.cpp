// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#include <Arduino.h>
#include <receiver_espnow.h>

#define LED 2
#define PIR_PIN 13

String EspID = "MOTION_SENSOR";
uint8_t senderMac[6];

bool isEnabled = false;

// Configs based on settings on nextion
String linkedSwitch = "";
int timeoutMins = -1;
String channel = "";

boolean pirState;
boolean motionDetected = false;
TimerHandle_t turnOffTimer = NULL;

void turnSwitchOff(TimerHandle_t xTimer)
{
  if (!isEnabled)
  {
    Serial.println("Tried to turn off but Motion Sensor not enabled");
    motionDetected = false;
    return;
  }

  char dataToSend[32] = "";
  Serial.println("Function executed after delay.");
  strcpy(dataToSend, (linkedSwitch + "OFF").c_str());
  motionDetected = true;

  esp_err_t result = esp_now_send(senderMac, (uint8_t *)dataToSend, sizeof(dataToSend));

  if (result == ESP_OK)
  {
    Serial.println("Sending Motion info confirmed");
  }
  else
  {
    Serial.printf("Sending Motion info error: %d\n", result);
  }
  motionDetected = false;
}

void motionDetectTask(void *params)
{
  while (true)
  {
    bool isConfigured = false;
    if (linkedSwitch != "" && timeoutMins != -1 && channel != "")
    {
      isConfigured = true;
    }

    if (!isConfigured)
    {
      Serial.println("Motion Sensor not configured. Waiting...");
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Yield for watchdog reset
      continue;
    }

    if (!isEnabled)
    {
      Serial.println("Motion Sensor not enabled");
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Yield for watchdog reset
      continue;
    }

    char dataToSend[32] = "";
    pirState = digitalRead(PIR_PIN);

    if (!motionDetected && pirState)
    {
      strcpy(dataToSend, (linkedSwitch + "ON" + channel).c_str());
      motionDetected = true;

      esp_err_t result = esp_now_send(senderMac, (uint8_t *)dataToSend, sizeof(dataToSend));

      if (result == ESP_OK)
      {
        Serial.println("Sending Motion info confirmed");
      }
      else
      {
        Serial.printf("Sending Motion info error: %d\n", result);
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      // Create and start FreeRTOS timer
      int timeoutInTicks = timeoutMins * 60 * 1000 / portTICK_PERIOD_MS; // Timeout in ticks

      if (timeoutInTicks <= 0)
      {
        timeoutInTicks = 1000 / portTICK_PERIOD_MS;
      }

      turnOffTimer = xTimerCreate(
          "MyTimer",
          pdMS_TO_TICKS(timeoutInTicks),
          pdFALSE,
          (void *)0,
          turnSwitchOff);

      xTimerStart(turnOffTimer, 0);

      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Ensure task is yielding to prevent watchdog trigger
  }
}

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Create FreeRTOS timer
  turnOffTimer = xTimerCreate("turnOffTimer", pdMS_TO_TICKS(1), pdFALSE, (void *)0, turnSwitchOff);

  // Create and start motion detection task
  xTaskCreatePinnedToCore(motionDetectTask, "Motion detection", 8192, NULL, 1, NULL, APP_CPU_NUM);

  vTaskDelete(NULL); // Delete main task as it is not used
}

void loop()
{
}
