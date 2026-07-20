#pragma once

#include <Arduino.h>
#include <string>
#include <math.h>

namespace precision_plex {

struct HVACZoneState {
  bool valid = false;
  uint32_t last_seen_ms = 0;
  uint32_t revision = 0;

  uint8_t raw_zone = 0;
  uint8_t raw_mode = 0;       // PID37 byte 4
  uint8_t raw_status = 0;     // PID37 byte 5
  uint8_t raw_fan = 0;        // PID37 byte 6
  uint8_t room_temp_f = 0;    // PID37 byte 7
  uint8_t setpoint_f = 0;     // PID37 byte 8
  uint8_t lockout_seconds = 0;// PID37 byte 9
  uint8_t raw_unknown_10 = 0; // PID37 byte 10
  std::string field_summary;

  bool compressor_lockout = false;
  std::string mode = "unknown";
  std::string request_phase = "unknown";
  std::string operating_state = "unknown";
  // Legacy compatibility: existing ESPHome text sensor named "Fan" now carries operating_state.
  std::string fan = "unknown";
  std::string raw_frame;
};

struct OutputState {
  bool valid = false;
  uint32_t last_seen_ms = 0;
  uint32_t revision = 0;

  bool awning_light = false;
  bool water_heater = false;
  bool tank_heater = false;
  bool water_pump = false;

  bool sofa_extend = false;
  bool sofa_retract = false;
  bool awning_extend = false;
  bool awning_retract = false;
  bool bedroom_slide_extend = false;
  bool bedroom_slide_retract = false;
  bool wardrobe_slide_extend = false;
  bool wardrobe_slide_retract = false;

  bool generator_running_bit = false;
  std::string raw_frame;
};

struct CoachTelemetryState {
  bool valid = false;
  uint32_t last_seen_ms = 0;
  uint32_t revision = 0;

  float house_battery_voltage = NAN;
  int propane_percent = -1;
  int fresh_tank_percent = -1;
  int grey_tank_percent = -1;
  int black_tank_percent = -1;

  std::string generator_state = "unknown";
  uint8_t generator_state_raw = 0;
  uint8_t generator_runtime_tenths_digit = 0;
  bool generator_runtime_valid = false;
  uint32_t generator_runtime_tenths = 0;
  std::string raw_frame;
};

struct PowerState {
  bool valid = false;
  uint32_t last_seen_ms = 0;
  uint32_t revision = 0;

  bool ac_converter_present = false;
  bool ignition_on = false;
  uint8_t raw_b5 = 0;
  std::string raw_frame;
};

struct CoachState {
  bool lin_bus_active = false;
  bool touchscreen_seen = false;
  bool wireless_tp_seen = false;

  uint32_t last_frame_ms = 0;
  uint32_t packets_total = 0;
  uint32_t known_packets_total = 0;
  uint32_t unknown_packets_total = 0;
  uint32_t bad_packets_total = 0;

  uint32_t last_sample_ms = 0;
  uint32_t last_sample_packets = 0;
  float packets_per_second = 0.0f;

  uint8_t last_pid = 0;
  std::string last_pid_hex = "--";
  std::string last_frame_raw;

  std::string pid1f_last_command;
  std::string pid1f_last_non_idle_command;
  std::string pid1f_last_learned_command;
  std::string pid1f_command_history;

  bool lin_tx_enabled = true;
  uint32_t lin_tx_count = 0;
  std::string lin_tx_last_attempt = "none";
  std::string lin_tx_last_result = "ready; no slot TX attempted since boot";
  std::string lin_tx_post_slot_capture = "none";
  bool lin_tx_echo_seen = false;
  bool lin_tx_collision_suspected = false;
  std::string pid1f_response_timing = "no PID1F timing capture yet";
  std::string pid5e_last_frame;
  std::string pid5e_last_non_idle_command;
  std::string pid5e_last_learned_command;
  std::string pid5e_command_history;
  std::string flight_recorder_status = "idle; no capture yet";
  std::string flight_recorder_capture = "none";
  std::string pid32_last_frame;
  std::string pid37_last_frame;
  std::string pid76_last_frame;
  std::string pid9c_last_frame;
  std::string pidba_last_frame;
  std::string piddd_last_frame;
  std::string pidec_last_frame;

  HVACZoneState hvac_zone_1;
  HVACZoneState hvac_zone_2;
  OutputState outputs;
  CoachTelemetryState telemetry;
  PowerState power;

  void observe_valid_frame(uint8_t pid, const std::string &raw) {
    packets_total++;
    known_packets_total++;
    last_frame_ms = millis();
    lin_bus_active = true;
    last_pid = pid;

    char pid_buf[4];
    snprintf(pid_buf, sizeof(pid_buf), "%02X", pid);
    last_pid_hex = pid_buf;
    last_frame_raw = raw;

    if (pid == 0x1F) touchscreen_seen = true;
    if (pid == 0x5E) wireless_tp_seen = true;
  }

  void observe_unknown_pid(uint8_t pid) {
    packets_total++;
    unknown_packets_total++;
    last_frame_ms = millis();
    lin_bus_active = true;
    last_pid = pid;

    char pid_buf[4];
    snprintf(pid_buf, sizeof(pid_buf), "%02X", pid);
    last_pid_hex = pid_buf;
  }

  void observe_bad_frame() {
    packets_total++;
    bad_packets_total++;
    last_frame_ms = millis();
    lin_bus_active = true;
  }

  void update_health() {
    const uint32_t now = millis();
    lin_bus_active = last_frame_ms != 0 && (now - last_frame_ms) < 3000;

    if (last_sample_ms == 0) {
      last_sample_ms = now;
      last_sample_packets = packets_total;
      return;
    }

    const uint32_t elapsed = now - last_sample_ms;
    if (elapsed >= 1000) {
      packets_per_second = ((float)(packets_total - last_sample_packets) * 1000.0f) / (float)elapsed;
      last_sample_ms = now;
      last_sample_packets = packets_total;
    }
  }
};

inline CoachState &coach_state() {
  static CoachState state;
  return state;
}


inline void append_output_diff(std::string &out, const char *name, bool before, bool after) {
  if (before == after) return;
  if (!out.empty()) out += "; ";
  out += name;
  out += after ? " ON" : " OFF";
}

inline std::string describe_output_diff(const OutputState &before, const OutputState &after) {
  std::string out;
  append_output_diff(out, "Awning Light", before.awning_light, after.awning_light);
  append_output_diff(out, "Water Heater", before.water_heater, after.water_heater);
  append_output_diff(out, "Tank Heater", before.tank_heater, after.tank_heater);
  append_output_diff(out, "Water Pump", before.water_pump, after.water_pump);
  append_output_diff(out, "Sofa Extend", before.sofa_extend, after.sofa_extend);
  append_output_diff(out, "Sofa Retract", before.sofa_retract, after.sofa_retract);
  append_output_diff(out, "Awning Retract", before.awning_retract, after.awning_retract);
  append_output_diff(out, "Awning Extend", before.awning_extend, after.awning_extend);
  append_output_diff(out, "Bedroom Slide Extend", before.bedroom_slide_extend, after.bedroom_slide_extend);
  append_output_diff(out, "Bedroom Slide Retract", before.bedroom_slide_retract, after.bedroom_slide_retract);
  append_output_diff(out, "Wardrobe Slide Retract", before.wardrobe_slide_retract, after.wardrobe_slide_retract);
  append_output_diff(out, "Wardrobe Slide Extend", before.wardrobe_slide_extend, after.wardrobe_slide_extend);
  append_output_diff(out, "Generator Running Bit", before.generator_running_bit, after.generator_running_bit);
  if (out.empty()) out = "no PID32 output change observed";
  return out;
}


inline void append_hvac_diff(std::string &out, const char *name, const std::string &before, const std::string &after) {
  if (before == after) return;
  if (!out.empty()) out += "; ";
  out += name;
  out += " ";
  out += before;
  out += "->";
  out += after;
}

inline void append_hvac_byte_diff(std::string &out, const char *name, uint8_t before, uint8_t after) {
  if (before == after) return;
  if (!out.empty()) out += "; ";
  char part[48];
  snprintf(part, sizeof(part), "%s %02X->%02X", name, before, after);
  out += part;
}

inline std::string describe_hvac_diff(const HVACZoneState &before, const HVACZoneState &after) {
  if (!after.valid) return "no PID37 HVAC change observed";

  std::string out;
  char zone_name[16];
  snprintf(zone_name, sizeof(zone_name), "Zone %u", after.raw_zone);
  out += zone_name;
  out += ": ";

  std::string changes;
  append_hvac_diff(changes, "mode", before.mode, after.mode);
  append_hvac_diff(changes, "phase", before.request_phase, after.request_phase);
  append_hvac_diff(changes, "state", before.operating_state, after.operating_state);
  append_hvac_byte_diff(changes, "room", before.room_temp_f, after.room_temp_f);
  append_hvac_byte_diff(changes, "setpoint", before.setpoint_f, after.setpoint_f);
  append_hvac_byte_diff(changes, "lockout", before.lockout_seconds, after.lockout_seconds);

  if (changes.empty()) return "no PID37 HVAC change observed";
  out += changes;
  return out;
}

inline void append_generator_diff(std::string &out, const char *name, const std::string &before, const std::string &after) {
  if (before == after) return;
  if (!out.empty()) out += "; ";
  out += name;
  out += " ";
  out += before;
  out += "->";
  out += after;
}

inline void append_generator_byte_diff(std::string &out, const char *name, uint8_t before, uint8_t after) {
  if (before == after) return;
  if (!out.empty()) out += "; ";
  char part[56];
  snprintf(part, sizeof(part), "%s %02X->%02X", name, before, after);
  out += part;
}

inline std::string describe_generator_diff(const CoachTelemetryState &before, const CoachTelemetryState &after) {
  if (!after.valid) return "no PIDBA generator change observed";
  std::string out;
  append_generator_diff(out, "generator_state", before.generator_state, after.generator_state);
  append_generator_byte_diff(out, "gen_state_raw", before.generator_state_raw, after.generator_state_raw);
  append_generator_byte_diff(out, "runtime_tenths", before.generator_runtime_tenths_digit, after.generator_runtime_tenths_digit);
  if (before.generator_runtime_valid != after.generator_runtime_valid ||
      before.generator_runtime_tenths != after.generator_runtime_tenths) {
    if (!out.empty()) out += "; ";
    char part[80];
    snprintf(part, sizeof(part), "runtime_total %s%lu->%s%lu",
             before.generator_runtime_valid ? "" : "invalid/",
             static_cast<unsigned long>(before.generator_runtime_tenths),
             after.generator_runtime_valid ? "" : "invalid/",
             static_cast<unsigned long>(after.generator_runtime_tenths));
    out += part;
  }
  if (out.empty()) return "no PIDBA generator change observed";
  return out;
}

inline const char *decode_hvac_mode(uint8_t zone, uint8_t b4) {
  // PID37 B4 is zone-aware. Some mode bytes differ between zones.
  if (zone == 1) {
    if (b4 == 0x00) return "off";
    if (b4 == 0x10) return "auto_heat_candidate";
    if (b4 == 0x11) return "heat_pump";
    if (b4 == 0x12) return "furnace";
    if (b4 == 0x20) return "cool";
  } else if (zone == 2) {
    if (b4 == 0x01) return "off";
    if (b4 == 0x11) return "heat_pump";
    if (b4 == 0x21) return "cool";
  }

  // Safe fallbacks for values observed in multiple contexts.
  if (b4 == 0x11) return "heat_pump";
  return "unknown";
}

inline const char *decode_hvac_request_phase(uint8_t mode, uint8_t b5) {
  // PID37 B5 is best understood as request/phase/context.
  // Furnace uses this byte heavily; heat pump and cooling use simpler request contexts.
  if (mode == 0x12) {
    if (b5 == 0x50) return "furnace_selected_idle";
    if (b5 == 0x54) return "furnace_call_ignition";
    if (b5 == 0x04) return "furnace_heat_active";
    if (b5 == 0x00) return "furnace_satisfied_post_cycle";
  }

  if (b5 == 0x00) return "idle_manual_or_no_request";
  if (b5 == 0x01) return "heat_pump_request_context";
  if (b5 == 0x10) return "cooling_request_context";
  if (b5 == 0x50) return "controller_satisfied_or_waiting_candidate";
  if (b5 == 0x54) return "furnace_call_ignition";
  if (b5 == 0x04) return "furnace_heat_active";
  return "unknown";
}

inline const char *decode_hvac_operating_state(uint8_t mode, uint8_t b6) {
  // PID37 B6 is current operating state. Furnace blower is not reported here;
  // furnace phase should be read from B5 while B4=0x12.
  if (mode == 0x12) {
    if (b6 == 0x50) return "furnace_context";
  }

  if (b6 == 0x00) return "off_idle";
  if (b6 == 0x11) return "fan_low_running";
  if (b6 == 0x37) return "fan_high_running";
  if (b6 == 0x40) return "cool_satisfied";
  if (b6 == 0x41) return "cooling_active_low";
  if (b6 == 0x47) return "cooling_active_high";
  if (b6 == 0x50) return "heat_selected_or_waiting";
  if (b6 == 0x51) return "heat_pump_active_low";
  if (b6 == 0x57) return "heat_pump_active_high_candidate";
  return "unknown";
}

// Backward-compatible helper for the existing text sensor named HVAC Zone X Fan.
inline const char *decode_hvac_fan(uint8_t fan) {
  return decode_hvac_operating_state(0xFF, fan);
}

inline int tank_percent_from_code(uint8_t v) {
  if (v == 0x0) return 0;
  if (v == 0x2) return 33;
  if (v == 0x4) return 67;
  if (v == 0x6) return 100;
  return -1;
}

inline int propane_percent_from_code(uint8_t v) {
  if (v == 0x0) return 0;
  if (v == 0x1) return 25;
  if (v == 0x3) return 50;
  if (v == 0x5) return 75;
  if (v == 0x6) return 100;
  return -1;
}

inline const char *generator_state_from_nibble(uint8_t high_nibble) {
  if (high_nibble == 0x0) return "idle_off";
  if (high_nibble == 0x1) return "running";
  if (high_nibble == 0x4) return "manual_starting";
  if (high_nibble == 0x5) return "manual_stopping";
  if (high_nibble == 0x6) return "auto_starting";
  if (high_nibble == 0x7) return "auto_stopping";
  return "unknown";
}

inline float battery_voltage_from_raw(uint8_t raw) {
  return ((float) raw) / 16.0f;
}

}  // namespace precision_plex
