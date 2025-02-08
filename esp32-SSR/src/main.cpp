#include <Arduino.h>
#include <receiver_espnow.h>

int CH1_PIN = 26;
int CH2_PIN = 27;

String EspID = "ESP32-SSR";

void setup()
{
  pinMode(CH1_PIN, OUTPUT);
  pinMode(CH2_PIN, OUTPUT);
  digitalWrite(CH1_PIN, HIGH);
  digitalWrite(CH2_PIN, HIGH);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  vTaskDelete(NULL);
}

void loop()
{
}