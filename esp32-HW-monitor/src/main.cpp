// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

// USING ESP32-S3 with COM and USB port
// Because we set ARDUINO_USB_CDC_ON_BOOT=1 in setup,
// Serial0 is our extra Serial, and Serial is our normal Serial
// Serial0 -> COM Port
// Serial  -> USB Port
// Usage: Send & receive data at COM port, use USB port as normal for debugging

#include "tasks.h"
#include "espnow_manager.h"
#include "secrets.h"
#include "WiFi.h"
#include "addresses_helper.h"
#include "esp_wifi.h"
#include "security_camera.h"

HardwareSerial nextion(2);

SemaphoreHandle_t hardwareDataSemaphore;
SemaphoreHandle_t weatherBinSemaphore;
SemaphoreHandle_t tideBinSemaphore;
QueueHandle_t commandQueue;
QueueHandle_t sendNextionQueue;
const uint8_t COMMAND_QUEUE_LEN = 10;
const uint8_t SEND_NEXTION_QUEUE_LEN = 50;

// All task handles
TaskHandle_t receiveHardwareDataHandle = NULL;
TaskHandle_t sendNextionSerialHandle = NULL;
TaskHandle_t sendHardwareDataHandle = NULL;
TaskHandle_t sendWeatherDataHandle = NULL;
TaskHandle_t sendTideDataHandle = NULL;
TaskHandle_t receiveNextionSerialHandle = NULL;
TaskHandle_t executeCommandsHandle = NULL;
TaskHandle_t sendTempDataHandle = NULL;

void setup()
{
  Serial0.begin(115200);
  Serial.begin(115200);
  nextion.begin(115200, SERIAL_8N1, 18, 17); // RX, TX

  delay(3000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  // Setup for camera capture functionality - see README
  // WiFi.mode(WIFI_AP_STA);
  // WiFi.begin(WIFI_SSID, WIFI_PASS);

  // // Wait for WiFi connection
  // int tries = 0;
  // while (WiFi.status() != WL_CONNECTED && tries < 10)
  // {
  //   delay(500);
  //   Serial.println("Connecting to WiFi...");
  //   tries++;
  // }

  // if (!SPIFFS.begin(true))
  // {
  //   Serial.println("SPIFFS initialization failed!");
  //   return;
  // }

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");
    return;
  }
  Serial.println("Initialized ESP-NOW");

  esp_now_register_recv_cb(OnDataRecv);
  espNowAddReceiver(broadcastAddress);

  hardwareDataSemaphore = xSemaphoreCreateCounting(2, 0); // 2 slots for sendHardwareData and sendTempData
  weatherBinSemaphore = xSemaphoreCreateBinary();
  tideBinSemaphore = xSemaphoreCreateBinary();
  commandQueue = xQueueCreate(COMMAND_QUEUE_LEN, sizeof(char[500]));
  sendNextionQueue = xQueueCreate(SEND_NEXTION_QUEUE_LEN, sizeof(char[100]));

  xTaskCreatePinnedToCore(receiveHardwareData, "Recieve Hardware Data from c# app", 16384, NULL, 2, &receiveHardwareDataHandle, PRO_CPU_NUM);
  xTaskCreatePinnedToCore(sendNextionSerial, "Send commands to nextion", 16384, NULL, 1, &sendNextionSerialHandle, APP_CPU_NUM);
  xTaskCreatePinnedToCore(sendHardwareData, "Send Hardware Data to nextion", 16384, NULL, 1, &sendHardwareDataHandle, APP_CPU_NUM);
  xTaskCreatePinnedToCore(sendWeatherData, "Send Weather Data to nextion", 16384, NULL, 1, &sendWeatherDataHandle, APP_CPU_NUM);
  // Uncomment to enable tide data transfer - see README
  // xTaskCreatePinnedToCore(sendTideData, "Send Tide Data to nextion", 16384, NULL, 1, NULL, APP_CPU_NUM);
  xTaskCreatePinnedToCore(receiveNextionSerial, "Receive data from nextion", 16384, NULL, 2, &receiveNextionSerialHandle, APP_CPU_NUM);
  xTaskCreatePinnedToCore(executeCommands, "Execute nextion commands", 16384, NULL, 2, &executeCommandsHandle, APP_CPU_NUM);
  xTaskCreatePinnedToCore(sendTempData, "Send Hardware temperature data", 2048, NULL, 1, &sendTempDataHandle, APP_CPU_NUM);
  vTaskSuspend(sendTempDataHandle);

  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());

  vTaskDelete(NULL); // Delete the main task -> loop() will never run
}

void loop()
{
  Serial.println("------------Memory Metrics------------");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
  UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
  Serial.println("Main stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(receiveHardwareDataHandle);
  Serial.println("receiveHardwareData stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(sendNextionSerialHandle);
  Serial.println("sendNextionSerial stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(sendHardwareDataHandle);
  Serial.println("ssendHardwareData stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(sendWeatherDataHandle);
  Serial.println("sendWeatherData stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(receiveNextionSerialHandle);
  Serial.println("receiveNextionSerial stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(executeCommandsHandle);
  Serial.println("executeCommands stack: " + String(stackLeft));
  stackLeft = uxTaskGetStackHighWaterMark(sendTempDataHandle);
  Serial.println("sendTempData stack: " + String(stackLeft));
  Serial.println("--------------------------------------");

  vTaskDelay(2000 / portTICK_PERIOD_MS);
}