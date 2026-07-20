#pragma once
#include "protocol.h"
#include "coach_state.h"

namespace precision_plex {

class BADecoder {
 public:
  void decode(const Frame &f) {
    uint8_t page = f.bytes[3];

    if (page == 0x80) {
      decode_page_80(f);
      return;
    }

    uint8_t gen_state_nibble = (page >> 4) & 0x0F;
    if (is_generator_page_(gen_state_nibble)) {
      decode_generator_page_(f, page);
      decode_coach_telemetry_(f, page);
      return;
    }

    const auto raw = raw_string(f);
    ESP_LOGI("BA_OTHER", "page=%02X raw=%s", page, raw.c_str());
  }

 private:
  uint8_t last_telem_[16] = {0};
  uint8_t last_80_[16] = {0};
  bool have_telem_ = false;
  bool have_80_ = false;

  uint8_t last_generator_page_ = 0;
  bool have_generator_page_ = false;

  bool is_generator_page_(uint8_t high_nibble) {
    return high_nibble == 0x0 ||
           high_nibble == 0x1 ||
           high_nibble == 0x4 ||
           high_nibble == 0x5 ||
           high_nibble == 0x6 ||
           high_nibble == 0x7;
  }

  const char *generator_state_(uint8_t high_nibble) {
    if (high_nibble == 0x0) return "Idle/Off";
    if (high_nibble == 0x1) return "Running";
    if (high_nibble == 0x4) return "Manual Starting";
    if (high_nibble == 0x5) return "Manual Stopping";
    if (high_nibble == 0x6) return "Auto Starting";
    if (high_nibble == 0x7) return "Auto Stopping";
    return "Unknown";
  }

  const char *tank_level_(uint8_t v) {
    if (v == 0x0) return "Empty";
    if (v == 0x2) return "1/3";
    if (v == 0x4) return "2/3";
    if (v == 0x6) return "Full";
    return "Unknown";
  }

  const char *lp_level_(uint8_t v) {
    if (v == 0x0) return "0%";
    if (v == 0x1) return "25%";
    if (v == 0x3) return "50%";
    if (v == 0x5) return "75%";
    if (v == 0x6) return "100%";
    return "Unknown";
  }

  float battery_voltage_(uint8_t raw) {
    return ((float) raw) / 16.0f;
  }

  void decode_generator_page_(const Frame &f, uint8_t page) {
    uint8_t high = (page >> 4) & 0x0F;
    uint8_t low = page & 0x0F;

    CoachState &state = coach_state();
    state.pidba_last_frame = raw_string(f);
    state.telemetry.valid = true;
    state.telemetry.last_seen_ms = millis();
    state.telemetry.generator_state_raw = high;
    state.telemetry.generator_state = generator_state_from_nibble(high);
    state.telemetry.generator_runtime_tenths_digit = low;
    state.telemetry.raw_frame = raw_string(f);

    if (!have_generator_page_ || page != last_generator_page_) {
      uint8_t old_high = (last_generator_page_ >> 4) & 0x0F;
      uint8_t old_low = last_generator_page_ & 0x0F;

      ESP_LOGI(
        "GENERATOR_STATE",
        "State: %s -> %s | runtime tenths digit: %u -> %u | page=%02X raw=%s",
        have_generator_page_ ? generator_state_(old_high) : "Unknown",
        generator_state_(high),
        have_generator_page_ ? old_low : 0,
        low,
        page,
        raw_string(f).c_str()
      );

      last_generator_page_ = page;
      have_generator_page_ = true;
    }
  }

  void decode_coach_telemetry_(const Frame &f, uint8_t page) {
    uint8_t battery_raw = f.bytes[7];

    uint8_t lp    = (f.bytes[8] >> 4) & 0x0F;
    uint8_t fresh = f.bytes[8] & 0x0F;
    uint8_t black = (f.bytes[9] >> 4) & 0x0F;
    uint8_t grey  = f.bytes[9] & 0x0F;

    CoachState &state = coach_state();
    state.pidba_last_frame = raw_string(f);
    state.telemetry.valid = true;
    state.telemetry.last_seen_ms = millis();
    state.telemetry.house_battery_voltage = battery_voltage_from_raw(battery_raw);
    state.telemetry.propane_percent = propane_percent_from_code(lp);
    state.telemetry.fresh_tank_percent = tank_percent_from_code(fresh);
    state.telemetry.black_tank_percent = tank_percent_from_code(black);
    state.telemetry.grey_tank_percent = tank_percent_from_code(grey);
    state.telemetry.raw_frame = raw_string(f);

    uint8_t old_battery_raw = last_telem_[7];

    uint8_t old_lp    = (last_telem_[8] >> 4) & 0x0F;
    uint8_t old_fresh = last_telem_[8] & 0x0F;
    uint8_t old_black = (last_telem_[9] >> 4) & 0x0F;
    uint8_t old_grey  = last_telem_[9] & 0x0F;

    if (!have_telem_ || battery_raw != old_battery_raw) {
      ESP_LOGI(
        "BATTERY",
        "House Battery: %.2fV -> %.2fV raw=%02X page=%02X",
        have_telem_ ? battery_voltage_(old_battery_raw) : 0.0f,
        battery_voltage_(battery_raw),
        battery_raw,
        page
      );
    }

    if (!have_telem_ || lp != old_lp) {
      ESP_LOGI("LP", "Propane: %s -> %s page=%02X", have_telem_ ? lp_level_(old_lp) : "Unknown", lp_level_(lp), page);
    }

    if (!have_telem_ || fresh != old_fresh) {
      ESP_LOGI("TANK", "Fresh: %s -> %s page=%02X", have_telem_ ? tank_level_(old_fresh) : "Unknown", tank_level_(fresh), page);
    }

    if (!have_telem_ || grey != old_grey) {
      ESP_LOGI("TANK", "Grey: %s -> %s page=%02X", have_telem_ ? tank_level_(old_grey) : "Unknown", tank_level_(grey), page);
    }

    if (!have_telem_ || black != old_black) {
      ESP_LOGI("TANK", "Black: %s -> %s page=%02X", have_telem_ ? tank_level_(old_black) : "Unknown", tank_level_(black), page);
    }

    // Unknown BA telemetry payload fields only. Known:
    // b7 = House Battery
    // b8 high = LP
    // b8 low = Fresh
    // b9 high = Black
    // b9 low = Grey
    for (int i = 4; i <= 10; i++) {
      if (i == 7 || i == 8 || i == 9) continue;

      uint8_t oldv = last_telem_[i];
      uint8_t newv = f.bytes[i];

      if (have_telem_ && oldv != newv) {
        ESP_LOGI(
          "BA_TELEM_UNKNOWN",
          "page=%02X b%d byte %02X -> %02X | raw=%s",
          page,
          i,
          oldv,
          newv,
          raw_string(f).c_str()
        );
      }
    }

    for (int i = 4; i <= 10; i++) last_telem_[i] = f.bytes[i];
    have_telem_ = true;
  }

  void decode_page_80(const Frame &f) {
    bool changed = !have_80_;

    for (int i = 4; i <= 10; i++) {
      if (last_80_[i] != f.bytes[i]) changed = true;
    }

    if (changed && have_80_) {
      const auto raw = raw_string(f);
      ESP_LOGI("BA_80", "Secondary BA telemetry/status page changed | raw=%s", raw.c_str());
    }

    for (int i = 4; i <= 10; i++) last_80_[i] = f.bytes[i];
    have_80_ = true;
  }
};

}  // namespace precision_plex
