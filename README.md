# IoT-HomeAutomation-ESP32

IoT Home Automation system using ESP32s to control various peripherals, developed with amazing help and under supervision of Alex Risos from Risos Enterprises for my summer 2024-25 internship. This project comes with pre-made examples of peripherals, but the esp32-Receiver-Firmware can be flashed and easily modified on any ESP32 for any custom functionality. There are 2 versions of the project:

<table align="center">
  <tr>
    <th style="text-align:center;">Simplified</th>
    <th style="text-align:center;">Full</th>
  </tr>
  <tr>
    <td align="center"><img src="readme_imgs/simplified.jpg" width="400"></td>
    <td align="center"><img src="readme_imgs/full.jpg" width="400"></td>
  </tr>
</table>




## Summary of features
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

## How to Use
### Getting started
1. Flash esp32-HW-monitor on central esp32-s3
2. Flash as many receiver esps as you would like
3. Connect COM port of esp32-s3 to a USB port on your computer
4. Power the nextion, make sure the esp32-s3 and the nextion share a common ground
5. Start the ArduinoWeatherHardwareMonitor desktop app, make sure to run as admin
6. Select the appropriate serial port by right clicking the tray icon

At this point, the waveforms and weather information should have loaded on the main page, the LOCKPC button and the music player should be working (Fill the music folder of the repo with your music files, any format supported by WinAmp). Note that Winamp is not necessary if you do not want the music player functionality, just press cancel when prompted to find the install directory on the desktop app.

### Linking and Turning Peripherals on/off
<div style="display: flex; align-items: center;">
  <div style="flex: 1;">
    <ol>
      <li>Press <strong>SETTINGS</strong>, then <strong>SCANESPS</strong> in the top left. Wait a couple of seconds for the peripherals to connect.</li>
      <li>Link a switch to any peripheral and hit <strong>APPLY</strong> on the bottom left.</li>
      <li>Exit back to the main page. You should now be able to turn that peripheral on/off using the switch you selected.</li>
    </ol>
    <p>Each switch has 4 states which go in sequence of ONA, ONAB, ONB, OFF. This allows each peripheral to handle 2 different actions, A and B, where ONAB is turning on both at the same time. This synergizes with usage of 2 channel SSRs, as seen in esp32-SSR. </p> <p>For the standard esp32/esp8266-Receiver-Firmware, all three ON states will just turn the builtin LED on, but this can be easily modified.</p>
  </div>
  <div style="flex: 1; text-align: center;">
    <div>
      <h4>Linking Switches</h4>
      <img src="readme_imgs/settings1.PNG" width="300">
    </div>
    <div>
      <h4>4 Switch States in order</h4>
      <img src="readme_imgs/switches.PNG" width="300">
    </div>
  </div>
</div>

## Pre-made Peripherals


## Known Issues
* Camera capture is very slow, around 10-15 seconds just for a single still image, and during this all buttons are unresponsive. It "works", but is generally pretty unusable, and hence has been disabled. It can easily be enabled by uncommenting code in the setup() function in the main file of HW-Monitor and filling in secret files in the HW-Monitor and CAM projects for WiFi SSID and Password.
* Tide data is also excluded in the simplified version, as the API used for the data (Stormglass.io) is unstable and could likely get rid of their free tier pricing. If stormglass is still free when reading this, or you have a paid api key, you can simply uncomment the task in the setup() function in the main file of HW-Monitor where the tasks are created, and provide your api key in the secret file in the ArduinoHardwareWeatherMonitor C# app.
* Rarely in settings, one or two selection slides for esps will not update upon pressing SCANESPS. Just press SCANESPS again, and it should update.

## Attributions
* Alex Risos, Risos Enterprises