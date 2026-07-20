#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
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
    const uint32_t now = millis();
    auto &s = ::precision_plex::coach_state();
    const bool meaningful_change =
        s.outputs.revision != last_outputs_revision_ ||
        s.telemetry.revision != last_telemetry_revision_ ||
        s.power.revision != last_power_revision_ ||
        s.hvac_zone_1.revision != last_hvac_zone_1_revision_ ||
        s.hvac_zone_2.revision != last_hvac_zone_2_revision_;
    const bool heartbeat_due = now - last_snapshot_ms_ >= 2000;
    if (meaningful_change || heartbeat_due) {
      last_snapshot_ms_ = now;
      s.update_health();
      last_outputs_revision_ = s.outputs.revision;
      last_telemetry_revision_ = s.telemetry.revision;
      last_power_revision_ = s.power.revision;
      last_hvac_zone_1_revision_ = s.hvac_zone_1.revision;
      last_hvac_zone_2_revision_ = s.hvac_zone_2.revision;
      snapshot_sequence_++;
      snapshot_callback_.call(build_snapshot_(meaningful_change));
    }
  }

  void dump_config() override {
    ESP_LOGCONFIG("precision_plex_lin", "Precision Plex LIN Analyzer:");
    ESP_LOGCONFIG("precision_plex_lin", "  Version: %s", PRECISION_PLEX_LIN_VERSION);
    ESP_LOGCONFIG("precision_plex_lin", "  Legacy build: %s", PRECISION_PLEX_LIN_BUILD);
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  ::precision_plex::CoachState &state() { return ::precision_plex::coach_state(); }

  const char *firmware_version() const { return PRECISION_PLEX_LIN_FIRMWARE_VERSION; }

  void add_on_snapshot_callback(std::function<void(std::string)> &&callback) {
    snapshot_callback_.add(std::move(callback));
  }

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
  static void add_separator_(std::string &out, bool &first) {
    if (!first) out += ',';
    first = false;
  }

  static void add_key_(std::string &out, bool &first, const char *key) {
    add_separator_(out, first);
    out += '"';
    out += key;
    out += "\":";
  }

  static void add_bool_(std::string &out, bool &first, const char *key, bool value) {
    add_key_(out, first, key);
    out += value ? "true" : "false";
  }

  static void add_uint_(std::string &out, bool &first, const char *key, uint32_t value) {
    add_key_(out, first, key);
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(value));
    out += buffer;
  }

  static void add_int_or_null_(std::string &out, bool &first, const char *key, int value) {
    add_key_(out, first, key);
    if (value < 0) {
      out += "null";
      return;
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    out += buffer;
  }

  static void add_float_(std::string &out, bool &first, const char *key, float value) {
    add_key_(out, first, key);
    if (isnan(value)) {
      out += "null";
      return;
    }
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    out += buffer;
  }

  static void add_string_(std::string &out, bool &first, const char *key, const std::string &value) {
    add_key_(out, first, key);
    out += '"';
    for (char ch : value) {
      if (ch == '"' || ch == '\\') out += '\\';
      if (ch == '\n' || ch == '\r' || ch == '\t') {
        out += ' ';
      } else if (static_cast<unsigned char>(ch) >= 0x20) {
        out += ch;
      }
    }
    out += '"';
  }

  std::string build_snapshot_(bool meaningful_change) {
    const uint32_t now = millis();
    auto &s = ::precision_plex::coach_state();
    const bool telemetry_active = s.telemetry.valid && now - s.telemetry.last_seen_ms <= 10000;
    const bool outputs_active = s.outputs.valid && now - s.outputs.last_seen_ms <= 10000;
    const bool power_active = s.power.valid && now - s.power.last_seen_ms <= 10000;
    const bool zone_1_active = s.hvac_zone_1.valid && now - s.hvac_zone_1.last_seen_ms <= 10000;
    const bool zone_2_active = s.hvac_zone_2.valid && now - s.hvac_zone_2.last_seen_ms <= 10000;

    std::string out = "{";
    out.reserve(1800);
    bool first = true;
    add_uint_(out, first, "schema", 1);
    add_string_(out, first, "firmware_version", firmware_version());
    add_uint_(out, first, "snapshot_sequence", snapshot_sequence_);
    add_string_(out, first, "snapshot_reason", meaningful_change ? "change" : "heartbeat");
    add_uint_(out, first, "uptime_ms", now);
    add_bool_(out, first, "bus_active", s.lin_bus_active);
    add_bool_(out, first, "telemetry_active", telemetry_active);
    add_bool_(out, first, "outputs_active", outputs_active);
    add_bool_(out, first, "power_active", power_active);
    add_bool_(out, first, "hvac_zone_1_active", zone_1_active);
    add_bool_(out, first, "hvac_zone_2_active", zone_2_active);
    add_bool_(out, first, "touchscreen_seen", s.touchscreen_seen);
    add_bool_(out, first, "wireless_tp_seen", s.wireless_tp_seen);
    add_float_(out, first, "packets_per_second", s.packets_per_second);
    add_uint_(out, first, "known_packets", s.known_packets_total);
    add_uint_(out, first, "unknown_packets", s.unknown_packets_total);
    add_uint_(out, first, "crc_errors", s.bad_packets_total);
    add_string_(out, first, "last_pid", s.last_pid_hex);

    add_float_(out, first, "coach_voltage", s.telemetry.house_battery_voltage);
    add_int_or_null_(out, first, "fresh_water_level", s.telemetry.fresh_tank_percent);
    add_int_or_null_(out, first, "grey_water_level", s.telemetry.grey_tank_percent);
    add_int_or_null_(out, first, "black_water_level", s.telemetry.black_tank_percent);
    add_int_or_null_(out, first, "lp_gas_level", s.telemetry.propane_percent);
    add_string_(out, first, "generator_status", s.telemetry.generator_state);
    add_uint_(out, first, "generator_runtime_tenths_digit", s.telemetry.generator_runtime_tenths_digit);
    add_bool_(out, first, "generator_runtime_valid", s.telemetry.generator_runtime_valid);
    if (s.telemetry.generator_runtime_valid) {
      add_uint_(out, first, "generator_runtime_tenths", s.telemetry.generator_runtime_tenths);
      add_float_(out, first, "generator_runtime_hours", s.telemetry.generator_runtime_tenths / 10.0f);
    }

    add_bool_(out, first, "generator_running", s.outputs.generator_running_bit || s.telemetry.generator_state == "running");
    add_bool_(out, first, "awning_light", s.outputs.awning_light);
    add_bool_(out, first, "water_heater", s.outputs.water_heater);
    add_bool_(out, first, "tank_heater", s.outputs.tank_heater);
    add_bool_(out, first, "water_pump", s.outputs.water_pump);
    add_bool_(out, first, "awning_out", s.outputs.awning_extend);
    add_bool_(out, first, "awning_in", s.outputs.awning_retract);
    add_bool_(out, first, "sofa_slide_out", s.outputs.sofa_extend);
    add_bool_(out, first, "sofa_slide_in", s.outputs.sofa_retract);
    add_bool_(out, first, "bed_slide_out", s.outputs.bedroom_slide_extend);
    add_bool_(out, first, "bed_slide_in", s.outputs.bedroom_slide_retract);
    add_bool_(out, first, "wardrobe_slide_out", s.outputs.wardrobe_slide_extend);
    add_bool_(out, first, "wardrobe_slide_in", s.outputs.wardrobe_slide_retract);

    add_bool_(out, first, "ac_converter_present", s.power.ac_converter_present);
    add_bool_(out, first, "ignition_on", s.power.ignition_on);
    add_hvac_zone_(out, first, "hvac_zone_1", s.hvac_zone_1);
    add_hvac_zone_(out, first, "hvac_zone_2", s.hvac_zone_2);
    out += '}';
    return out;
  }

  static void add_hvac_zone_(std::string &out, bool &first, const char *prefix,
                             const ::precision_plex::HVACZoneState &zone) {
    std::string key(prefix);
    add_uint_(out, first, (key + "_room_temp").c_str(), zone.room_temp_f);
    add_uint_(out, first, (key + "_setpoint").c_str(), zone.setpoint_f);
    add_string_(out, first, (key + "_mode").c_str(), zone.mode);
    add_string_(out, first, (key + "_request_phase").c_str(), zone.request_phase);
    add_string_(out, first, (key + "_operating_state").c_str(), zone.operating_state);
    add_string_(out, first, (key + "_fan").c_str(), zone.fan);
    add_bool_(out, first, (key + "_compressor_lockout").c_str(), zone.compressor_lockout);
    add_uint_(out, first, (key + "_lockout_seconds").c_str(), zone.lockout_seconds);
    add_uint_(out, first, (key + "_mode_raw").c_str(), zone.raw_mode);
    add_uint_(out, first, (key + "_status_raw").c_str(), zone.raw_status);
    add_uint_(out, first, (key + "_fan_raw").c_str(), zone.raw_fan);
  }

  uart::UARTComponent *uart_parent_{nullptr};
  uint32_t last_snapshot_ms_{0};
  uint32_t snapshot_sequence_{0};
  uint32_t last_outputs_revision_{0};
  uint32_t last_telemetry_revision_{0};
  uint32_t last_power_revision_{0};
  uint32_t last_hvac_zone_1_revision_{0};
  uint32_t last_hvac_zone_2_revision_{0};
  CallbackManager<void(std::string)> snapshot_callback_;
};

class SnapshotTrigger : public Trigger<std::string> {
 public:
  explicit SnapshotTrigger(PrecisionPlexLinComponent *parent) {
    parent->add_on_snapshot_callback(
        [this](std::string snapshot) { this->trigger(snapshot); });
  }
};

}  // namespace precision_plex_lin
}  // namespace esphome
