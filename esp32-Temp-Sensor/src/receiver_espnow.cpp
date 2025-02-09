#include <receiver_espnow.h>

boolean isCentralEspPaired = false;
extern String EspID;
extern uint8_t senderMac[6];

extern String linkedSwitch;
extern String channel;
extern bool isEnabled;
extern int triggerTemp;
extern int hysterisisPercentage;
extern bool isCheckingHysterisis;

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
    memcpy(senderMac, mac, sizeof(senderMac));
    if (strncmp(receivedData, "GETMACADDRESS", strlen("GETMACADDRESS")) == 0)
    {
        espNowAddReceiver(mac);
        Serial.println("Sending MAC");
        char dataToSend[32];
        strcpy(dataToSend, "MACSEND");
        // strcat(dataToSend, WiFi.macAddress().c_str());
        strcat(dataToSend, EspID.c_str());
        esp_now_send(mac, (const uint8_t *)dataToSend, sizeof(dataToSend));
    }
    else if (strncmp(receivedData, "ON", strlen("ON")) == 0)
    {
        digitalWrite(2, HIGH);
    }
    else if (strncmp(receivedData, "OFF", strlen("ON")) == 0)
    {
        digitalWrite(2, LOW);
    }
    else if (strncmp(receivedData, "SW", strlen("SW")) == 0)
    {
        linkedSwitch = receivedData;
        Serial.println("Linked Switch: " + linkedSwitch);
    }
    else if (strncmp(receivedData, "TRIGGER", strlen("TRIGGER")) == 0)
    {
        String dataString = ((String)receivedData).substring(strlen("TRIGGER"));
        triggerTemp = dataString.toInt();
        Serial.println("Trigger temp: " + (String)triggerTemp);
        isCheckingHysterisis = false; // On reconfig, reset to starting state
    }
    else if (strncmp(receivedData, "HYSTERISIS", strlen("HYSTERISIS")) == 0)
    {
        String dataString = ((String)receivedData).substring(strlen("HYSTERISIS"));
        hysterisisPercentage = dataString.toInt();

        Serial.println("Hysterisis: " + (String)hysterisisPercentage);
    }
    else if (strncmp(receivedData, "ENABLE", strlen("ENABLE")) == 0)
    {
        isEnabled = true;
    }
    else if (strncmp(receivedData, "DISABLE", strlen("DISABLE")) == 0)
    {
        isEnabled = false;
    }
    else if (strncmp(receivedData, "CHANNEL", strlen("CHANNEL")) == 0)
    {
        channel = ((String)receivedData).substring(strlen("CHANNEL"));
        Serial.println("Channel: " + channel);
    }
}

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}