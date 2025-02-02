#include "tasks.h"

extern SemaphoreHandle_t hardwareDataMutex;
extern SemaphoreHandle_t weatherBinSemaphore;
extern SemaphoreHandle_t tideBinSemaphore;
extern uint8_t receiverAddress[];
extern HardwareSerial nextion;
extern QueueHandle_t commandQueue;
extern std::vector<std::string> commandPrefixes;
extern TaskHandle_t sendTempDataHandle;
String hardwareData;
String weatherData;
String tideData;

const int BUFFER_SIZE = 500;

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
        if (Serial0.available() > 0)
        {
            receivedData = Serial0.readStringUntil(0x03);

            if (receivedData.startsWith("HARDWARE"))
            {
                splicedData = receivedData.substring(8);
                if (xSemaphoreTake(hardwareDataMutex, 0) == pdTRUE)
                {
                    hardwareData = splicedData;
                    xSemaphoreGive(hardwareDataMutex);
                }
            }
            else if (receivedData.startsWith("WEATHER"))
            {

                if (receivedData.startsWith("WEATHERLOCATION"))
                {
                    String location = receivedData.substring(15);
                    sendNextionCommand("main.locationText.txt=\"" + location + "\"");
                }
                else
                {
                    splicedData = receivedData.substring(7);
                    weatherData = splicedData;
                    xSemaphoreGive(weatherBinSemaphore);
                }
            }
            else if (receivedData.startsWith("TIDE"))
            {
                splicedData = receivedData.substring(4);
                tideData = splicedData;
                xSemaphoreGive(tideBinSemaphore);
            }
            else if (receivedData.startsWith("MUSICSTRING"))
            {
                splicedData = receivedData.substring(11);
                Serial.println(splicedData);
                splicedData.replace("BREAK", "\\r");
                sendNextionCommand("main.musicList.path=\"\""); // For Visual confirmation of refresh
                sendNextionCommand("main.musicList.path=\"" + splicedData + "\"");
            }
            else if (receivedData.startsWith("SCHEDULE"))
            {
                String receivedString = receivedData.substring(8);
                Serial.println("Testing: " + receivedString);
                for (int i = 0; i < numSwitches; i++)
                {
                    String currentSwitch = (String)switches[i];
                    if (receivedString.startsWith(currentSwitch))
                    {
                        String SW = currentSwitch;
                        String action = receivedString.substring(SW.length());

                        if (action == "ONA")
                        {
                            sendNextionCommand("main." + SW + ".val=1");
                            sendNextionCommand("main." + SW + ".bco2=1024");
                            sendNextionCommand("main." + SW + "Val.val=1");
                        }
                        else if (action == "ONAB")
                        {
                            sendNextionCommand("main." + SW + ".val=1");
                            sendNextionCommand("main." + SW + ".bco2=1527");
                            sendNextionCommand("main." + SW + "Val.val=2");
                        }
                        else if (action == "ONB")
                        {
                            sendNextionCommand("main." + SW + ".val=1");
                            sendNextionCommand("main." + SW + ".bco2=1048");
                            sendNextionCommand("main." + SW + "Val.val=3");
                        }
                        else if (action == "OFF")
                        {
                            sendNextionCommand("main." + SW + ".val=0");
                            sendNextionCommand("main." + SW + ".bco2=1024");
                            sendNextionCommand("main." + SW + "Val.val=0");
                        }
                        else if (action == "ENABLE")
                        {
                            sendNextionCommand("main." + SW + ".val=1");
                        }
                        else if (action == "DISABLE")
                        {
                            sendNextionCommand("main." + SW + ".val=0");
                        }
                        break;
                    }
                }
                if (xQueueSend(commandQueue, receivedString.c_str(), 0) != pdTRUE)
                {
                    Serial.println("Queue full");
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

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
        if (nextion.available())
        {

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
                if ((byteReceived < 0x21 || byteReceived > 0x7E) && byteReceived != 0x03)
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
                    Serial.println("Queue full");
                }

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
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void executeCommands(void *params)
{
    char command[BUFFER_SIZE];

    while (true)
    {
        if (xQueueReceive(commandQueue, &command, portMAX_DELAY) != pdTRUE)
        {
            Serial.println("Nothing in queue");
            return;
        }

        Serial.println(command);
        String commandString = (String)command;

        if (commandString == "RESETSETTINGS")
        {
            switchDeviceMap.clear();
        }
        else if (commandString == "LOCKPC")
        {
            Serial.println("Lock PC");
            Serial0.println("LOCKPC"); // Redirect command to c# app
        }
        else if (commandString.startsWith("SCANESPS"))
        {
            deviceMACMap.clear();
            Serial.println("Scanning Esps");
            getMacAddresses();
            vTaskDelay(3000 / portTICK_PERIOD_MS); // Wait for responses from espnow

            String multilineString = "";
            for (const auto &pair : deviceMACMap)
            {
                if (!multilineString.isEmpty())
                {
                    multilineString += "\\r";
                }
                multilineString += pair.first;
            }

            for (int i = 0; i <= 19; i++) // 19 total select esp dropdowns on nextion
            {
                sendNextionCommand("select" + String(i) + ".path=\"" + multilineString + "\"");
            }

            String selectedEspsString = commandString.substring(8);
            Serial.println(selectedEspsString);

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
                    sendNextionCommand(selected + ".txt=\"Select Esp\"");
                }

                startIndex = breakIndex + 5;
            }
        }
        else if (commandString.startsWith("SW"))
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

                        for (const auto &device : devices)
                        {
                            sendCommandToDevice(command, device);
                        }
                    }
                    break;
                }
            }
        }
        else if (commandString.startsWith("LINK"))
        {
            String withoutCommand = commandString.substring(4);

            for (int i = 0; i < numSwitches; i++)
            {
                String currentSwitch = (String)switches[i];
                if (withoutCommand.startsWith(currentSwitch))
                {
                    String device = withoutCommand.substring(currentSwitch.length());
                    switchDeviceMap[currentSwitch].push_back(device);
                    displaySwitchDeviceMap();
                    break;
                }
            }
        }
        else if (commandString.startsWith("SCHEDULE"))
        {
            Serial.println("Sending to desktop app");
            Serial0.println(commandString);
        }
        else if (commandString.startsWith("WEATHERDELTA"))
        {
            Serial.println("Sending to desktop app");
            Serial0.println(commandString);
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
        }
        else if (commandString.startsWith("PLAYMUSIC"))
        {
            Serial.print("Play Music: ");
            int songIndex = nextionNumConvert(13, commandString.length() - 4, commandString);
            Serial.println((String)songIndex);
            Serial0.print("PLAYMUSIC");
            Serial0.println(songIndex);
        }
        else if (commandString == "REFRESHMUSIC")
        {
            Serial.println("Refreshing music list");
            Serial0.println("REFRESHMUSIC");
        }
        else if (commandString == "PAUSEMUSIC")
        {
            Serial.println("Pause Music");
            Serial0.println("PAUSEMUSIC");
        }
        else if (commandString == "INCREASEMUSIC")
        {
            Serial.println("Increase volume");
            Serial0.println("INCREASEMUSIC");
        }
        else if (commandString == "DECREASEMUSIC")
        {
            Serial.println("Decrease volume");
            Serial0.println("DECREASEMUSIC");
        }
        else if (commandString.startsWith("LOCATION"))
        {
            Serial.println("Location data");
            Serial0.println(commandString);
        }
        else if (commandString == "REFRESHWEATHER")
        {
            Serial.println("Refresh weather");
            Serial0.println("REFRESHWEATHER"); // Redirect command to c# app to fetch weather data
        }
        else if (commandString == "REFRESHTIDE")
        {
            Serial.println("Refreshing Tides");
            Serial0.println("REFRESHTIDE");
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
                sendNextionCommand("main.tempSensorText.txt=\"" + currentTemp + "\"");
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

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
        if (xSemaphoreTake(hardwareDataMutex, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, hardwareData);
            xSemaphoreGive(hardwareDataMutex);
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

        // waveform is 100px high
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

        sendNextionCommand("main.cpuTemp.val=" + String((int)cpuPackageTemp));
        sendNextionCommand("main.gpuTemp.val=" + String((int)gpuTemp));
        sendNextionCommand("main.ramUsage.val=" + String((int)ramPercentage));
        sendNextionCommand("main.powerUsage.val=" + String((int)totalPower));

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

struct WeatherForecast
{
    int id;
    float temp;
    int prec_probability;
    float prec;
    int weather_code;
    float wind_speed;
    int wind_dir;
    bool isDay;
};

void sendWeatherData(void *params)
{
    JsonDocument doc;
    DeserializationError error;
    String location;

    while (true)
    {
        if (xSemaphoreTake(weatherBinSemaphore, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, weatherData);
        }
        serializeJsonPretty(doc, Serial);

        WeatherForecast forecast = {
            doc["id"].as<int>(),
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
        sendNextionCommand("main.wPic" + String(forecast.id) + ".pic=" + String(weatherImageID));
        sendNextionCommand("main.wTemp" + String(forecast.id) + ".txt=\"" + String((int)round(forecast.temp)) + "\"");
        sendNextionCommand("main.wRain" + String(forecast.id) + ".pic=" + String(weatherRainID));
        sendNextionCommand("main.wBarb" + String(forecast.id) + ".pic=" + String(weatherBarbID));

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

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
        if (xSemaphoreTake(tideBinSemaphore, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, tideData);
        }
        serializeJsonPretty(doc, Serial);

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

        sendNextionCommand("main.currentTide.x=" + String(currentX));
        sendNextionCommand("main.currentTide.y=" + String(currentY));

        String high1HeightString = String(round(high1Height * 100.0) / 100.0);
        String lowHeightString = String(round(lowHeight * 100.0) / 100.0);
        String high2HeightString = String(round(high2Height * 100.0) / 100.0);

        sendNextionCommand("main.high1Time.txt=\"" + high1Time + "\"");
        sendNextionCommand("main.lowTime.txt=\"" + lowTime + "\"");
        sendNextionCommand("main.high2Time.txt=\"" + high2Time + "\"");
        sendNextionCommand("main.high1Height.txt=\"" + high1HeightString + "m\"");
        sendNextionCommand("main.lowHeight.txt=\"" + lowHeightString + "m\"");
        sendNextionCommand("main.high2Height.txt=\"" + high2HeightString + "m\"");

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void sendTempData(void *params)
{
    JsonDocument cacheDoc;
    JsonDocument doc;
    DeserializationError error;

    float gpuTemp;

    while (true)
    {
        if (xSemaphoreTake(hardwareDataMutex, portMAX_DELAY) == pdTRUE)
        {
            error = deserializeJson(doc, hardwareData);
            xSemaphoreGive(hardwareDataMutex);
        }

        if (!error)
        {
            cacheDoc.set(doc);
        }
        else
        {
            doc.set(cacheDoc);
        }

        serializeJsonPretty(doc, Serial); // Display whole data for debugging

        gpuTemp = doc["GpuTemp"];
        String tempString = "GPUTEMP" + (String)(int)gpuTemp;

        sendCommandToDevice(tempString, statusLedDevice);

        vTaskDelay(2500 / portTICK_PERIOD_MS);
    }
}