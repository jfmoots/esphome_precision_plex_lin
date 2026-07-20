#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "analyzer.h"
#include "version.h"

namespace esphome {
namespace precision_plex_lin {

class PrecisionPlexLinComponent : public Component {
 public:
  void set_uart_parent(uart::UARTComponent *uart_parent) { uart_parent_ = uart_parent; }

  void setup() override {
    ::precision_plex::global_analyzer().release_wireless_tp_mute();
    ESP_LOGI("precision_plex_lin", "Precision Plex LIN %s (Build %s)",
             PRECISION_PLEX_LIN_VERSION, PRECISION_PLEX_LIN_BUILD);
  }

  void loop() override {
    if (uart_parent_ != nullptr) ::precision_plex::global_analyzer().process(uart_parent_);
  }

  void dump_config() override {
    ESP_LOGCONFIG("precision_plex_lin", "Precision Plex LIN Analyzer:");
    ESP_LOGCONFIG("precision_plex_lin", "  Version: %s", PRECISION_PLEX_LIN_VERSION);
    ESP_LOGCONFIG("precision_plex_lin", "  Legacy build: %s", PRECISION_PLEX_LIN_BUILD);
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  ::precision_plex::CoachState &state() { return ::precision_plex::coach_state(); }

  const char *firmware_version() const { return PRECISION_PLEX_LIN_FIRMWARE_VERSION; }

  void release_wireless_tp_mute() {
    ::precision_plex::global_analyzer().release_wireless_tp_mute();
  }

  void queue_pid5e_slot_tx(uint8_t b0, uint8_t b1, const char *label,
                           uint16_t delay_us = 0) {
    ::precision_plex::global_analyzer().queue_pid5e_slot_tx(b0, b1, label, delay_us);
  }

  void trigger_flight_recorder(const char *label, uint32_t duration_ms = 10000) {
    ::precision_plex::global_analyzer().trigger_flight_recorder(label, duration_ms);
  }

 protected:
  uart::UARTComponent *uart_parent_{nullptr};
};

}  // namespace precision_plex_lin
}  // namespace esphome
