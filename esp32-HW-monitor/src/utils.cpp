#include "utils.h"

extern HardwareSerial nextion;
extern QueueHandle_t sendNextionQueue;

void queueNextionCommand(String command)
{
    if (xQueueSend(sendNextionQueue, command.c_str(), 0) != pdTRUE)
    {
        Serial.println("Queue Full, Send Nextion Command Dropped!");
    }
}

void sendNextionCommand(String command)
{
    nextion.print(command);
    nextion.write(0xFF);
    nextion.write(0xFF);
    nextion.write(0xFF);
}

uint32_t nextionNumConvert(int start, int end, String command)
{
    int len = end - start;
    uint8_t numberData[len];
    uint32_t commandNumData = 0;

    // Nextion sends number data in little-endian format, so reverse it
    for (int i = 0; i < len; i++)
    {
        numberData[len - 1 - i] = static_cast<uint8_t>(command.charAt(start + i));
    }

    // Merge bytes
    for (int i = 0; i < len; i++)
    {
        commandNumData = (commandNumData << 8) | numberData[i]; // Shift and add the next byte
    }

    return commandNumData;
}

void nextionWaveformYAxisScale(int min, int max, String waveformID)
{
    int currVal = min;
    int step = (max - min) / 5;
    for (int i = 0; i <= 5; i++)
    {
        queueNextionCommand(waveformID + i + ".txt=\"" + currVal + "\"");
        currVal += step;
    }
}

float calculateAverage(const float arr[], int size)
{
    float sum = 0.0;
    int count = 0;

    for (int i = 0; i < size; i++)
    {
        if (arr[i] != 0.0)
        {
            sum += arr[i];
            count++;
        }
    }

    return count > 0 ? sum / count : 0.0;
}

void sendCommandToDevice(String command, String device)
{
    std::array<uint8_t, 6> MAC = deviceMACMap[device];

    char dataToSend[32];
    strncpy(dataToSend, command.c_str(), sizeof(dataToSend) - 1);
    dataToSend[sizeof(dataToSend) - 1] = '\0';

    esp_err_t result = esp_now_send(MAC.data(), (uint8_t *)&dataToSend, sizeof(dataToSend));

    if (result == ESP_OK)
    {
        Serial.println("Sending " + String(dataToSend) + " confirmed to " + device + " with MAC: ");
    }
    else
    {
        Serial.println("Sending " + String(dataToSend) + " error to " + device + " with MAC: ");
    }
    displayMacAddress(MAC);
}

// WEATHER STUFF - a lot of hard coding ids, but it's unavoidable due to how images are stored on the Nextion display
int weatherCodeToNextionPicID(int code, bool isDay)
{
    if (code == 0) // Clear
    {
        return isDay ? 0 : 1;
    }
    else if (code == 1) // Partly cloudy
    {
        return isDay ? 2 : 3;
    }
    else if (code == 2) // Cloudy
    {
        return 4;
    }
    else if (code == 3) // Overcast
    {
        return 6;
    }
    else if (code == 45 || code == 48) // Fog
    {
        return 16;
    }
    else if (code >= 51 && code <= 57) // Drizzle
    {
        return isDay ? 2 : 3;
    }
    else if (code >= 61 && code <= 67) // Rain
    {
        return 6;
    }
    else if (code >= 80 && code <= 82) // Shower
    {
        return 6;
    }
    else if ((code >= 71 && code <= 77) || (code == 85 || code == 86)) // Snow
    {
        return 14;
    }
    else if (code >= 95 && code <= 99) // Thunderstorm
    {
        return 12;
    }
    else
    {
        return 19; // Unknown
    }
}

int windSpeedToNextionPicID(float windSpeed)
{
    int windKnotsRounded = (round(windSpeed * 0.539957 / 5) * 5);

    // Follows Beufort wind scale
    if (windKnotsRounded == 0) // No wind
    {
        return 19;
    }
    else if (windKnotsRounded == 5 || windKnotsRounded == 10) // low wind
    {
        return 21;
    }
    else if (windKnotsRounded == 15 || windKnotsRounded == 20) // moderate wind
    {
        return 22;
    }
    else if (windKnotsRounded > 20) // strong wind
    {
        return 20;
    }
    else
    {
        return 19;
    }
}

int rainToNextionRainID(float prec, int prec_probability)
{
    String imgName = "";

    if (prec < 0.5) // No rain
    {
        return 19;
    }
    else if (prec >= 0.5 && prec <= 1.0) // 1 raindrop
    {
        imgName += "1";
    }
    else if (prec > 1.0 && prec <= 2.0) // 2 raindrops
    {
        imgName += "2";
    }
    else if (prec > 2.0 && prec <= 4.0) // 3 raindrops
    {
        imgName += "3";
    }
    else if (prec > 4.0) // 4 raindrops
    {
        imgName += "4";
    }

    if (prec_probability < 30) // Light blue
    {
        imgName += "low";
    }
    else if (prec_probability >= 30 && prec_probability <= 60) // Blue
    {
        imgName += "mid";
    }
    else if (prec_probability > 60) // Dark blue
    {
        imgName += "high";
    }

    if (imgName == "1high")
    {
        return 91;
    }
    else if (imgName == "1low")
    {
        return 92;
    }
    else if (imgName == "1mid")
    {
        return 93;
    }
    else if (imgName == "2high")
    {
        return 94;
    }
    else if (imgName == "2low")
    {
        return 95;
    }
    else if (imgName == "2mid")
    {
        return 96;
    }
    else if (imgName == "3high")
    {
        return 97;
    }
    else if (imgName == "3low")
    {
        return 98;
    }
    else if (imgName == "3mid")
    {
        return 99;
    }
    else if (imgName == "4high")
    {
        return 100;
    }
    else if (imgName == "4low")
    {
        return 101;
    }
    else if (imgName == "4mid")
    {
        return 102;
    }
    return 19; // Unknown
}

int windToNextionWindBarbID(float windSpeed, int windDir)
{

    int windKnots = (round(windSpeed * 0.539957 / 5) * 5);

    if (windKnots == 0) // Calm
    {
        return 25;
    }

    String cardinalDir;

    if (windDir >= 0 && windDir <= 22 || windDir > 337 && windDir <= 360)
    {
        cardinalDir = "S"; // Assuming cardinal direction is opposite of wind, i.e. wind coming from the north means wind is blowing south
    }
    else if (windDir > 22 && windDir <= 67)
    {
        cardinalDir = "SW";
    }
    else if (windDir > 67 && windDir <= 112)
    {
        cardinalDir = "W";
    }
    else if (windDir > 112 && windDir <= 157)
    {
        cardinalDir = "NW";
    }
    else if (windDir > 157 && windDir <= 202)
    {
        cardinalDir = "N";
    }
    else if (windDir > 202 && windDir <= 247)
    {
        cardinalDir = "NE";
    }
    else if (windDir > 247 && windDir <= 292)
    {
        cardinalDir = "E";
    }
    else if (windDir > 292 && windDir <= 337)
    {
        cardinalDir = "SE";
    }

    if (cardinalDir == "N")
    {
        if (windKnots == 5)
        {
            return 34;
        }
        else if (windKnots == 10)
        {
            return 35;
        }
        else if (windKnots == 15)
        {
            return 36;
        }
        else if (windKnots == 20)
        {
            return 37;
        }
        else if (windKnots == 25)
        {
            return 38;
        }
        else if (windKnots == 30)
        {
            return 39;
        }
        else if (windKnots == 35)
        {
            return 40;
        }
        else if (windKnots == 40)
        {
            return 41;
        }
    }
    else if (cardinalDir == "NE")
    {
        if (windKnots == 5)
        {
            return 42;
        }
        else if (windKnots == 10)
        {
            return 43;
        }
        else if (windKnots == 15)
        {
            return 44;
        }
        else if (windKnots == 20)
        {
            return 45;
        }
        else if (windKnots == 25)
        {
            return 46;
        }
        else if (windKnots == 30)
        {
            return 47;
        }
        else if (windKnots == 35)
        {
            return 48;
        }
        else if (windKnots == 40)
        {
            return 49;
        }
    }
    else if (cardinalDir == "E")
    {
        if (windKnots == 5)
        {
            return 26;
        }
        else if (windKnots == 10)
        {
            return 27;
        }
        else if (windKnots == 15)
        {
            return 28;
        }
        else if (windKnots == 20)
        {
            return 29;
        }
        else if (windKnots == 25)
        {
            return 30;
        }
        else if (windKnots == 30)
        {
            return 31;
        }
        else if (windKnots == 35)
        {
            return 32;
        }
        else if (windKnots == 40)
        {
            return 33;
        }
    }
    else if (cardinalDir == "SE")
    {
        if (windKnots == 5)
        {
            return 66;
        }
        else if (windKnots == 10)
        {
            return 67;
        }
        else if (windKnots == 15)
        {
            return 68;
        }
        else if (windKnots == 20)
        {
            return 69;
        }
        else if (windKnots == 25)
        {
            return 70;
        }
        else if (windKnots == 30)
        {
            return 71;
        }
        else if (windKnots == 35)
        {
            return 72;
        }
        else if (windKnots == 40)
        {
            return 73;
        }
    }
    else if (cardinalDir == "S")
    {
        if (windKnots == 5)
        {
            return 58;
        }
        else if (windKnots == 10)
        {
            return 59;
        }
        else if (windKnots == 15)
        {
            return 60;
        }
        else if (windKnots == 20)
        {
            return 61;
        }
        else if (windKnots == 25)
        {
            return 62;
        }
        else if (windKnots == 30)
        {
            return 63;
        }
        else if (windKnots == 35)
        {
            return 64;
        }
        else if (windKnots == 40)
        {
            return 65;
        }
    }
    else if (cardinalDir == "SW")
    {
        if (windKnots == 5)
        {
            return 74;
        }
        else if (windKnots == 10)
        {
            return 75;
        }
        else if (windKnots == 15)
        {
            return 76;
        }
        else if (windKnots == 20)
        {
            return 77;
        }
        else if (windKnots == 25)
        {
            return 78;
        }
        else if (windKnots == 30)
        {
            return 79;
        }
        else if (windKnots == 35)
        {
            return 80;
        }
        else if (windKnots == 40)
        {
            return 81;
        }
    }
    else if (cardinalDir == "W")
    {
        if (windKnots == 5)
        {
            return 82;
        }
        else if (windKnots == 10)
        {
            return 83;
        }
        else if (windKnots == 15)
        {
            return 84;
        }
        else if (windKnots == 20)
        {
            return 85;
        }
        else if (windKnots == 25)
        {
            return 86;
        }
        else if (windKnots == 30)
        {
            return 87;
        }
        else if (windKnots == 35)
        {
            return 88;
        }
        else if (windKnots == 40)
        {
            return 89;
        }
    }
    else if (cardinalDir == "NW")
    {
        if (windKnots == 5)
        {
            return 50;
        }
        else if (windKnots == 10)
        {
            return 51;
        }
        else if (windKnots == 15)
        {
            return 52;
        }
        else if (windKnots == 20)
        {
            return 53;
        }
        else if (windKnots == 25)
        {
            return 54;
        }
        else if (windKnots == 30)
        {
            return 55;
        }
        else if (windKnots == 35)
        {
            return 56;
        }
        else if (windKnots == 40)
        {
            return 57;
        }
    }

    return 19; // Unknown
}