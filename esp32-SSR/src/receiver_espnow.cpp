#include <receiver_espnow.h>

boolean isCentralEspPaired = false;
extern String EspID;
extern int CH1_PIN;
extern int CH2_PIN;

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
    else if (strncmp(receivedData, "ONAB", strlen("ONAB")) == 0)
    {
        Serial.println("ONAB");
        digitalWrite(CH1_PIN, LOW);
        digitalWrite(CH2_PIN, LOW);
    }
    else if (strncmp(receivedData, "ONA", strlen("ONA")) == 0)
    {
        Serial.println("ONA");
        digitalWrite(CH1_PIN, LOW);
        digitalWrite(CH2_PIN, HIGH);
    }
    else if (strncmp(receivedData, "ONB", strlen("ONB")) == 0)
    {
        Serial.println("ONB");
        digitalWrite(CH1_PIN, HIGH);
        digitalWrite(CH2_PIN, LOW);
    }
    else if (strncmp(receivedData, "OFF", strlen("ON")) == 0)
    {
        Serial.println("OFF");
        digitalWrite(CH1_PIN, HIGH);
        digitalWrite(CH2_PIN, HIGH);
    }
}

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}