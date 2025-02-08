#include <Arduino.h>
#include <receiver_espnow.h>
#include "DHTesp.h"

#define LED 2
#define DHT 26
DHTesp dht;

String EspID = "TEMP_SENSOR";
uint8_t senderMac[6];

bool isEnabled = false;

// Configs based on settings on nextion
String linkedSwitch = "";
String channel = "";
int triggerTemp = -1;
int hysterisisPercentage = -1;
bool isCheckingHysterisis = false;

void tempDetectTask(void *params)
{
  float temperature;
  float hysterisisTemp;

  while (true)
  {
    bool isConfigured = false;
    if (linkedSwitch != "" && triggerTemp != -1 && hysterisisPercentage != -1 && channel != "")
    {
      isConfigured = true;
    }

    if (!isConfigured)
    {
      Serial.println("Temp Sensor not configured. Waiting...");
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Yield for watchdog reset
      continue;
    }

    if (!isEnabled)
    {
      Serial.println("Temp Sensor not enabled");
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Yield for watchdog reset
      continue;
    }

    temperature = dht.getTemperature();
    hysterisisTemp = (float)triggerTemp - ((float)triggerTemp * (float)hysterisisPercentage / 100);
    // Serial.println("Temp:" + temperature);
    Serial.print("Trigger: ");
    Serial.println(triggerTemp);
    Serial.print("Hysterisis: ");
    Serial.println(hysterisisTemp);
    Serial.print("Current Temp: ");
    Serial.println(temperature);

    String temp = String(temperature);

    char dataToSend[32] = "";
    strcpy(dataToSend, ("TEMPSENSORCURRENTTEMP" + temp).c_str());

    esp_err_t result = esp_now_send(senderMac, (uint8_t *)dataToSend, sizeof(dataToSend));

    if (floor(temperature) >= triggerTemp && !isCheckingHysterisis)
    {
      char dataToSend[32] = "";
      strcpy(dataToSend, (linkedSwitch + "ON" + channel).c_str());

      esp_err_t result = esp_now_send(senderMac, (uint8_t *)dataToSend, sizeof(dataToSend));

      if (result == ESP_OK)
      {
        Serial.println("Turning on" + linkedSwitch + "confirmed");
      }
      else
      {
        Serial.printf("Turning on Temp linked SW error: %d\n", result);
      }

      isCheckingHysterisis = true;
    }

    if (isCheckingHysterisis && temperature < hysterisisTemp)
    {
      Serial.println("Turn Switch off");
      char dataToSend[32] = "";
      strcpy(dataToSend, (linkedSwitch + "OFF").c_str());

      esp_err_t result = esp_now_send(senderMac, (uint8_t *)dataToSend, sizeof(dataToSend));

      if (result == ESP_OK)
      {
        Serial.println("Turning off" + linkedSwitch + "confirmed");
      }
      else
      {
        Serial.printf("Turning off Temp linked SW error: %d\n", result);
      }
      isCheckingHysterisis = false;
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  pinMode(LED, OUTPUT);
  dht.setup(DHT, DHTesp::DHT22);
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait to pass unstable status (see documentation)
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.channel(1);
  esp_now_init();

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  xTaskCreatePinnedToCore(tempDetectTask, "Temperature detection", 2048, NULL, 1, NULL, APP_CPU_NUM);

  vTaskDelete(NULL); // Delete main task as it is not used
}

void loop()
{
  delay(dht.getMinimumSamplingPeriod()); // 2000 ms for DHT22

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
  delay(2000);
}
