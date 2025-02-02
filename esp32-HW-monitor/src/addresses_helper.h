#ifndef ADDRESSES_HELPER_H
#define ADDRESSES_HELPER_H

#include <Arduino.h>
#include <string>
#include <vector>
#include <esp_now.h>
#include <map>

extern std::map<String, std::array<uint8_t, 6>> deviceMACMap;
extern std::map<String, std::vector<String>> switchDeviceMap;
extern const uint8_t broadcastAddress[];
extern const String switches[];
extern const int numSwitches;
extern String tempSensorDevice;
extern String tempSensorSwitch;
extern String motionSensorDevice;
extern String motionSensorSwitch;
extern String ledStripDevice;
extern String statusLedDevice;
void getMacAddresses();
void displaySwitchDeviceMap();
void displayMacAddress(std::array<uint8_t, 6> macValue);
#endif