#include "addresses_helper.h"

std::map<String, std::array<uint8_t, 6>> deviceMACMap; // Maps EspID to MAC Address
const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const String switches[] = {
    "SW1",
    "SW2",
    "SW3",
    "SW4",
    "SW5",
    "SW6",
    "SW7",
    "SW8",
    "TEMPSENSOR",
    "MOTIONSENSOR",
}; // Available options on nextion settings page

// Sensor and peripheral links
String tempSensorDevice = "";
String tempSensorSwitch = "";
String motionSensorDevice = "";
String motionSensorSwitch = "";
String ledStripDevice = "";
String statusLedDevice = "";

const int numSwitches = sizeof(switches) / sizeof(switches[0]);

std::map<String, std::vector<String>> switchDeviceMap; // Maps settings switch to a Device

void displayMacAddress(std::array<uint8_t, 6> macValue)
{
    for (size_t i = 0; i < macValue.size(); i++)
    {
        if (i > 0)
        {
            Serial.print(":");
        }
        Serial.print(macValue[i], HEX);
    }
    Serial.println();
}

void getMacAddresses()
{
    deviceMACMap.clear();
    const char *message = "GETMACADDRESS";
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)message, strlen(message));

    if (result == ESP_OK)
    {
        Serial.println("Sent GETMACADDRESS command");
    }
    else
    {
        Serial.println("No ESPS powered/detected");
    }
}

void displaySwitchDeviceMap()
{
    for (auto it = switchDeviceMap.begin(); it != switchDeviceMap.end(); ++it)
    {
        Serial.print(it->first + ": ");
        for (const auto &device : it->second)
        {
            Serial.print(device + " ");
        }
        Serial.println();
    }
}