# ESP32 Voice Bot

A voice-controlled assistant using an ESP32, I2S microphone, Deepgram for transcription, and Google's Gemini AI for generating responses. This project turns your ESP32 into a smart, voice-interactive device capable of answering questions and performing tasks based on your voice commands.

## Project Workflow

```
[Your Voice] -> (I2S Microphone) -> [ESP32] -> (WiFi) -> [Deepgram API] -> (Transcription) -> [ESP32] -> (WiFi) -> [Gemini AI] -> (Response) -> [ESP32] -> [Serial Monitor]
```

This workflow illustrates how your voice is captured, processed, and responded to by the system.

## Hardware Required

| Component | Description | Alternatives |
|---|---|---|
| ESP32 Dev Board | A powerful microcontroller with built-in Wi-Fi and Bluetooth. | ESP32-S2, ESP32-S3 |
| I2S Microphone | A digital microphone that uses the I2S protocol. | INMP441, SPH0645 |
| SD Card Module | For storing the recorded audio file. | Any SPI SD card module |
| Jumper Wires | For connecting the components. | |

## Software Required

| Software | Description |
|---|---|
| Arduino IDE | The integrated development environment for writing and uploading code to the ESP32. | PlatformIO |
| ESP32 Board Manager | The ESP32 core for the Arduino IDE. | |
| Libraries | The following libraries are required: `ArduinoJson`, `HTTPClient`, `WiFi`, `FS`, `SD`, `SPI`. | |

## Configuration

1.  **Copy `secrets.example.h` to `secrets.h`:** This file will store your sensitive information, such as API keys and Wi-Fi credentials.
2.  **Fill in your Wi-Fi credentials:** Open `secrets.h` and replace `YOUR_WIFI_SSID` and `YOUR_WIFI_PASSWORD` with your Wi-Fi network's SSID and password.
3.  **Get your API keys:**
    *   **Deepgram:** Sign up for a Deepgram account and create an API key.
    *   **Google Gemini:** Go to Google AI Studio, create a new API key.
4.  **Add your API keys to `secrets.h`:** Replace `YOUR_DEEPGRAM_API_KEY` and `YOUR_GEMINI_API_KEY` with your actual API keys.

**Important:** Never push your `secrets.h` file to a public repository. The `.gitignore` file should be configured to ignore this file.

## Setup and Usage

1.  **Install the Arduino IDE:** Download and install the latest version of the Arduino IDE.
2.  **Install the ESP32 Board Manager:** Follow the instructions here to add ESP32 support to your Arduino IDE.
3.  **Install the required libraries:** Open the Arduino IDE's Library Manager and install the following libraries:
    *   `ArduinoJson`
4.  **Connect the hardware:** Connect the I2S microphone and SD card module to the ESP32 as per the pinout table below.
5.  **Open the project in the Arduino IDE:** Open the `src/main.ino` file.
6.  **Upload the code to the ESP32:** Select your ESP32 board and port, then click the "Upload" button.
7.  **Open the Serial Monitor:** Set the baud rate to `115200`.
8.  **Start recording:** Press the "Enter" key in the Serial Monitor to start recording your voice.
9.  **View the response:** The transcribed text and the response from Gemini will be displayed in the Serial Monitor.

## Pinout

| ESP32 Pin | I2S Microphone (INMP441) | SD Card Module |
|---|---|---|
| GPIO15 | WS | |
| GPIO32 | SD | |
| GPIO14 | SCK | |
| GPIO5 | | CS |
| GPIO18 | | SCK |
| GPIO19 | | MISO |
| GPIO23 | | MOSI |
| 3.3V | VCC | VCC |
| GND | GND | GND |

## Troubleshooting

*   **"Failed to mount SD card" error:**
    *   Check your wiring.
    *   Make sure your SD card is formatted as FAT32.
    *   Try a different SD card.
*   **"WiFi Connection Failed" error:**
    *   Double-check your Wi-Fi credentials in `secrets.h`.
    *   Make sure your Wi-Fi network is 2.4GHz, as the ESP32 does not support 5GHz.
*   **No audio recorded:**
    *   Check your microphone wiring.
    *   Make sure you are using a compatible I2S microphone.
*   **Deepgram or Gemini API errors:**
    *   Check your API keys in `secrets.h`.
    *   Make sure you have a stable internet connection.
    *   Check the API documentation for any changes.

## Future Improvements

*   **Add a speaker:** Use a speaker to play the response from Gemini.
*   **Add a display:** Use a display to show the transcribed text and the response from Gemini.
*   **Add wake word detection:** Use a wake word engine to start recording without pressing a button.
*   **Add support for other languages:** Use a different language model to support other languages.

## Contributing

Contributions are welcome! If you have any ideas, suggestions, or bug reports, please open an issue or create a pull request.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
