#include <Arduino.h>
#include <FastLED.h>
#include <receiver_espnow.h>

#define HW_NUM_LEDS 19 // Number of LEDs in the strip, change this to your strip length
#define DATA_PIN 26

String EspID = "STATUS_LED";
CRGB leds[HW_NUM_LEDS];

float hue = 213;      // Initial purple colour, will change when gpu temp is received
int brightness = 128; // Start 50% brightness to match nextion gui
int NUM_LEDS = -1;
bool isStripOn = false;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  // FastLED supports many other LED Strips, enter your own here
  FastLED.addLeds<WS2815, DATA_PIN, RGB>(leds, HW_NUM_LEDS);
  FastLED.clear();
}

void loop()
{
  if (NUM_LEDS > 0) // check NUM_LEDS is configured in nextion settings
  {
    if (isStripOn)
    {
      FastLED.clear();
      FastLED.setBrightness(brightness);
      for (int i = 0; i < min(NUM_LEDS, HW_NUM_LEDS); i++)
      {
        leds[i] = CHSV((uint8_t)hue, 255, 255);
      }
    }
    else
    {
      FastLED.clear();
    }
    FastLED.show();
  }

  vTaskDelay(100 / portTICK_PERIOD_MS);
}