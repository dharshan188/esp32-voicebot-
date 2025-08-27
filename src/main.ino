#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "driver/i2s.h"
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"  // Include API keys from external file

const uint32_t SAMPLE_RATE = 11025;
const uint16_t BITS_PER_SAMPLE = 16;
const uint16_t CHANNELS = 1;
#define RECORD_TIME_SEC 5
#define WAV_FILE_PATH "/record.wav"

// I2S Microphone pins
#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14

// SD card pin
#define SD_CS 5

// Gemini
const int GEMINI_MAX_OUTPUT_TOKENS = 100;

// --- Function Prototypes ---
void recordAudio();
void writeWavHeader(File file, int dataSize);
String sendToDeepgram();
String getGeminiResponse(String userQuestion);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(1); }

    Serial.println(F("--- Starting ESP32 Voice Assistant Setup ---"));

    // Connect WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print(F("Connecting to WiFi "));
    Serial.println(WIFI_SSID);
    unsigned long wifiConnectStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(F("."));
        if (millis() - wifiConnectStart > 20000) {
            Serial.println(F("\n❌ WiFi Connection Failed! Restarting..."));
            ESP.restart();
        }
    }
    Serial.println(F("\n✅ WiFi Connected!"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());

    // Sync time
    Serial.print(F("⏳ Syncing time with NTP..."));
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    unsigned long ntpStart = millis();
    while (time(nullptr) < 100000 && millis() - ntpStart < 15000) { delay(100); Serial.print(F(".")); }
    if (time(nullptr) < 100000) { Serial.println(F("\n❌ Time Sync Failed! Restarting...")); ESP.restart(); }
    Serial.println(F("\n✅ Time Synced!"));

    // SD card
    if (!SD.begin(SD_CS)) { Serial.println(F("❌ SD Card Mount Failed. Restarting...")); ESP.restart(); }
    Serial.println(F("💾 SD Card OK"));

    Serial.println(F("\n--- Setup Complete. Ready for Voice Input ---"));
}

void loop() {
    Serial.println(F("\nPress Enter in Serial Monitor to start recording..."));
    while (!Serial.available()) { delay(100); }
    while (Serial.available()) { Serial.read(); }

    // I2S Config
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    Serial.println(F("Configuring I2S driver..."));
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK ||
        i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
        Serial.println(F("❌ I2S Setup Failed"));
        return;
    }
    i2s_zero_dma_buffer(I2S_NUM_0);
    Serial.println(F("✅ I2S Ready"));

    // Record -> Transcribe -> Gemini
    recordAudio();
    i2s_driver_uninstall(I2S_NUM_0);

    String transcript = sendToDeepgram();
    if (transcript.length() > 0 && transcript != "No transcript found." && !transcript.startsWith("Deepgram error:")) {
        Serial.print(F("\nSending transcript to Gemini: "));
        Serial.println(transcript);
        String geminiAnswer = getGeminiResponse(transcript);
        Serial.println(F("\n--- Gemini's Answer ---"));
        Serial.println(geminiAnswer);
        Serial.println(F("---------------------\n"));
    } else {
        Serial.println(F("No valid transcript received from Deepgram."));
    }
    delay(2000);
}

// --- Audio Recording ---
void recordAudio() {
    File file = SD.open(WAV_FILE_PATH, FILE_WRITE);
    if (!file) { Serial.println(F("❌ Failed to open WAV file")); return; }
    writeWavHeader(file, 0);

    uint32_t bytesToRecord = SAMPLE_RATE * (BITS_PER_SAMPLE / 8) * CHANNELS * RECORD_TIME_SEC;
    uint32_t bytesWritten = 0;
    uint8_t buffer[512];
    unsigned long lastPrintTime = millis();
    Serial.printf(F("🎙️ Recording %d seconds of audio...\n"), RECORD_TIME_SEC);

    while (bytesWritten < bytesToRecord) {
        size_t bytesRead;
        esp_err_t result = i2s_read(I2S_NUM_0, &buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
        if (result == ESP_OK && bytesRead > 0) {
            bytesWritten += file.write(buffer, bytesRead);
            if (millis() - lastPrintTime > 1000) { Serial.printf(F("Progress: %d%%\r"), (bytesWritten * 100) / bytesToRecord); lastPrintTime = millis(); }
        } else if (result != ESP_OK) { Serial.printf(F("❌ I2S Read Error: %d\n"), result); break; }
    }

    file.seek(0);
    writeWavHeader(file, bytesWritten);
    file.close();
    Serial.printf(F("\n✅ Recording complete. Saved %lu bytes.\n"), bytesWritten);
}

void writeWavHeader(File file, int dataSize) {
    uint32_t fileSize = dataSize + 36;
    uint16_t audioFormat = 1;
    uint16_t numChannels = CHANNELS;
    uint32_t byteRate = SAMPLE_RATE * numChannels * (BITS_PER_SAMPLE / 8);
    uint16_t blockAlign = numChannels * (BITS_PER_SAMPLE / 8);

    file.write((const uint8_t*)"RIFF", 4);
    file.write((uint8_t*)&fileSize, 4);
    file.write((const uint8_t*)"WAVE", 4);
    file.write((const uint8_t*)"fmt ", 4);
    uint32_t subchunk1Size = 16;
    file.write((uint8_t*)&subchunk1Size, 4);
    file.write((uint8_t*)&audioFormat, 2);
    file.write((uint8_t*)&numChannels, 2);
    file.write((uint8_t*)&SAMPLE_RATE, 4);
    file.write((uint8_t*)&byteRate, 4);
    file.write((uint8_t*)&blockAlign, 2);
    file.write((uint8_t*)&BITS_PER_SAMPLE, 2);
    file.write((const uint8_t*)"data", 4);
    file.write((uint8_t*)&dataSize, 4);
}

// --- Deepgram ---
String sendToDeepgram() {
    File file = SD.open(WAV_FILE_PATH, FILE_READ);
    if (!file || file.size() == 0) { Serial.println(F("❌ WAV file missing")); if(file) file.close(); SD.remove(WAV_FILE_PATH); return ""; }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    String url = "https://api.deepgram.com/v1/listen?model=nova-2&language=en-US&punctuate=true";
    https.begin(client, url);
    https.addHeader("Authorization", String("Token ") + DEEPGRAM_API_KEY);
    https.addHeader("Content-Type", "audio/wav");
    int httpCode = https.sendRequest("POST", &file, file.size());
    file.close();

    String transcript = "";
    if (httpCode == HTTP_CODE_OK) {
        String response = https.getString();
        StaticJsonDocument<4096> doc;
        if(!deserializeJson(doc, response)) {
            if (doc.containsKey("results") && doc["results"]["channels"][0]["alternatives"][0].containsKey("transcript")) {
                transcript = doc["results"]["channels"][0]["alternatives"][0]["transcript"].as<String>();
            }
        }
    } else {
        Serial.printf(F("❌ Deepgram HTTP Error %d\n"), httpCode);
        transcript = "Deepgram error: " + https.errorToString(httpCode);
    }
    https.end();
    SD.remove(WAV_FILE_PATH);
    return transcript;
}

// --- Gemini ---
String getGeminiResponse(String userQuestion) {
    if (userQuestion.length() < 2) return "Prompt too short.";

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(GEMINI_API_KEY);
    if(!https.begin(client, url)) return "Error connecting to Gemini";

    https.addHeader("Content-Type", "application/json");
    DynamicJsonDocument reqDoc(512 + userQuestion.length());
    reqDoc["contents"][0]["parts"][0]["text"] = userQuestion;
    reqDoc["generationConfig"]["maxOutputTokens"] = GEMINI_MAX_OUTPUT_TOKENS;
    String payload; serializeJson(reqDoc, payload);

    int httpCode = https.POST(payload);
    String geminiAnswer = "Error: No response";
    if(httpCode == HTTP_CODE_OK) {
        String response = https.getString();
        DynamicJsonDocument resDoc(2048);
        if(!deserializeJson(resDoc, response)) {
            if (resDoc.containsKey("candidates") && resDoc["candidates"][0]["content"]["parts"][0].containsKey("text"))
                geminiAnswer = resDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
        }
    } else {
        geminiAnswer = "Gemini HTTP error: " + https.errorToString(httpCode);
    }
    https.end();
    return geminiAnswer;
}
