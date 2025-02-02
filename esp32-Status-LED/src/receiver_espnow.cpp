#include <receiver_espnow.h>

boolean isCentralEspPaired = false;
extern String EspID;
extern uint8_t red;
extern uint8_t green;
extern uint8_t blue;

extern bool isStripOn;
extern int NUM_LEDS;
extern float hue;

void espNowAddReceiver(const uint8_t *receiver)
{
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiver, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
    }
    else
    {
        Serial.println("paired success");
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{

    char receivedData[32];
    Serial.println(receivedData);
    memcpy(&receivedData, incomingData, sizeof(receivedData));

    if (strncmp(receivedData, "GETMACADDRESS", strlen("GETMACADDRESS")) == 0)
    {
        if (!isCentralEspPaired)
        {
            espNowAddReceiver(mac);
            isCentralEspPaired = true;
        }
        Serial.println("Sending MAC");
        char dataToSend[32];
        strcpy(dataToSend, "MACSEND");
        // strcat(dataToSend, WiFi.macAddress().c_str());
        strcat(dataToSend, EspID.c_str());
        esp_now_send(mac, (const uint8_t *)dataToSend, sizeof(dataToSend));
    }
    else if (strncmp(receivedData, "ON", strlen("ON")) == 0)
    {
        isStripOn = true;
    }
    else if (strncmp(receivedData, "OFF", strlen("ON")) == 0)
    {
        isStripOn = false;
    }
    else if (String(receivedData).startsWith("NUMLEDS"))
    {
        NUM_LEDS = String(receivedData).substring(7).toInt();
    }
    else if (String(receivedData).startsWith("GPUTEMP"))
    {
        int gpuTemp = String(receivedData).substring(7).toInt();
        Serial.println(gpuTemp);
        // gpuTemp = 1000; //manually set for testing
        float clampedTemp = fmax(30, fmin(100, gpuTemp));

        // map(value, fromLow, fromHigh, toLow, toHigh) -> rescales our gpu temp to 160-0 scale
        hue = map(clampedTemp, 30, 100, 128, 0);
    }
}

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}