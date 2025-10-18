#include <Arduino.h>
#include "driver/twai.h"

// CAN transceiver pins
static const gpio_num_t TX_GPIO = GPIO_NUM_17;  // ESP32 -> SN65HVD230 TXD
static const gpio_num_t RX_GPIO = GPIO_NUM_16;  // SN65HVD230 RXD -> ESP32

// Analog temp sensor pin (choose an ADC-capable pin)
static const int TEMP_PIN = 34;  // <-- change to your wiring

// TWAI (CAN) config
static twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_NORMAL);

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(TEMP_PIN, INPUT);  // safe for ADC

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK &&
      twai_start() == ESP_OK) {
    Serial.println("TWAI (CAN) started (Sender).");
  } else {
    Serial.println("Failed to start TWAI.");
    while (true) delay(1000);
  }
}

void loop() {
  static uint32_t counter = 0;

  // Read sensor (ESP32 ADC is 12-bit: 0..4095). Pack down to one CAN byte.
  int tempRaw = analogRead(TEMP_PIN);       // 0..4095
  uint8_t temp8 = (uint8_t)(tempRaw >> 4);  // 12-bit -> 8-bit (0..255)

  twai_message_t msg = {};
  msg.identifier = 0x123;     // 11-bit ID
  msg.extd = 0;               // standard frame
  msg.rtr = 0;                // data frame
  msg.data_length_code = 8;

  msg.data[0] = (counter >> 24) & 0xFF;
  msg.data[1] = (counter >> 16) & 0xFF;
  msg.data[2] = (counter >>  8) & 0xFF;
  msg.data[3] = (counter      ) & 0xFF;
  msg.data[4] = temp8;        // your temp sample, scaled to 1 byte
  msg.data[5] = 0xAD;
  msg.data[6] = 0xBE;
  msg.data[7] = 0xEF;

  esp_err_t res = twai_transmit(&msg, pdMS_TO_TICKS(1000));
  if (res == ESP_OK) {
    Serial.printf("Sent ID 0x%03X, counter=%lu, tempRaw=%d, temp8=%u\n",
                  msg.identifier, (unsigned long)counter, tempRaw, temp8);
    counter++;
  } else {
    Serial.printf("TX failed: %d\n", (int)res);
  }

  delay(500);
}
