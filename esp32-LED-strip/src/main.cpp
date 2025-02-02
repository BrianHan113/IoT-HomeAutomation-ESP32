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
  WiFi.channel(1);
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

// uint8_t r, g, b;
// r = (colour >> 11) & 0x1F; // Red: 5 bits
// g = (colour >> 5) & 0x3F;  // Green: 6 bits
// b = colour & 0x1F;         // Blue: 5 bits

// // Scale to 8 bits (0–255)
// r = (r * 255) / 31; // Scale 5-bit to 8-bit
// g = (g * 255) / 63; // Scale 6-bit to 8-bit
// b = (b * 255) / 31;
// leds[1] = CRGB(g, r, b);