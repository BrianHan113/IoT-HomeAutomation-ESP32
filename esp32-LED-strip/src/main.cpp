#include <Arduino.h>
#include <FastLED.h>
#include <receiver_espnow.h>

#define HW_NUM_LEDS 19
#define DATA_PIN 26

String EspID = "LED_STRIP";
CRGB leds[HW_NUM_LEDS];

uint8_t red = 255;
uint8_t green = 255;
uint8_t blue = 255;

CRGB colour;

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

  FastLED.addLeds<WS2815, DATA_PIN, RGB>(leds, HW_NUM_LEDS);
  FastLED.clear();
}

void loop()
{

  if (NUM_LEDS != -1)
  {
    if (isStripOn)
    {
      colour = CRGB(red, green, blue);
      FastLED.clear();
      for (int i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = colour;
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