#include <Arduino.h>
#include <receiver_espnow.h>

#define LED 2

String EspID = "ESP8266";

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH); // Ensure LED is off
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