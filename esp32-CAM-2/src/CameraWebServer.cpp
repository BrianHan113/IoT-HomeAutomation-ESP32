// NOTE: Most of this is taken from the example code provided by the library and adapted to fit the project needs

#include "esp_camera.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "secrets.h"

#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"
#define LED_GPIO_NUM 4

void startCameraServer();

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QQVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  // if (config.pixel_format == PIXFORMAT_JPEG)
  // {
  //   if (psramFound())
  //   {
  //     config.jpeg_quality = 10;
  //     config.fb_count = 2;
  //     config.grab_mode = CAMERA_GRAB_LATEST;
  //   }
  //   else
  //   {
  //     // Limit the frame size when PSRAM is not available
  //     config.frame_size = FRAMESIZE_SVGA;
  //     config.fb_location = CAMERA_FB_IN_DRAM;
  //   }
  // }
  // else
  // {
  //   // Best option for face detection/recognition
  //   config.frame_size = FRAMESIZE_240X240;
  // }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // sensor_t *s = esp_camera_sensor_get();
  // // drop down frame size for higher initial frame rate
  // if (config.pixel_format == PIXFORMAT_JPEG)
  // {
  //   s->set_framesize(s, FRAMESIZE_QVGA);
  // }

  // // Hosting server on esp
  // WiFi.softAP(ssid, password);
  // Serial.println("Access Point Started");
  // Serial.print("AP IP address: ");
  // Serial.println(WiFi.softAPIP());

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  if (MDNS.begin("esp32cam2"))
  {
    Serial.println("MDNS responder started. Access at http://esp32cam2.local");
  }

  startCameraServer();

  Serial.println("Camera Ready! Use the above IP to connect");

  // Connecting to external WiFi
  // WiFi.begin(ssid, password);
  // WiFi.setSleep(false);

  // Serial.print("WiFi connecting");
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi connected");

  // startCameraServer();

  // Serial.print("Camera Ready! Use 'http://");
  // Serial.print(WiFi.localIP());
  // Serial.println("' to connect");
  vTaskDelete(NULL);
}

void loop()
{
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}