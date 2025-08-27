# ESP32 Voice Bot

A voice-controlled assistant using ESP32, I2S microphone (INMP441), Deepgram transcription API, and Gemini AI for responses.

## Features

- Records voice using an I2S microphone
- Transcribes speech to text using Deepgram API
- Sends transcription to Gemini AI for responses
- Outputs Gemini’s answer via Serial Monitor

## Hardware Required

- ESP32 Dev Board
- I2S Microphone (INMP441 or SPH0645)
- SD Card Module
- SPI connections for SD card
- Jumper wires

## Usage

1. Copy `secrets.example.h` to `secrets.h` and fill your WiFi and API keys.
2. Open `src/main.ino` in Arduino IDE.
3. Connect ESP32 and upload the code.
4. Open Serial Monitor, press Enter to record your question, and see AI response.

## Important

- **Never push your real API keys to GitHub.**
- Make sure your microphone and SD card wiring matches the code.
