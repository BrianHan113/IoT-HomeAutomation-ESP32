// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#include "addresses_helper.h"

std::map<String, std::array<uint8_t, 6>> deviceMACMap;                   // Saves EspIDs to MAC Address, e.g. "LIGHT1" -> {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB}
const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Special MAC address to send to all nearby ESPs
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
}; // All available selection options on nextion settings page

// Sensor and peripheral devices
String tempSensorDevice = "";
String tempSensorSwitch = "";
String motionSensorDevice = "";
String motionSensorSwitch = "";
String ledStripDevice = "";
String statusLedDevice = "";

const int numSwitches = sizeof(switches) / sizeof(switches[0]); // Handy for iterating over all switches in for-loop
std::map<String, std::vector<String>> switchDeviceMap;          // Saves switches to all linked EspIDs, e.g. "SW1" -> {"LIGHT1", "LIGHT2"}

// Send a message to all ESPs telling them to send their EspIDs, and their MAC addresses
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

// Helper debugging functions
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