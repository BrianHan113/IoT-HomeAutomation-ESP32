#include <Arduino.h>
#include <receiver_espnow.h>

#define LED 2

String EspID = "ENTER_ID_HERE";

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.channel(1);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
}