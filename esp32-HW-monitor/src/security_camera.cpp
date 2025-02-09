#include "security_camera.h"

extern HardwareSerial nextion;

const String cam1ServerUrl = "http://esp32cam1.local/capture";
const String cam2ServerUrl = "http://esp32cam2.local/capture";

// Follows sending file instructions from Nextion's instruction set documentation
void sendPacket(uint8_t *data, size_t length, uint16_t packetID)
{
    uint8_t header[12] = {
        0x3A, 0xA1, 0xBB, 0x44, 0x7F, 0xFF, 0xFE, // pkConst
        0x00,                                     // vType (0x00 = no CRC)
        (uint8_t)(packetID & 0xFF),               // pkID low byte
        (uint8_t)((packetID >> 8) & 0xFF),        // pkID high byte
        (uint8_t)(length & 0xFF),                 // dataSize low byte
        (uint8_t)((length >> 8) & 0xFF)           // dataSize high byte
    };

    nextion.write(header, sizeof(header)); // Send header
    nextion.write(data, length);           // Send packet data
}

void sendJpg(const char *filePath, String camId)
{
    Serial.println("Start");
    File file = SPIFFS.open(filePath);
    if (!file)
    {
        Serial.println("Failed to open file from SPIFFS");
        return;
    }

    size_t fileSize = file.size();
    nextion.printf("twfile \"sd0/%s.jpg\",%d\xFF\xFF\xFF", camId.c_str(), fileSize);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    uint8_t buffer[4096];
    size_t bytesSent = 0;
    uint16_t packetID = 0;

    while (bytesSent < fileSize)
    {
        size_t chunkSize = file.read(buffer, sizeof(buffer));
        if (chunkSize > 0)
        {
            sendPacket(buffer, chunkSize, packetID);
            delay(10); // Wait for acknowledgment from nextion
            bytesSent += chunkSize;
            packetID++;
        }
    }

    file.close();
    Serial.println("File sent successfully to Nextion's SD card");
}

void deleteCapturedImage(const char *filePath)
{
    if (SPIFFS.exists(filePath))
    {
        if (SPIFFS.remove(filePath))
        {
            Serial.println("File deleted successfully");
        }
        else
        {
            Serial.println("Failed to delete the file");
        }
    }
    else
    {
        Serial.println("File does not exist");
    }
}

void uploadCamCaptureToNextion(String camServer, String camId)
{
    HTTPClient http;

    http.begin(camServer);
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK)
    {
        Serial.println("Getting capture for " + camId + "...");
        WiFiClient *stream = http.getStreamPtr();

        // Open a file to save the image
        File file = SPIFFS.open("/" + camId + ".jpg", FILE_WRITE);
        if (!file)
        {
            Serial.println("Failed to open file for writing");
            http.end();
            return;
        }

        // Save image to SPIFFS in chunks
        uint8_t buffer[128];
        size_t totalSize = 0;
        size_t size;

        while ((size = stream->readBytes(buffer, sizeof(buffer))) > 0)
        {
            file.write(buffer, size);
            totalSize += size;
        }

        file.close();
        if (totalSize > 0)
        {
            Serial.println("Image saved to SPIFFS as /" + camId + ".jpg");
            Serial.printf("The size of the image is: %d bytes\n", totalSize);
        }
        else
        {
            Serial.println("No data received, deleting file");
            SPIFFS.remove("/" + camId + ".jpg");
        }
    }
    else
    {
        Serial.printf("Failed to capture image, HTTP response: %d\n", httpResponseCode);
    }

    http.end();

    if (SPIFFS.exists("/" + camId + ".jpg"))
    {
        sendJpg(("/" + camId + ".jpg").c_str(), camId);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        sendNextionCommand("main." + camId + ".path=\"sd0/" + camId + ".jpg\"");
        deleteCapturedImage(("/" + camId + ".jpg").c_str());
    }
}