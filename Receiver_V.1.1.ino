#include <Arduino.h>
#include "driver/twai.h"

static const gpio_num_t TX_GPIO = GPIO_NUM_17;  // ESP32 -> SN65HVD230 TXD
static const gpio_num_t RX_GPIO = GPIO_NUM_16;  // SN65HVD230 RXD -> ESP32

static twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static twai_general_config_t g_config =
    TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_NORMAL);

void setup() {
  Serial.begin(115200);
  delay(200);

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK &&
      twai_start() == ESP_OK) {
    Serial.println("TWAI (CAN) started (Receiver).");
  } else {
    Serial.println("Failed to start TWAI.");
    while (true) delay(1000);
  }
}

void loop() {
  twai_message_t rx_msg;
  esp_err_t res = twai_receive(&rx_msg, pdMS_TO_TICKS(1000));

  if (res == ESP_OK) {
    Serial.printf("RX ID 0x%03X  %s  DLC=%d  Data:",
                  rx_msg.identifier,
                  rx_msg.extd ? "EXT" : "STD",
                  rx_msg.data_length_code);

    for (int i = 0; i < rx_msg.data_length_code; i++) {
      Serial.printf(" %02X", rx_msg.data[i]);
    }
    Serial.println();

    if (!rx_msg.rtr && !rx_msg.extd && rx_msg.identifier == 0x123 && rx_msg.data_length_code >= 4) {
      uint32_t counter = (uint32_t(rx_msg.data[0]) << 24) |
                         (uint32_t(rx_msg.data[1]) << 16) |
                         (uint32_t(rx_msg.data[2]) << 8)  |
                         (uint32_t(rx_msg.data[3]));
      Serial.printf("  -> Counter parsed: %lu\n", (unsigned long)counter);
    }
  } else if (res != ESP_ERR_TIMEOUT) {
    Serial.printf("RX error: %d\n", (int)res);
  }
}
