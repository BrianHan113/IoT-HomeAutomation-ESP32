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
esp_timer_handle_t timerHandle = NULL;

void turnSwitchOff(void *arg)
{
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
      int64_t delayMicroseconds = timeoutMins * 60 * 1000000;
      // delayMicroseconds = 5 * 1000000; // 5 seconds for testing
      esp_timer_create_args_t timerArgs = {
          .callback = &turnSwitchOff,
          .arg = NULL,
          .dispatch_method = ESP_TIMER_TASK,
          .name = "turnSwitchOffAfterDelay"};

      esp_timer_create(&timerArgs, &timerHandle);
      esp_timer_start_once(timerHandle, delayMicroseconds);
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
  WiFi.channel(1);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  xTaskCreatePinnedToCore(motionDetectTask, "Motion detection", 2048, NULL, 1, NULL, APP_CPU_NUM);

  vTaskDelete(NULL); // Delete main task as it is not used
}

void loop()
{
  // The loop is empty because we're using FreeRTOS tasks
}
