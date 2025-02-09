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
* Quadruple state switches with actions of A, B, A+B or OFF
* Control of LED strips (Status-LED which shows a colour dependant on Gpu Temperature, and LED-Strip for colour picking using RGB sliders on nextion GUI)
* Trigger switches using a Motion Sensor
* Trigger switches using a Temperature Sensor
* Trigger switches and/or enable sensors with a scheduler
* 2 Camera slots for image captures (Not in simplified version)
* Weather information with adjustable time range
* Tide extremes and current tide height information (Not in simplified version)
* CPU Temperature, GPU Temperature, Ram Usage and Power consumption shown on nextion display as waveforms
* Music player
* Lock PC button

## How to Use
### Getting started
1. Flash esp32-HW-monitor on central esp32-s3
2. Flash as many receiver esps as you would like
3. Connect COM port of esp32-s3 to a USB port on your computer
4. Flash whichever .HMI file (simplified or full) in nextion-HMI onto the nextion 
5. Make sure the esp32-s3 and the nextion share a common ground
6. Start the ArduinoWeatherHardwareMonitor desktop app, make sure to run as admin
7. Select the appropriate serial port by right clicking the tray icon

At this point, the waveforms and weather information should have loaded on the main page, the LOCKPC button and the music player should be working. Right click the tray icon to select your WinAmp installation folder and your music folder. You will only have to do this once, unless you change your WinAmp path or your music folder. The music player plays MP3, MIDI, MOD, MPEG-1, AAC, M4A, FLAC, WAV, WMA, Ogg Vorbis.  

### Linking and Turning Peripherals on/off
<table align="center">
  <tr>
    <td><div>
    <ol>
      <li>Press <strong>SETTINGS</strong>, then <strong>SCANESPS</strong> in the top left. Wait a couple of seconds for the peripherals to connect.</li>
      <li>Link a switch to any peripheral and hit <strong>APPLY</strong> on the bottom left.</li>
      <li>Exit back to the main page. You should now be able to turn that peripheral on/off using the switch you selected.</li>
      <li>To unlink any peripheral, press the <strong>X</strong> to the right of the selection to clear, and press <strong>APPLY</strong>.</li>
    </ol>
    <p>Each switch has 4 states which go in sequence of ONA, ONAB, ONB, OFF. This allows each peripheral to handle 2 different actions, A and B, where ONAB is turning on both at the same time. This synergizes with usage of 2 channel SSRs, as seen in esp32-SSR. </p> <p>For the standard esp32/esp8266-Receiver-Firmware, all three ON states will just turn the builtin LED on, but this can be easily modified.</p>
  </div></td>
    <td align="center"><div style="align-items: center;">
  <div>
    <div>
      <h4>Linking Switches</h4>
      <img src="readme_imgs/settings1.PNG" width="1000">
    </div>
    <div>
      <h4>4 Switch States in order</h4>
      <img src="readme_imgs/switches.PNG" width="1000">
    </div>
  </div>
</div></td>
  </tr>
</table>

### Temperature Sensor
<table align="center">
  <tr>
    <td><div>
    <ol>
      <li><strong>Select ESP</strong>: The device that is connected to the temperature sensor</li>
      <li><strong>Select SW</strong>: The Switch that should be triggered when the temperature exceeds the trigger temperature</li>
      <li><strong>Channel selection</strong>: Which channel of the switch should be turned on (A, B, AB)</li>
      <li><strong>Trigger SW at</strong>: The temperature threshold, in celcius, where anything that exceeds it will trigger the connected switch</li>
      <li><strong>Hysterisis</strong>: What percentage of the trigger temperature does the measured temperature need to fall below in order to turn the switch off. E.g., if trigger temp is 50C, and hystersis is 50%, when triggered, the measured temperature must fall below 25C in order for the switch to turn off automatically.</li>
    </ol>
    <p>After selecting these settings, you must hit <strong>APPLY</strong> and navigate back to the main screen, and press <strong>TEMP EN</strong> to enable the sensor to make automatic changes. If it any point you wish to override the sensor/ignore it, just press <strong>TEMP EN</strong> again to disable it.</p>
  </div></td>
    <td align="center"><div style="align-items: center;">
  <div>
    <div>
      <h4>Temperature Sensor Channels</h4>
      <img src="readme_imgs/tempsensor.PNG" width="1000">
    </div>
  </div>
</div></td>
  </tr>
</table>

### Motion Sensor

### Scheduler

### LED Strip

### Status LED Strip

### Weather Settings


## Pre-made Peripherals


## Known Issues
* Camera capture is very slow, around 10-15 seconds just for a single still image, and during this all buttons are unresponsive. It "works", but is generally pretty unusable, and hence has been disabled. It can easily be enabled by uncommenting code in the setup() function in the main file of HW-Monitor and filling in secret files in the HW-Monitor and CAM projects for WiFi SSID and Password.
* Tide data is also excluded in the simplified version, as the API used for the data (Stormglass.io) is unstable and could likely get rid of their free tier pricing. If stormglass is still free when reading this, or you have a paid api key, you can simply uncomment the task in the setup() function in the main file of HW-Monitor where the tasks are created, and provide your api key in the secret file in the ArduinoHardwareWeatherMonitor C# app.
* Rarely in settings, one or two selection slides for esps will not update upon pressing SCANESPS. Just press SCANESPS again, and it should update.

## Attributions
* Alex Risos, Risos Enterprises