#include "espnow_manager.h"
extern QueueHandle_t commandQueue;
extern HardwareSerial nextion;

void espNowAddReceiver(const uint8_t *receiver)
{
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiver, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer or already added");
    }
    else
    {
        Serial.println("paired success");
    }
}

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status)
{
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    char receivedData[32];
    memcpy(&receivedData, incomingData, sizeof(receivedData));

    if (strncmp(receivedData, "MACSEND", strlen("MACSEND")) == 0)
    {
        String receivedString = String(receivedData);
        String id = receivedString.substring(strlen("MACSEND"), len);

        std::array<uint8_t, 6> macValue;
        memcpy(macValue.data(), mac, 6);
        deviceMACMap[id] = macValue;
        espNowAddReceiver(macValue.data());
    }
    else // Used to push commands sent by eps-now from sensors to commandQueue, ie SW1OFF
    {
        String receivedString = String(receivedData);
        for (int i = 0; i < numSwitches; i++)
        {
            String currentSwitch = (String)switches[i];
            if (receivedString.startsWith(currentSwitch))
            {
                String SW = currentSwitch;
                String action = receivedString.substring(SW.length());

                if (action == "ONA")
                {
                    queueNextionCommand("main." + SW + ".val=1");
                    queueNextionCommand("main." + SW + ".bco2=1024");
                    queueNextionCommand("main." + SW + "Val.val=1");
                }
                else if (action == "ONAB")
                {
                    queueNextionCommand("main." + SW + ".val=1");
                    queueNextionCommand("main." + SW + ".bco2=1527");
                    queueNextionCommand("main." + SW + "Val.val=2");
                }
                else if (action == "ONB")
                {
                    queueNextionCommand("main." + SW + ".val=1");
                    queueNextionCommand("main." + SW + ".bco2=1048");
                    queueNextionCommand("main." + SW + "Val.val=3");
                }
                else if (action == "OFF")
                {
                    queueNextionCommand("main." + SW + ".val=0");
                    queueNextionCommand("main." + SW + ".bco2=1024");
                    queueNextionCommand("main." + SW + "Val.val=0");
                }
                break;
            }
        }
        if (xQueueSend(commandQueue, &receivedData, 0) != pdTRUE)
        {
            Serial.println("Queue full");
        }
    }
}