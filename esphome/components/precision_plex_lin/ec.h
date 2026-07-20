#pragma once
#include "protocol.h"
#include "coach_state.h"

namespace precision_plex {

class ECDecoder {
 public:
  void decode(const Frame &f) {
    const auto raw = raw_string(f);
    uint8_t b5 = f.bytes[5];

    bool ac_present = b5 & 0x04;
    bool ignition = b5 & 0x01;

    CoachState &state = coach_state();
    state.pidec_last_frame = raw;
    state.power.valid = true;
    state.power.last_seen_ms = millis();
    state.power.ac_converter_present = ac_present;
    state.power.ignition_on = ignition;
    state.power.raw_b5 = b5;
    state.power.raw_frame = raw;

    if (!have_ || b5 != last_b5_) {
      state.power.revision++;
      ESP_LOGI(
        "EC_FLAGS",
        "AC/Converter=%s Ignition=%s raw_b5=%02X",
        ac_present ? "ON" : "OFF",
        ignition ? "ON" : "OFF",
        b5
      );
    }

    for (int i = 3; i <= 6; i++) {
      if (i == 5) continue;

      if (have_ && last_[i] != f.bytes[i]) {
        ESP_LOGI(
          "EC_UNKNOWN",
          "b%d byte %02X -> %02X | raw=%s",
          i,
          last_[i],
          f.bytes[i],
          raw.c_str()
        );
      }
    }

    for (int i = 3; i <= 6; i++) last_[i] = f.bytes[i];
    last_b5_ = b5;
    have_ = true;
  }

 private:
  uint8_t last_[16] = {0};
  uint8_t last_b5_ = 0;
  bool have_ = false;
};

}  // namespace precision_plex
