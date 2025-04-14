// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

#include "tasks.h"

extern SemaphoreHandle_t hardwareDataSemaphore;
extern SemaphoreHandle_t weatherBinSemaphore;
extern SemaphoreHandle_t tideBinSemaphore;
extern uint8_t receiverAddress[];
extern HardwareSerial nextion;
extern QueueHandle_t commandQueue;
extern QueueHandle_t sendNextionQueue;
extern std::vector<std::string> commandPrefixes;
extern TaskHandle_t sendTempDataHandle;
String hardwareData;
String weatherData;
String tideData;

const int BUFFER_SIZE = 500;

// Handle data sent by serial from the c# desktop app
void receiveHardwareData(void *params)
{

    static char buffer[BUFFER_SIZE];
    static int bufferIndex = 0;

    String receivedData;
    String splicedData;
    receivedData.reserve(BUFFER_SIZE);
    splicedData.reserve(BUFFER_SIZE);
    hardwareData.reserve(BUFFER_SIZE);
    weatherData.reserve(BUFFER_SIZE);
    tideData.reserve(BUFFER_SIZE);

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("Stack rec HW Data: " + String(stackLeft));

        if (Serial.available() > 0)
        {
            receivedData = Serial.readStringUntil(0x03); // 0x03 is the termination character

            // Serial.println(receivedData); // Testing code

            if (receivedData.startsWith("HARDWARE")) // CPU, GPU, RAM, Power data
            {
                splicedData = receivedData.substring(8);
                hardwareData = splicedData;
                // Give twice so both sendHardwareData and sendTempData tasks can use the data
                xSemaphoreGive(hardwareDataSemaphore);
                xSemaphoreGive(hardwareDataSemaphore);
            }
            else if (receivedData.startsWith("WEATHER"))
            {
                if (receivedData.startsWith("WEATHERLOCATION")) // Lat, Long datas
                {
                    String location = receivedData.substring(15);
                    queueNextionCommand("main.locationText.txt=\"" + location + "\"");
                }
                else
                {
                    splicedData = receivedData.substring(7); // Weather forecast data
                    weatherData = splicedData;
                    xSemaphoreGive(weatherBinSemaphore);
                }
            }
            else if (receivedData.startsWith("TIDE")) // Low High tide data, and current time data
            {
                splicedData = receivedData.substring(4);
                tideData = splicedData;
                xSemaphoreGive(tideBinSemaphore);
            }
            else if (receivedData.startsWith("MUSICSTRING")) // Framed string of all songs in the selected music folder
            {
                splicedData = receivedData.substring(11);
                Serial.println(splicedData);
                splicedData.replace("BREAK", "\\r");
                // Cant queue music string as it may be too long to store
                // Hence, this may not work sometimes, but it's a rare case, and users can easily just refresh again
                sendNextionCommand("main.musicList.path=\"\""); // Make it blank For Visual confirmation of refresh
                sendNextionCommand("main.musicList.path=\"" + splicedData + "\"");
            }
            else if (receivedData.startsWith("SCHEDULE")) // Commands requested by the c# scheduler
            {
                String receivedString = receivedData.substring(8);
                // Serial.println("Testing: " + receivedString);
                for (int i = 0; i < numSwitches; i++)
                {
                    String currentSwitch = (String)switches[i];
                    if (receivedString.startsWith(currentSwitch))
                    {
                        String SW = currentSwitch;
                        String action = receivedString.substring(SW.length());

                        // Update GUI on nextion main screen based on the action, this is just visual, the actual action is handled by the executeCommands task
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
                        else if (action == "ENABLE")
                        {
                            queueNextionCommand("main." + SW + ".val=1");
                        }
                        else if (action == "DISABLE")
                        {
                            queueNextionCommand("main." + SW + ".val=0");
                        }
                        break;
                    }
                }
                // Push the command to commandQueue for executeCommands Task to handle
                if (xQueueSend(commandQueue, receivedString.c_str(), 0) != pdTRUE)
                {
                    Serial.println("Queue full");
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// Handle commands sent by the nextion display
void receiveNextionSerial(void *params)
{

    static char buffer[BUFFER_SIZE];
    static char miniBuffer[4]; // Store previous 4 characters to check for NUMS or NUME
    static int bufferIndex = 0;
    static int miniBufferIndex = 0; // Index for miniBuffer
    bool isProcessingNum = false;
    int currBufferSize = 0;

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("Stack rec nextion serial: " + String(stackLeft));

        if (nextion.available())
        {

            // Project follows rule that number data is sent between NUMS and NUME to avoid confusion with 0x03 termination char
            if (miniBuffer[0] == 'N' && miniBuffer[1] == 'U' && miniBuffer[2] == 'M' && miniBuffer[3] == 'S')
            {
                isProcessingNum = true;
            }
            else if (miniBuffer[0] == 'N' && miniBuffer[1] == 'U' && miniBuffer[2] == 'M' && miniBuffer[3] == 'E')
            {
                isProcessingNum = false;
            }

            uint8_t byteReceived = nextion.read();
            if (!isProcessingNum)
            {
                if ((byteReceived < 0x21 || byteReceived > 0x7E) && byteReceived != 0x03 || byteReceived == 0x24) // Confirmation bytes sent automatically by nextion, ingore them
                {
                    continue;
                }
            }

            if (byteReceived != 0x03 && byteReceived != 0x00)
            {
                if (miniBufferIndex < 4)
                {
                    miniBuffer[miniBufferIndex] = static_cast<char>(byteReceived);
                    miniBufferIndex++;
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                    {
                        miniBuffer[i] = miniBuffer[i + 1];
                    }
                    miniBuffer[3] = static_cast<char>(byteReceived);
                }

                char receivedChar = static_cast<char>(byteReceived);

                if (bufferIndex < BUFFER_SIZE - 1)
                {
                    buffer[bufferIndex] = receivedChar;
                    bufferIndex++;
                    currBufferSize++;
                }
            }

            if (!isProcessingNum && byteReceived == 0x03)
            {
                // const char *invalid = "�";
                char startingHexValue[5];
                sprintf(startingHexValue, "0x%02X", buffer[0]);

                // Invalid first character
                if (strcmp(startingHexValue, "0xFF") == 0 || strcmp(startingHexValue, "0xFE") == 0)
                {
                    memmove(buffer, buffer + 1, currBufferSize - 1);
                    buffer[currBufferSize - 1] = '\0';
                }

                if (xQueueSend(commandQueue, &buffer, 0) != pdTRUE)
                {
                    Serial.println("From receiveNextionSerial: Queue full");
                }
                else
                {
                    Serial.println("From receieveNextionSerial: " + String(buffer));
                }

                // Reset both buffers for next command
                bufferIndex = 0;
                memset(buffer, 0, BUFFER_SIZE);
                currBufferSize = 0;

                miniBufferIndex = 0;
                memset(miniBuffer, 0, 4);
            }
            // Treat 0x03 as data, not as a termination char
            else if (isProcessingNum && byteReceived == 0x03)
            {
                char receivedChar = static_cast<char>(byteReceived);
                if (bufferIndex < BUFFER_SIZE - 1)
                {
                    buffer[bufferIndex] = receivedChar;
                    bufferIndex++;
                }
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

// Send commands to the nextion display
void sendNextionSerial(void *pvParameters)
{
    char command[100];

    while (true)
    {
        if (xQueueReceive(sendNextionQueue, &command, portMAX_DELAY) != pdTRUE)
        {
            Serial.println("Nothing in queue");
            return;
        }

        // Serial.println("Sending to Nextion: " + String(command));
        nextion.print(String(command));
        nextion.write(0xFF);
        nextion.write(0xFF);
        nextion.write(0xFF);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Execute commands sent to commandQueue from all sources (nextion, c# app, esp-now)
void executeCommands(void *params)
{

    char command[BUFFER_SIZE];

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("Stack executecmds: " + String(stackLeft));

        if (xQueueReceive(commandQueue, &command, portMAX_DELAY) != pdTRUE)
        {
            Serial.println("Nothing in queue");
            return;
        }

        String commandString = (String)command;

        if (commandString == "RESETSETTINGS")
        {
            switchDeviceMap.clear();
        }
        else if (commandString == "LOCKPC")
        {
            Serial.println("Lock PC");
            Serial.println("LOCKPC"); // Redirect command to c# app
        }
        else if (commandString.startsWith("SCANESPS"))
        {
            deviceMACMap.clear();
            Serial.println("Scanning Esps");
            getMacAddresses();                     // Send broadcast to all espnow devices to get their mac addresses
            vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for responses from espnow

            // Build a multiline string of all the detected and paired ESP devices
            String multilineString = "";
            for (const auto &pair : deviceMACMap)
            {
                if (!multilineString.isEmpty())
                {
                    multilineString += "\\r";
                }
                multilineString += pair.first;
            }

            // Send the multiline string to all the select esp dropdowns on the nextion display
            for (int i = 0; i <= 19; i++) // 20 total select esp dropdowns on nextion
            {
                queueNextionCommand("select" + String(i) + ".path=\"" + multilineString + "\"");
            }

            String selectedEspsString = commandString.substring(8); // Framed string of previously selected ESPs in settings
            // Serial.println(selectedEspsString);

            // Parse the selectedEspsString to check if the selected ESPs are still available, if they are not, display "Select Esp" on the nextion display to clear it
            // This keeps all previous selections, only clearing the ESPs that are no longer available
            int startIndex = 0;
            while (true)
            {
                int selectIndex = selectedEspsString.indexOf("select", startIndex);
                if (selectIndex == -1)
                {
                    break;
                }

                int hasIndex = selectedEspsString.indexOf("HAS", selectIndex);
                int breakIndex = selectedEspsString.indexOf("BREAK", selectIndex);

                String selected = selectedEspsString.substring(selectIndex, hasIndex);
                String espId = selectedEspsString.substring(hasIndex + 3, breakIndex);

                if (deviceMACMap.find(espId) == deviceMACMap.end()) // espId not found in new deviceMacMap
                {
                    queueNextionCommand(selected + ".txt=\"Select Esp\"");
                }

                startIndex = breakIndex + 5;
            }
        }
        else if (commandString.startsWith("SW")) // For commands like "SW1ONAB", "SW7OFF", etc
        {
            for (int i = 0; i < numSwitches; i++)
            {
                String currentSwitch = (String)switches[i];
                if (commandString.startsWith(currentSwitch))
                {
                    if (switchDeviceMap.find(currentSwitch) != switchDeviceMap.end())
                    {
                        String command = commandString.substring(currentSwitch.length());
                        std::vector<String> devices = switchDeviceMap[currentSwitch];

                        // Send the ON/OFF command to all devices linked to the switch
                        for (const auto &device : devices)
                        {
                            sendCommandToDevice(command, device);
                        }
                    }
                    break;
                }
            }
        }
        else if (commandString.startsWith("LINK")) // Linking switches to devices
        {
            String withoutCommand = commandString.substring(4);

            for (int i = 0; i < numSwitches; i++)
            {
                String currentSwitch = (String)switches[i];
                if (withoutCommand.startsWith(currentSwitch))
                {
                    String device = withoutCommand.substring(currentSwitch.length());
                    switchDeviceMap[currentSwitch].push_back(device); // Add device to the list of devices for the switch
                    break;
                }
            }
            // displaySwitchDeviceMap();
        }
        else if (commandString.startsWith("SCHEDULE"))
        {
            // Serial.println("Sending to desktop app");
            Serial.println(commandString);
        }
        else if (commandString.startsWith("WEATHERDELTA"))
        {
            String delta = commandString.substring(12);
            // Serial.println("Sending to desktop app");
            Serial.println(commandString);
        }
        else if (commandString.startsWith("LEDSTRIP"))
        {
            String withoutCommand = commandString.substring(8);

            if (withoutCommand.startsWith("LINK"))
            {
                ledStripDevice = withoutCommand.substring(4);
            }
            else if (withoutCommand.startsWith("NUMLEDSNUMS"))
            {
                uint32_t numData = nextionNumConvert(11, withoutCommand.length() - 4, withoutCommand);
                String dataToSendString = "NUMLEDS" + String(numData);

                sendCommandToDevice(dataToSendString, ledStripDevice);
            }
            else if (withoutCommand.startsWith("COLOUR"))
            {
                uint32_t numData;
                String dataToSendString;

                if (withoutCommand.startsWith("COLOURRED"))
                {
                    numData = nextionNumConvert(13, withoutCommand.length() - 4, withoutCommand);
                    dataToSendString = "COLOURRED" + String(numData);
                }
                else if (withoutCommand.startsWith("COLOURGREEN"))
                {
                    numData = nextionNumConvert(15, withoutCommand.length() - 4, withoutCommand);
                    dataToSendString = "COLOURGREEN" + String(numData);
                }
                else if (withoutCommand.startsWith("COLOURBLUE"))
                {
                    numData = nextionNumConvert(14, withoutCommand.length() - 4, withoutCommand);
                    dataToSendString = "COLOURBLUE" + String(numData);
                }

                sendCommandToDevice(dataToSendString, ledStripDevice);
            }
        }
        else if (commandString.startsWith("STATUSLED"))
        {
            String withoutCommand = commandString.substring(9);

            if (withoutCommand.startsWith("LINK"))
            {
                statusLedDevice = withoutCommand.substring(4);
            }
            else if (withoutCommand.startsWith("ON"))
            {
                vTaskResume(sendTempDataHandle);
                sendCommandToDevice("ON", statusLedDevice);
            }
            else if (withoutCommand.startsWith("OFF"))
            {
                vTaskSuspend(sendTempDataHandle);
                sendCommandToDevice("OFF", statusLedDevice);
            }
            else if (withoutCommand.startsWith("NUMLEDSNUMS"))
            {
                uint32_t numData = nextionNumConvert(11, withoutCommand.length() - 4, withoutCommand);
                String dataToSendString = "NUMLEDS" + String(numData);

                sendCommandToDevice(dataToSendString, statusLedDevice);
            }
            else if (withoutCommand.startsWith("BRIGHTNESSNUMS"))
            {
                uint32_t numData = nextionNumConvert(14, withoutCommand.length() - 4, withoutCommand);
                String dataToSendString = "BRIGHTNESS" + String(numData);
                Serial.println(dataToSendString);
                sendCommandToDevice(dataToSendString, statusLedDevice);
            }
        }
        else if (commandString.startsWith("PLAYMUSIC"))
        {
            Serial.print("Play Music: ");
            int songIndex = nextionNumConvert(13, commandString.length() - 4, commandString);
            Serial.println((String)songIndex);
            Serial.print("PLAYMUSIC");
            Serial.println(songIndex);
        }
        else if (commandString == "REFRESHMUSIC")
        {
            Serial.println("Refreshing music list");
            Serial.println("REFRESHMUSIC");
        }
        else if (commandString == "PAUSEMUSIC")
        {
            Serial.println("Pause Music");
            Serial.println("PAUSEMUSIC");
        }
        else if (commandString == "INCREASEMUSIC")
        {
            Serial.println("Increase volume");
            Serial.println("INCREASEMUSIC");
        }
        else if (commandString == "DECREASEMUSIC")
        {
            Serial.println("Decrease volume");
            Serial.println("DECREASEMUSIC");
        }
        else if (commandString.startsWith("LOCATION"))
        {
            // Serial.println("Location data");
            Serial.println(commandString);
        }
        else if (commandString == "REFRESHWEATHER")
        {
            // Serial.println("Refresh weather");
            Serial.println("REFRESHWEATHER"); // Redirect command to c# app to fetch weather data
        }
        else if (commandString == "REFRESHTIDE")
        {
            // Serial.println("Refreshing Tides");
            Serial.println("REFRESHTIDE");
        }
        else if (commandString == "REFRESHCAM1")
        {
            Serial.println("Refresh cam 1");
            Serial.println(cam1ServerUrl);
            uploadCamCaptureToNextion(cam1ServerUrl, "cam1");
        }
        else if (commandString == "REFRESHCAM2")
        {
            Serial.println("Refresh cam 2");
            Serial.println(cam2ServerUrl);
            uploadCamCaptureToNextion(cam2ServerUrl, "cam2");
        }
        else if (commandString.startsWith("MOTIONSENSOR"))
        {
            String withoutSensor = commandString.substring(12);
            if (withoutSensor.startsWith("DEVICE"))
            {
                motionSensorDevice = withoutSensor.substring(6);
            }

            else if (withoutSensor.startsWith("SW"))
            {
                motionSensorSwitch = withoutSensor;
                sendCommandToDevice(motionSensorSwitch, motionSensorDevice);
            }
            else if (withoutSensor.startsWith("CHANNEL"))
            {
                String channel = withoutSensor.substring(7);
                String dataToSendString = "CHANNEL" + String(channel);

                sendCommandToDevice(dataToSendString, motionSensorDevice);
            }
            else if ((withoutSensor == "ENABLE") || (withoutSensor == "DISABLE"))
            {
                sendCommandToDevice(withoutSensor, motionSensorDevice);
            }
            else if (withoutSensor.startsWith("TIMEOUTNUMS"))
            {
                uint32_t numData = nextionNumConvert(11, withoutSensor.length() - 4, withoutSensor);
                String dataToSendString = "TIMEOUT" + String(numData);

                sendCommandToDevice(dataToSendString, motionSensorDevice);
            }
        }
        else if (commandString.startsWith("TEMPSENSOR"))
        {
            String withoutSensor = commandString.substring(10);

            if (withoutSensor.startsWith("DEVICE"))
            {
                tempSensorDevice = withoutSensor.substring(6);
            }

            else if (withoutSensor.startsWith("SW"))
            {
                tempSensorSwitch = withoutSensor;
                sendCommandToDevice(tempSensorSwitch, tempSensorDevice);
            }
            else if (withoutSensor.startsWith("CHANNEL"))
            {
                String channel = withoutSensor.substring(7);
                String dataToSendString = "CHANNEL" + String(channel);

                sendCommandToDevice(dataToSendString, tempSensorDevice);
            }
            else if ((withoutSensor == "ENABLE") || (withoutSensor == "DISABLE"))
            {
                sendCommandToDevice(withoutSensor, tempSensorDevice);
            }
            else if (withoutSensor.startsWith("TRIGGERNUMS"))
            {
                uint32_t numData = nextionNumConvert(11, withoutSensor.length() - 4, withoutSensor);
                String dataToSendString = "TRIGGER" + String(numData);

                sendCommandToDevice(dataToSendString, tempSensorDevice);
            }
            else if (withoutSensor.startsWith("HYSTERISISNUMS"))
            {
                uint32_t numData = nextionNumConvert(14, withoutSensor.length() - 4, withoutSensor);
                String dataToSendString = "HYSTERISIS" + String(numData);

                sendCommandToDevice(dataToSendString, tempSensorDevice);
            }
            else if (withoutSensor.startsWith("CURRENTTEMP"))
            {
                String currentTemp = withoutSensor.substring(11);
                Serial.println("Current Temp: " + currentTemp);
                queueNextionCommand("main.tempSensorText.txt=\"" + currentTemp + "\"");
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Send hardware data to the nextion display
void sendHardwareData(void *params)
{

    static JsonDocument cacheDoc;
    static JsonDocument doc;
    DeserializationError error;

    float ramUsed;
    float ramAvail;
    float gpuTemp;
    int cpuPackageTemp;
    float cpuPackagePower;
    float gpuPower;
    int ramPercentage;
    float totalPower;

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("Stack sendHWdata: " + String(stackLeft));

        if (xSemaphoreTake(hardwareDataSemaphore, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, hardwareData);
        }

        if (!error)
        {
            cacheDoc.set(doc);
        }
        else
        {
            doc.set(cacheDoc);
        }

        // serializeJsonPretty(doc, Serial); // Display whole data for debugging

        ramUsed = doc["RamUsed"];
        ramAvail = doc["RamAvail"];
        gpuTemp = doc["GpuTemp"];
        cpuPackageTemp = doc["CpuPackageTemp"];
        cpuPackagePower = doc["CpuPackagePower"];
        gpuPower = doc["GpuPower"];

        totalPower = ((cpuPackagePower + gpuPower));

        // Dynamic scaling of y-axis for power, the waveform is 100px high
        if (totalPower <= 100)
        {
            nextionWaveformYAxisScale(0, 100, "main.power");
        }
        else if (totalPower > 100 && totalPower <= 200)
        {
            nextionWaveformYAxisScale(100, 200, "main.power");
            totalPower -= 100;
        }
        else if (totalPower > 200 && totalPower <= 300)
        {
            nextionWaveformYAxisScale(200, 300, "main.power");
            totalPower -= 200;
        }
        else if (totalPower > 300 && totalPower <= 400)
        {
            nextionWaveformYAxisScale(300, 400, "main.power");
            totalPower -= 300;
        }
        else if (totalPower > 400 && totalPower <= 500)
        {
            nextionWaveformYAxisScale(400, 500, "main.power");
            totalPower -= 400;
        }
        else if (totalPower > 500 && totalPower <= 600)
        {
            nextionWaveformYAxisScale(500, 600, "main.power");
            totalPower -= 500;
        }
        else
        {
            nextionWaveformYAxisScale(0, 1000, "main.power");
        }

        ramPercentage = (int)((ramUsed / (ramUsed + ramAvail)) * 100);

        queueNextionCommand("main.cpuTemp.val=" + String((int)cpuPackageTemp));
        queueNextionCommand("main.gpuTemp.val=" + String((int)gpuTemp));
        queueNextionCommand("main.ramUsage.val=" + String((int)ramPercentage));
        queueNextionCommand("main.powerUsage.val=" + String((int)totalPower));

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

struct WeatherForecast
{
    int id;
    int delta;
    float temp;
    int prec_probability;
    float prec;
    int weather_code;
    float wind_speed;
    int wind_dir;
    bool isDay;
};

// Send weather forecasts to the nextion display
void sendWeatherData(void *params)
{

    JsonDocument doc;
    DeserializationError error;
    String location;

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("send Weather data: " + String(stackLeft));

        if (xSemaphoreTake(weatherBinSemaphore, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, weatherData);
        }
        // serializeJsonPretty(doc, Serial);

        WeatherForecast forecast = {
            doc["id"].as<int>(),
            doc["delta"].as<int>(),
            doc["temp"].as<float>(),
            doc["prec_probability"].as<int>(),
            doc["prec"].as<float>(),
            doc["weather_code"].as<int>(),
            doc["wind_speed"].as<float>(),
            doc["wind_dir"].as<int>(),
            doc["isDay"].as<bool>(),
        };

        int weatherImageID = weatherCodeToNextionPicID(forecast.weather_code, forecast.isDay);
        int weatherBarbID = windToNextionWindBarbID(forecast.wind_speed, forecast.wind_dir);
        int weatherRainID = rainToNextionRainID(forecast.prec, forecast.prec_probability);
        String deltaLabel;
        if (forecast.delta == 0)
        {
            deltaLabel = "Now";
        }
        else
        {
            deltaLabel = String(forecast.delta) + "h";
        }

        queueNextionCommand("main.wPic" + String(forecast.id) + ".pic=" + String(weatherImageID));
        queueNextionCommand("main.wTitle" + String(forecast.id) + ".txt=\"" + deltaLabel + "\"");
        queueNextionCommand("main.wTemp" + String(forecast.id) + ".txt=\"" + String((int)round(forecast.temp)) + "\"");
        queueNextionCommand("main.wRain" + String(forecast.id) + ".pic=" + String(weatherRainID));
        queueNextionCommand("main.wBarb" + String(forecast.id) + ".pic=" + String(weatherBarbID));

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// Send tide data to the nextion display
void sendTideData(void *params)
{

    JsonDocument doc;
    DeserializationError error;

    String high1Time;
    String lowTime;
    String high2Time;
    String currentTime;
    float high1Height;
    float lowHeight;
    float high2Height;

    float currentHeight;
    int currentX;
    int currentY;

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("Stack sendTide: " + String(stackLeft));

        if (xSemaphoreTake(tideBinSemaphore, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, tideData);
        }
        // serializeJsonPretty(doc, Serial);

        high1Time = doc["High1"]["time"].as<const char *>();
        lowTime = doc["Low"]["time"].as<const char *>();
        high2Time = doc["High2"]["time"].as<const char *>();
        high1Height = doc["High1"]["height"].as<float>();
        lowHeight = doc["Low"]["height"].as<float>();
        high2Height = doc["High2"]["height"].as<float>();
        currentTime = doc["currentTime"].as<const char *>();

        // Determine current tide height
        // Find the time value for cosine function
        int high1TimeInt = (high1Time.substring(0, 2) + high1Time.substring(3, 5)).toInt();
        int lowTimeInt = (lowTime.substring(0, 2) + lowTime.substring(3, 5)).toInt();
        int high2TimeInt = (high2Time.substring(0, 2) + high2Time.substring(3, 5)).toInt();
        int currentTimeInt = (currentTime.substring(0, 2) + currentTime.substring(3, 5)).toInt();

        if (high2TimeInt < high1TimeInt)
        {
            high2TimeInt += 2400;
        }

        if (currentTimeInt < high1TimeInt)
        {
            currentTimeInt += 2400;
        }

        float timeRatio = (float(currentTimeInt - high1TimeInt) / float(high2TimeInt - high1TimeInt));
        float timeRadians = timeRatio * 2 * M_PI;
        Serial.println("Time Radians: " + String(timeRadians));

        // Calculation of height
        float amplitude = (high1Height - lowHeight) / 2.0;
        currentHeight = amplitude * cos(timeRadians) + amplitude + lowHeight;

        Serial.println("Current Height: " + String(currentHeight));

        // Calculate x and y coordinates
        currentX = timeRatio * (380 - 200) + 200;
        float heightRatio = (currentHeight - high1Height) / (lowHeight - high1Height);
        currentY = heightRatio * (510 - 452) + 452;

        Serial.println("Current X: " + String(currentX));
        Serial.println("Current Y: " + String(currentY));

        queueNextionCommand("main.currentTide.x=" + String(currentX));
        queueNextionCommand("main.currentTide.y=" + String(currentY));

        String high1HeightString = String(round(high1Height * 100.0) / 100.0);
        String lowHeightString = String(round(lowHeight * 100.0) / 100.0);
        String high2HeightString = String(round(high2Height * 100.0) / 100.0);

        queueNextionCommand("main.high1Time.txt=\"" + high1Time + "\"");
        queueNextionCommand("main.lowTime.txt=\"" + lowTime + "\"");
        queueNextionCommand("main.high2Time.txt=\"" + high2Time + "\"");
        queueNextionCommand("main.high1Height.txt=\"" + high1HeightString + "m\"");
        queueNextionCommand("main.lowHeight.txt=\"" + lowHeightString + "m\"");
        queueNextionCommand("main.high2Height.txt=\"" + high2HeightString + "m\"");

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// Send temperature data to the status led if the task is resumed
void sendTempData(void *params)
{

    JsonDocument cacheDoc;
    JsonDocument doc;
    DeserializationError error;

    float gpuTemp;

    while (true)
    {
        // UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        // Serial.println("Stack sendTempdata: " + String(stackLeft));

        if (xSemaphoreTake(hardwareDataSemaphore, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, hardwareData);
        }

        if (!error)
        {
            cacheDoc.set(doc);
        }
        else
        {
            doc.set(cacheDoc);
        }

        // serializeJsonPretty(doc, Serial); // Display whole data for debugging

        gpuTemp = doc["GpuTemp"];
        String tempString = "GPUTEMP" + (String)(int)gpuTemp;

        sendCommandToDevice(tempString, statusLedDevice);

        vTaskDelay(2500 / portTICK_PERIOD_MS);
    }
}