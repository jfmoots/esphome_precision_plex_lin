#pragma once
#include "protocol.h"
#include "coach_state.h"

namespace precision_plex {

class PID37Decoder {
 public:
  void decode(const Frame &f) {
    const uint8_t zone = f.bytes[3];
    const uint8_t b4 = f.bytes[4];
    const uint8_t b5 = f.bytes[5];
    const uint8_t b6 = f.bytes[6];
    const uint8_t room = f.bytes[7];
    const uint8_t setp = f.bytes[8];
    const uint8_t lockout = f.bytes[9];
    const uint8_t unknown10 = f.len > 10 ? f.bytes[10] : 0;
    const auto raw = raw_string(f);
    const std::string payload = payload_string(f);

    const char *mode = decode_hvac_mode(zone, b4);
    const char *request_phase = decode_hvac_request_phase(b4, b5);
    const char *operating_state = decode_hvac_operating_state(b4, b6);

    CoachState &state = coach_state();
    HVACZoneState *zone_state = nullptr;
    if (zone == 1) zone_state = &state.hvac_zone_1;
    else if (zone == 2) zone_state = &state.hvac_zone_2;

    if (zone_state != nullptr) {
      zone_state->valid = true;
      zone_state->last_seen_ms = millis();
      zone_state->raw_zone = zone;
      zone_state->raw_mode = b4;
      zone_state->raw_status = b5;
      zone_state->raw_fan = b6;
      zone_state->raw_unknown_10 = unknown10;
      zone_state->room_temp_f = room;
      zone_state->setpoint_f = setp;
      zone_state->lockout_seconds = lockout;
      zone_state->compressor_lockout = lockout > 0;
      zone_state->mode = mode;
      zone_state->request_phase = request_phase;
      zone_state->operating_state = operating_state;
      zone_state->fan = operating_state;
      zone_state->raw_frame = raw;

      char summary[96];
      snprintf(summary, sizeof(summary),
               "Z=%02X B4=%02X B5=%02X B6=%02X RT=%02X SP=%02X LO=%02X B10=%02X",
               zone, b4, b5, b6, room, setp, lockout, unknown10);
      zone_state->field_summary = summary;
    }
    state.pid37_last_frame = raw;

    bool changed = false;
    if (zone == 1 && payload != last_z1_) { last_z1_ = payload; changed = true; }
    else if (zone == 2 && payload != last_z2_) { last_z2_ = payload; changed = true; }
    else if (zone != 1 && zone != 2) { changed = true; }
    if (!changed) return;
    if (zone_state != nullptr) zone_state->revision++;

    const char *zone_name = zone == 1 ? "Front" : zone == 2 ? "Rear" : "Unknown";

    const char *b5_desc = request_phase;

    ESP_LOGI(
      "HVAC",
      "%s Zone | mode=%s b4=%02X b5=%02X (%s) b6=%02X (%s) room=%uF set=%uF lockout=%us b10=%02X | raw=%s",
      zone_name, mode, b4, b5, b5_desc, b6, operating_state, room, setp, lockout, unknown10, raw.c_str()
    );
  }

 private:
  std::string last_z1_;
  std::string last_z2_;
};

}  // namespace precision_plex
