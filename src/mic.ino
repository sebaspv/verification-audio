#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>

const char* ssid = "lambda";
const char* password = "MilanesA336";
const char* serverURL = "http://973a-189-165-44-123.ngrok-free.app/api/upload_audio";

// PDM configuration
const int sampleRate = 16000;
const float recordTime = 1;  // Record for 0.5 seconds at a time
const int bufferSize = sampleRate * recordTime;  // Buffer size for 0.5 seconds of audio
int16_t* audioData = NULL;

#define PDM_CLK_PIN 18  // Connect to SCK
#define PDM_DATA_PIN 21  // Connect to SD

void setup() {
    Serial.begin(115200);
    connectWiFi();

    // Configure I2S for PDM mode
    i2s_config_t i2sConfig = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // Mono input from PDM mic
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false
    };

    i2s_pin_config_t pinConfig = {
        .bck_io_num = I2S_PIN_NO_CHANGE,  // Not used in PDM mode
        .ws_io_num = I2S_PIN_NO_CHANGE,   // Not used in PDM mode
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PDM_DATA_PIN       // Data input pin
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        Serial.println("Failed to install I2S driver");
    } else {
        Serial.println("I2S driver installed successfully in PDM mode");
        i2s_set_pin(I2S_NUM_0, &pinConfig);
        i2s_set_clk(I2S_NUM_0, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    }
}

void loop() {
    Serial.println("Recording audio...");
    recordAndSendAudio();
    delay(30000);  // Wait 30 seconds before recording again
}

void connectWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
}

void recordAndSendAudio() {
    // Allocate memory in DRAM for 0.5 seconds of audio data
    audioData = (int16_t*)malloc(bufferSize * sizeof(int16_t));
    if (audioData == NULL) {
        Serial.println("Failed to allocate DRAM");
        return;
    }

    // Record audio for 0.5 seconds
    size_t bytesRead = 0;
    int offset = 0;
    while (offset < bufferSize) {
        i2s_read(I2S_NUM_0, (char*)(audioData + offset), sizeof(int16_t) * (bufferSize - offset), &bytesRead, portMAX_DELAY);
        offset += bytesRead / sizeof(int16_t);
    }

    // Send the recorded audio data
    sendAudio();

    // Free the memory after sending
    free(audioData);
}

void sendAudio() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Prepare the POST request
        http.begin(serverURL);
        http.addHeader("Content-Type", "application/octet-stream");
        http.addHeader("Sample-Rate", String(sampleRate));  // Include sample rate
        http.addHeader("Channels", "1");                     // Include number of channels (mono)
        http.addHeader("Bits-Per-Sample", "16");             // Include bits per sample

        // Send the raw audio data
        int responseCode = http.POST((uint8_t*)audioData, bufferSize * sizeof(int16_t));

        Serial.printf("Response code: %d\n", responseCode);

        if (responseCode > 0) {
            String response = http.getString();
            Serial.println("Server response: " + response);
        }
        http.end();
    } else {
        Serial.println("Error: Not connected to WiFi");
    }
}




