#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>

const char* ssid = "lambda";
const char* password = "MilanesA336";
const char* serverURL = "http://15f1-189-165-44-123.ngrok-free.app/api/upload_audio";

#define I2S_NUM I2S_NUM_0 // Periférico I2S
#define SAMPLE_RATE 32000 // Frecuencia de muestreo
#define DURATION 1        // Duración en segundos
#define BITS_PER_SAMPLE 16 // Bits por muestra
#define ADC_INPUT_PIN GPIO4 // Entrada analógica (conectada al OUT del MAX9814)
#define BUTTON_PIN 18      // Pin del botón
#define LED_PIN 23 // Pin LED
#define LED2_PIN 22 // Pin LED ROJO

#define BUFFER_SIZE (SAMPLE_RATE * DURATION * (BITS_PER_SAMPLE / 8)) // Tamaño del buffer

uint8_t audioBuffer[BUFFER_SIZE]; // Buffer para almacenar datos de audio

void setup() {
  Serial.begin(115200);

  // Configurar el botón como entrada
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Usa PULLUP para evitar problemas con señales flotantes
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");

  // Configurar I2S para entrada desde ADC
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 32000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

  // Inicializar I2S
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_7); // Asignar el canal ADC (GPIO34)
  i2s_adc_enable(I2S_NUM); // Habilitar lectura del ADC
}

void loop() {
  // Leer estado del botón
  if (digitalRead(BUTTON_PIN) == LOW) { // El botón está presionado (LOW debido al PULLUP)
    Serial.println("Botón presionado, grabando audio...");

    // Capturar audio durante 1 segundo
    size_t bytesRead;
    i2s_read(I2S_NUM, audioBuffer, BUFFER_SIZE, &bytesRead, portMAX_DELAY);
    Serial.printf("Audio capturado: %d bytes\n", bytesRead);

    // Enviar audio a la API
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);
      http.addHeader("Content-Type", "application/octet-stream");
      http.addHeader("Sample-Rate", String(SAMPLE_RATE));
      http.addHeader("Channels", "1");
      http.addHeader("Bits-Per-Sample", String(BITS_PER_SAMPLE));

      int httpResponseCode = http.POST(audioBuffer, bytesRead);
      Serial.printf("Codigo: %d\n", httpResponseCode);
      if (httpResponseCode == 200) {
        Serial.printf("Autorizado :)\n");
        digitalWrite(LED_PIN, HIGH);
      } else {
        Serial.printf("No autorizado\n");
        digitalWrite(LED2_PIN, HIGH);
      }
      http.end();
    }

    delay(1000); // Esperar 1 segundo para evitar múltiples grabaciones al mantener el botón presionado
  }
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  delay(100); // Reducir el consumo de CPU en el bucle principal
}
