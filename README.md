# IoT-HomeAutomation-ESP32

IoT Home Automation system using ESP32s to control various peripherals. This project comes with pre-made examples of peripherals, but the esp32-Receiver-Firmware can be flashed and modified on any ESP32 for any custom functionality you want. 

Summary of features:
* Detection and pairing with powered ESPs that have the receiver firmware programmed
* Link any paired ESP to any of 8 switches on nextion display, can link multiple ESPs on one switch
* Control of 2-channel low-level solid state relays
* Quadruple state switches for interacting with SSRs (A, B, A+B or OFF)
* Control of LED strips (Status-LED which shows a colour dependant on Gpu Temperature, and LED-Strip for colour picking using RGB sliders on nextion GUI)
* Trigger switches using a Motion Sensor
* Trigger switches using a Temperature Sensor
* Scheduling functionality for Switches and Sensors
* 2 Camera slots for image captures
* Weather information with adjustable time range
* Tide extreme and current height information shown on a nice visual
* CPU Temperature, GPU Temperature, Ram Usage and Power consumption shown on nextion display as waveforms
* Music player
* Lock PC button
