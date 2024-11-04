#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "lambda";
const char* password = "MilanesA336";

const char* serverUrl = "http://0bcd-189-165-44-123.ngrok-free.app/api/upload_audio";

#define SAMPLE_RATE 4000  // Change to 4000 Hz
#define SAMPLE_BUFFER_SIZE (SAMPLE_RATE)
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_18
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_22
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

// I2S and buffer configuration
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,  // Change to 16-bit recording
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

// Buffer for raw 16-bit audio samples
int16_t raw_samples[SAMPLE_BUFFER_SIZE];

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Start the I2S interface
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
}

void loop() {
  size_t bytes_read = 0;

  // Read 16-bit audio from the I2S device
  i2s_read(I2S_NUM_0, raw_samples, sizeof(int16_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int16_t);

  // Send the 16-bit audio data to the server
  sendAudioToServer((uint8_t*)raw_samples, samples_read);
  
  delay(1000);  // Add delay for pacing
}

void sendAudioToServer(uint8_t* audio_data, int samples_read) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);  // Specify the URL

    // Set the headers
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("Sample-Rate", String(SAMPLE_RATE));
    http.addHeader("Channels", "1");  // Mono
    http.addHeader("Bits-Per-Sample", "16");  // Now sending 16-bit audio

    // Send the POST request with the audio data
    int httpResponseCode = http.POST(audio_data, samples_read * sizeof(int16_t));

    // Check the response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();  // Free resources
  } else {
    Serial.println("WiFi not connected");
  }
}
