#include <receiver_espnow.h>

boolean isCentralEspPaired = false;
extern String EspID;

void espNowAddReceiver(const uint8_t *receiver)
{
    if (esp_now_is_peer_exist((u8 *)receiver))
    {
        Serial.println("Peer already exists");
        return;
    }

    if (esp_now_add_peer((uint8_t *)receiver, ESP_NOW_ROLE_SLAVE, 0, NULL, 0) != 0)
    {
        Serial.println("Failed to add peer");
    }
    else
    {
        Serial.println("Paired success");
    }
}

void OnDataRecv(u8 *mac, u8 *data, u8 len)
{
    char receivedData[32];
    memcpy(&receivedData, data, sizeof(receivedData));

    Serial.println(receivedData);

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
        strcat(dataToSend, EspID.c_str());
        esp_now_send(mac, (u8 *)dataToSend, sizeof(dataToSend));
    }
    else if (strncmp(receivedData, "ON", strlen("ON")) == 0)
    {
        digitalWrite(2, LOW);
    }
    else if (strncmp(receivedData, "OFF", strlen("OFF")) == 0)
    {
        digitalWrite(2, HIGH);
    }
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
    Serial.println(sendStatus == 0 ? "Success" : "Fail");
}
