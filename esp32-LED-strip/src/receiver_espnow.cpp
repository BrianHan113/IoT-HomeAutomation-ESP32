// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#include <receiver_espnow.h>

boolean isCentralEspPaired = false;
extern String EspID;
extern uint8_t red;
extern uint8_t green;
extern uint8_t blue;

extern bool isStripOn;
extern int NUM_LEDS;

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
        // strcat(dataToSend, WiFi.macAddress().c_str());
        strcat(dataToSend, EspID.c_str());
        esp_now_send(mac, (const uint8_t *)dataToSend, sizeof(dataToSend));
    }
    else if (strncmp(receivedData, "ON", strlen("ON")) == 0)
    {
        isStripOn = true;
    }
    else if (strncmp(receivedData, "OFF", strlen("OFF")) == 0)
    {
        isStripOn = false;
    }
    else if (String(receivedData).startsWith("NUMLEDS"))
    {
        NUM_LEDS = String(receivedData).substring(7).toInt();
    }
    else if (String(receivedData).startsWith("COLOUR"))
    {
        if (String(receivedData).startsWith("COLOURRED"))
        {
            int redVal = String(receivedData).substring(9).toInt();
            // Serial.println(redVal);
            red = (redVal * 255) / 31;
        }
        else if (String(receivedData).startsWith("COLOURGREEN"))
        {
            int greenVal = String(receivedData).substring(11).toInt();
            green = (greenVal * 255) / 63;
        }
        else if (String(receivedData).startsWith("COLOURBLUE"))
        {
            int blueVal = String(receivedData).substring(10).toInt();
            blue = (blueVal * 255) / 31;
        }
    }
}

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}