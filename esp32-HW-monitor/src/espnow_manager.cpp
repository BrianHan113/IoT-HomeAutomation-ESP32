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

    // MACSEND header is sent by receiver ESPs when SCANESPS button is pressed
    if (strncmp(receivedData, "MACSEND", strlen("MACSEND")) == 0)
    {
        String receivedString = String(receivedData);
        String id = receivedString.substring(strlen("MACSEND"), len);

        std::array<uint8_t, 6> macValue;
        memcpy(macValue.data(), mac, 6);
        deviceMACMap[id] = macValue;        // Save the EspID and MAC to deviceMACMap
        espNowAddReceiver(macValue.data()); // Pair the receiver ESP
    }
    else // All other received data will be commands sent by receiver ESPs (ie SW1OFF)
    {
        String receivedString = String(receivedData);
        for (int i = 0; i < numSwitches; i++) // Find the switch that the command is for
        {
            String currentSwitch = (String)switches[i];
            if (receivedString.startsWith(currentSwitch))
            {
                String SW = currentSwitch;
                String action = receivedString.substring(SW.length());

                // Update the GUI on Nextion main screen based on the action
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
        // Push the command to commandQueue for executeCommands Task to handle
        if (xQueueSend(commandQueue, &receivedData, 0) != pdTRUE)
        {
            Serial.println("Queue full");
        }
    }
}