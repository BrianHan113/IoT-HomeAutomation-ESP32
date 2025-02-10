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

  xTaskCreatePinnedToCore(receiveHardwareData, "Recieve Hardware Data from c# app", 16384, NULL, 2, NULL, PRO_CPU_NUM);
  xTaskCreatePinnedToCore(sendNextionSerial, "Send commands to nextion", 16384, NULL, 1, NULL, APP_CPU_NUM);
  xTaskCreatePinnedToCore(sendHardwareData, "Send Hardware Data to nextion", 16384, NULL, 1, NULL, APP_CPU_NUM);
  xTaskCreatePinnedToCore(sendWeatherData, "Send Weather Data to nextion", 16384, NULL, 1, NULL, APP_CPU_NUM);
  // Uncomment to enable tide data transfer - see README
  // xTaskCreatePinnedToCore(sendTideData, "Send Tide Data to nextion", 16384, NULL, 1, NULL, APP_CPU_NUM);
  xTaskCreatePinnedToCore(receiveNextionSerial, "Receive data from nextion", 16384, NULL, 2, NULL, APP_CPU_NUM);
  xTaskCreatePinnedToCore(executeCommands, "Execute nextion commands", 16384, NULL, 2, NULL, APP_CPU_NUM);
  xTaskCreatePinnedToCore(sendTempData, "Send Hardware temperature data", 2048, NULL, 1, &sendTempDataHandle, APP_CPU_NUM);
  vTaskSuspend(sendTempDataHandle);

  Serial.print("Reset reason: ");
  Serial.println(esp_reset_reason());

  vTaskDelete(NULL); // Delete the main task -> loop() will never run
}

void loop()
{
  Serial.println("------------Main Loop------------");
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
  UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
  Serial.println("Stack Loop: " + String(stackLeft));
  vTaskDelay(2000 / portTICK_PERIOD_MS);
}