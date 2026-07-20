#pragma once
#include "protocol.h"

namespace precision_plex {

class SchedulerDecoder {
 public:
  void decode_9c(const Frame &f) {
    const std::string payload = payload_string(f);
    if (payload == last_9c_) return;
    last_9c_ = payload;

    const uint8_t b3 = f.bytes[3];
    const uint8_t b4 = f.bytes[4];
    const char *label = label_9c_(b3, b4);
    ESP_LOGD("PID_9C", "payload=%02X %02X | %s | raw=%s", b3, b4, label, raw_string(f).c_str());
  }

  void decode_dd(const Frame &f) {
    const std::string payload = payload_string(f);
    if (payload == last_dd_) return;
    last_dd_ = payload;

    const uint8_t b3 = f.bytes[3];
    const uint8_t b4 = f.bytes[4];
    const char *label = label_dd_(b3, b4);
    ESP_LOGD("PID_DD", "payload=%02X %02X | %s | raw=%s", b3, b4, label, raw_string(f).c_str());
  }

  void decode_5e(const Frame &f) {
    const std::string payload = payload_string(f);
    if (payload == last_5e_) return;
    last_5e_ = payload;

    const auto raw = raw_string(f);
    if (f.bytes[3] == 0x3F && f.bytes[4] == 0x00) {
      ESP_LOGD("PID_5E", "Wireless TP/bridge channel: idle/housekeeping 3F 00 | raw=%s", raw.c_str());
    } else if (f.bytes[3] == 0xB0 && f.bytes[4] == 0x02) {
      ESP_LOGD("PID_5E", "Wireless TP/bridge channel: housekeeping pulse B0 02 | raw=%s", raw.c_str());
    } else if (f.bytes[3] == 0xB1 && f.bytes[4] == 0x01) {
      ESP_LOGD("PID_5E", "Wireless TP/bridge channel: housekeeping pulse B1 01 | raw=%s", raw.c_str());
    } else {
      ESP_LOGI("PID_5E", "Wireless TP/bridge channel: unmapped payload=%02X %02X | raw=%s", f.bytes[3], f.bytes[4], raw.c_str());
    }
  }

 private:
  std::string last_9c_;
  std::string last_dd_;
  std::string last_5e_;

  const char *label_9c_(uint8_t b3, uint8_t b4) {
    if (b3 != 0x3E) return "unmapped non-3E scheduler/HVAC auxiliary";

    // E0/E4/E6 were original scheduler phases. E1/E3/E5 appeared strongly during
    // HVAC activity and may carry touchscreen/HVAC auxiliary traffic, including
    // setpoint or room-temperature reporting. Keep them visible but no longer
    // call them unexpected.
    switch (b4) {
      case 0xE0: return "scheduler/housekeeping phase E0";
      case 0xE1: return "HVAC auxiliary phase E1";
      case 0xE3: return "HVAC auxiliary phase E3";
      case 0xE4: return "scheduler/housekeeping phase E4";
      case 0xE5: return "HVAC auxiliary phase E5";
      case 0xE6: return "scheduler/housekeeping phase E6";
      default: return "unmapped 3E scheduler/HVAC auxiliary phase";
    }
  }

  const char *label_dd_(uint8_t b3, uint8_t b4) {
    if (b3 != 0x3E) return "unmapped non-3E scheduler/auxiliary";
    switch (b4) {
      case 0xD0: return "scheduler/housekeeping phase D0";
      case 0xD4: return "scheduler/housekeeping phase D4";
      case 0xD6: return "scheduler/housekeeping phase D6";
      default: return "unmapped 3E scheduler/auxiliary phase";
    }
  }
};

}  // namespace precision_plex
