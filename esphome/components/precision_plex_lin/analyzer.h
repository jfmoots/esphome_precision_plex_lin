#pragma once
#include <Arduino.h>
#include <deque>
#include "esphome/core/helpers.h"
#include "esphome/components/uart/uart.h"
#include "protocol.h"
#include "coach_state.h"
#include "scheduler.h"
#include "pid32.h"
#include "pid37.h"
#include "ba.h"
#include "ec.h"
#include "pid1f.h"
#include "command_intent.h"

namespace precision_plex {

class Analyzer {
 public:
  void release_wireless_tp_mute() {
    release_wireless_tp_mute_("manual/bootstrap release");
  }

  void queue_pid1f_slot_tx(uint8_t b0, uint8_t b1, const char *label, uint16_t delay_us = 0) {
    pending_slot_tx_ = true;
    pending_slot_b0_ = b0;
    pending_slot_b1_ = b1;
    pending_slot_label_ = label ? label : "PID1F command";
    pending_slot_delay_us_ = delay_us;
    pending_slot_queued_ms_ = millis();

    uint8_t checksum = classic_checksum_(0x1F, b0, b1);
    char attempt[160];
    snprintf(attempt, sizeof(attempt), "%s / queued payload %02X %02X checksum %02X; delay=%uus; waiting for PID1F slot",
             pending_slot_label_.c_str(), b0, b1, checksum, (unsigned) pending_slot_delay_us_);

    CoachState &state = coach_state();
    state.lin_tx_enabled = true;
    state.lin_tx_last_attempt = attempt;
    state.lin_tx_last_result = "queued; waiting for PMM PID1F poll/header";

    ESP_LOGW("LIN_TX", "Queued slot TX: %s", attempt);
  }



  void queue_pid5e_slot_tx(uint8_t b0, uint8_t b1, const char *label, uint16_t delay_us = 0) {
    if (!output_command_request_.empty() || awning_phase_ != AwningJogPhase::IDLE) {
      ESP_LOGW("LIN_TX", "Ignored diagnostic toggle while a desired-state command is running");
      return;
    }
    if (pid5e_commands_.size() >= 8) {
      ESP_LOGW("LIN_TX", "PID5E queue full; rejected %s", label ? label : "command");
      return;
    }
    pid5e_commands_.push_back({b0, b1, delay_us, label ? label : "PID5E command"});
    if (!wireless_tp_mute_asserted_ && !pid5e_waiting_for_idle_) arm_pid5e_command_();
  }

  struct OutputCommandResult {
    std::string request_id;
    const char *status = "idle";
    uint32_t revision = 0;
  };

  const OutputCommandResult &output_command_result() const { return output_command_result_; }

  void set_output(const std::string &key, bool desired, const std::string &request_id) {
    if (request_id.empty() || request_id.size() > 64) return;
    if (request_id == output_command_request_ || request_id == output_command_result_.request_id) return;

    uint8_t opcode;
    if (key == "awning_light") opcode = 0x00;
    else if (key == "water_heater") opcode = 0x04;
    else if (key == "water_pump") opcode = 0x07;
    else {
      publish_output_command_(request_id, "unsupported");
      return;
    }

    if (!output_command_request_.empty() || !pid5e_commands_.empty() ||
        awning_phase_ != AwningJogPhase::IDLE) {
      publish_output_command_(request_id, "busy");
      return;
    }

    const auto &outputs = coach_state().outputs;
    if (!outputs.valid || millis() - outputs.last_seen_ms > OUTPUT_FRESH_MS) {
      publish_output_command_(request_id, "not_sent");
      return;
    }
    if (output_for_opcode_(opcode) == desired) {
      publish_output_command_(request_id, "confirmed");
      return;
    }

    output_command_request_ = request_id;
    output_command_opcode_ = opcode;
    output_command_desired_ = desired;
    output_command_started_ms_ = millis();
    output_command_sent_ = false;
    pid5e_commands_.push_back({opcode, 0x00, 0, "desired PID32 output"});
    if (!pid5e_waiting_for_idle_) arm_pid5e_command_();
  }

  const OutputCommandResult &awning_motion_result() const { return awning_motion_result_; }

  void awning_motion(const std::string &operation, bool extend,
                     const std::string &request_id, int max_ms) {
    if (request_id.empty() || request_id.size() > 64) return;
    if (operation == "start") {
      if (request_id == awning_motion_result_.request_id) return;
      if (max_ms <= 0 || max_ms > 90000 || !can_start_awning_()) {
        // Keep the result of an active owner available to its client.
        if (awning_phase_ == AwningJogPhase::IDLE)
          publish_awning_motion_(request_id, "refused");
        return;
      }
      publish_awning_motion_(request_id, "armed");
      awning_max_ms_ = static_cast<uint32_t>(max_ms);
      awning_five_second_test_ = false;
      arm_awning_(extend);
      return;
    }
    if (awning_max_ms_ == 0 || request_id != awning_motion_result_.request_id) return;
    if (operation == "renew" &&
        (awning_phase_ == AwningJogPhase::START || awning_phase_ == AwningJogPhase::HOLD)) {
      awning_renewed_ms_ = millis();
    } else if (operation == "stop") {
      stop_awning_jog();
    }
  }

  void start_awning_jog(bool extend, bool five_second_test = false) {
    if (!can_start_awning_()) {
      ESP_LOGW("LIN_AWNING", "Jog refused: busy, unsafe or stale coach state");
      return;
    }
    awning_max_ms_ = 0;
    awning_five_second_test_ = five_second_test;
    arm_awning_(extend);
  }

  void stop_awning_jog() {
    if (awning_phase_ == AwningJogPhase::START) {
      finish_awning_jog_("canceled before transmitting", "canceled");
    } else if (awning_phase_ == AwningJogPhase::HOLD || awning_phase_ == AwningJogPhase::RELEASE) {
      release_awning_("stopped");
    }
  }

  void trigger_flight_recorder(const char *label, uint32_t duration_ms = 10000) {
    flight_recorder_active_ = true;
    flight_recorder_start_ms_ = millis();
    flight_recorder_duration_ms_ = duration_ms;
    flight_recorder_label_ = label ? label : "manual all-PID capture";
    flight_recorder_frames_.clear();
    flight_recorder_frames_.reserve(FLIGHT_FRAME_LIMIT);
    flight_dump_index_ = 0;
    flight_recorder_frame_count_ = 0;

    CoachState &state = coach_state();
    char status[180];
    snprintf(status, sizeof(status), "active; %s; duration=%lums; now perform HA/mobile action",
             flight_recorder_label_.c_str(), (unsigned long) flight_recorder_duration_ms_);
    state.flight_recorder_status = status;
    state.flight_recorder_capture = "capture active; waiting for frames";
    ESP_LOGW("LIN_FLIGHT", "%s", status);
  }

  void process(esphome::uart::UARTComponent *uart) {
    expire_awning_jog_();
    expire_output_command_();
    expire_pid5e_command_();
    while (uart->available()) {
      uint8_t c;
      if (!uart->read_byte(&c)) break;

      // Build 010.6 fast-path: when a PID5E command is queued, react to the
      // raw byte stream as soon as the PMM header 00 55 5E is observed.  The
      // older parser-level TX waited until normal frame processing, which let
      // the factory Wireless TP answer idle (3F 00 62) first.  This path is
      // intentionally before timing sniffers, post-slot capture, and buffer
      // maintenance so the response can start as early as possible.
      const bool fast_pid5e_tx_sent = maybe_send_pending_pid5e_fast_response_(uart, c);

      // PID1F response timing was useful during the touchscreen race.  It is
      // intentionally disabled in 010.6 to reduce per-byte latency while
      // testing the PID5E fast-response path.
      // sniff_pid1f_response_timing_(c);

      if (!fast_pid5e_tx_sent) capture_post_slot_byte_(c);
      buf_.push_back(c);

      if (buf_.size() > 128) {
        ESP_LOGW("LIN_SYNC", "Buffer overflow, clearing. tail=%s", buffer_tail_string_(64).c_str());
        buf_.clear();
        return;
      }

      while (buf_.size() >= 2 && !(buf_[0] == 0x00 && buf_[1] == 0x55)) {
        buf_.erase(buf_.begin());
      }

      if (buf_.size() < 3) continue;

      uint8_t pid = buf_[2];

      // Build 009.7: slot-timed PID1F TX.
      // Do not initiate a complete LIN frame asynchronously.  Queue a payload,
      // then respond only when the PMM has just issued the PID1F header
      // (BREAK/SYNC/PID observed here as 00 55 1F).
      maybe_send_pending_pid1f_slot_response_(uart, pid);

      int len = expected_len(pid);
      if (len == 0) {
        coach_state().observe_unknown_pid(pid);
        count_unknown_++;
        buf_.erase(buf_.begin());
        continue;
      }

      if ((int) buf_.size() < len) continue;

      // A PID76 header without a response can immediately precede PID32.
      // During retract, its overlapping byte window can pass the PID76
      // checksum. Check the entire PID32 candidate before accepting the alias.
      constexpr int HEADER_LEN = 3;
      if (pid == 0x76 && buf_[3] == 0x00 && buf_[4] == 0x55 && buf_[5] == 0x32) {
        const Frame motion = buffered_frame_(0x32, expected_len(0x32), HEADER_LEN);
        if (motion.len == 0) continue;
        if (checksum_ok(motion)) {
          buf_.erase(buf_.begin(), buf_.begin() + HEADER_LEN + motion.len);
          handle_frame(motion);
          expire_pid1f_response_timing_if_needed_();
          continue;
        }
      }

      const Frame f = buffered_frame_(pid, len);

      if (!checksum_ok(f)) {
        if (tx_capture_active_) publish_post_slot_capture_("bad_frame_during_post_slot_capture");
        coach_state().observe_bad_frame();
        count_bad_++;
        int embedded = -1;
        for (int i = 2; i < len - 1; i++) {
          if (f.bytes[i] == 0x00 && f.bytes[i + 1] == 0x55) {
            embedded = i;
            break;
          }
        }
        if (embedded > 0) buf_.erase(buf_.begin(), buf_.begin() + embedded);
        else buf_.erase(buf_.begin());
        continue;
      }

      buf_.erase(buf_.begin(), buf_.begin() + len);
      handle_frame(f);
      expire_pid1f_response_timing_if_needed_();
    }
    expire_pid1f_response_timing_if_needed_();
    expire_flight_recorder_if_needed_();
    dump_flight_chunk_();
    if (tx_capture_active_ && millis() - tx_capture_started_ms_ > 100) {
      publish_post_slot_capture_("capture_timeout_100ms");
    }
  }

 private:
  Frame buffered_frame_(uint8_t pid, int len, int offset = 0) const {
    Frame frame;
    if (buf_.size() < static_cast<size_t>(offset + len)) return frame;
    frame.pid = pid;
    frame.len = len;
    std::memcpy(frame.bytes, buf_.data() + offset, len);
    return frame;
  }

  static constexpr uint8_t WIRELESS_TP_MUTE_GPIO = 25;
  bool wireless_tp_mute_asserted_ = false;

  void assert_wireless_tp_mute_(const char *reason) {
    // Open-drain style: drive LOW to claim, never drive HIGH.
    // External wiring: GPIO25 -> 1k resistor -> Wireless TP MCP2004E FAULT/TXE pad.
    pinMode(WIRELESS_TP_MUTE_GPIO, OUTPUT);
    digitalWrite(WIRELESS_TP_MUTE_GPIO, LOW);
    wireless_tp_mute_asserted_ = true;
    ESP_LOGW("LIN_MUTE", "Wireless TP claimed on GPIO25: %s", reason ? reason : "asserted");
  }

  void release_wireless_tp_mute_(const char *reason) {
    // Float the line so the Wireless TP board controls/pulls FAULT/TXE normally.
    pinMode(WIRELESS_TP_MUTE_GPIO, INPUT);
    wireless_tp_mute_asserted_ = false;
    ESP_LOGW("LIN_MUTE", "Wireless TP claim released on GPIO25: %s", reason ? reason : "released");
  }

  std::vector<uint8_t> buf_;
  PID32Decoder pid32_;
  PID37Decoder pid37_;
  BADecoder ba_;
  ECDecoder ec_;
  PID1FDecoder pid1f_;
  SchedulerDecoder scheduler_;
  CommandIntentObserver command_intent_;

  uint32_t count_ba_ = 0;
  uint32_t count_76_ = 0;
  uint32_t count_32_ = 0;
  uint32_t count_37_z1_ = 0;
  uint32_t count_37_z2_ = 0;
  uint32_t count_9c_ = 0;
  uint32_t count_dd_ = 0;
  uint32_t count_ec_ = 0;
  uint32_t count_1f_ = 0;
  uint32_t count_5e_ = 0;
  uint32_t count_bad_ = 0;
  uint32_t count_unknown_ = 0;
  uint32_t last_report_ms_ = 0;

  bool pending_slot_tx_ = false;
  uint32_t pending_slot_queued_ms_ = 0;
  uint8_t pending_slot_b0_ = 0;
  uint8_t pending_slot_b1_ = 0;
  uint16_t pending_slot_delay_us_ = 0;
  std::string pending_slot_label_;

  struct PID5ECommand {
    uint8_t b0;
    uint8_t b1;
    uint16_t delay_us;
    std::string label;
  };
  std::deque<PID5ECommand> pid5e_commands_;
  esphome::HighFrequencyLoopRequester pid5e_fast_loop_;
  uint32_t pid5e_phase_started_ms_ = 0;
  bool pid5e_waiting_for_idle_ = false;
  enum class AwningJogPhase { IDLE, START, HOLD, RELEASE, VERIFY };
  AwningJogPhase awning_phase_ = AwningJogPhase::IDLE;
  bool awning_extend_ = false;
  bool awning_five_second_test_ = false;
  uint8_t awning_hold_slots_ = 0;
  bool awning_motion_observed_ = false;
  bool awning_jog_locked_ = false;
  uint32_t awning_started_ms_ = 0;
  uint32_t awning_release_ms_ = 0;
  uint32_t awning_max_ms_ = 0;
  uint32_t awning_renewed_ms_ = 0;
  const char *awning_stop_status_ = "stopped";
  OutputCommandResult awning_motion_result_;

  bool can_start_awning_() const {
    const auto &out = coach_state().outputs;
    return !awning_jog_locked_ && awning_phase_ == AwningJogPhase::IDLE &&
        !pid5e_waiting_for_idle_ && !wireless_tp_mute_asserted_ &&
        !pending_slot_tx_ && pid5e_commands_.empty() && output_command_request_.empty() &&
        awning_inputs_safe_() && !out.awning_extend && !out.awning_retract;
  }

  void arm_awning_(bool extend) {
    awning_extend_ = extend;
    awning_motion_observed_ = false;
    awning_hold_slots_ = 0;
    awning_started_ms_ = awning_renewed_ms_ = millis();
    awning_phase_ = AwningJogPhase::START;
    awning_stop_status_ = "stopped";
    pid5e_fast_loop_.start();
    assert_wireless_tp_mute_("awning motion");
    ESP_LOGI("LIN_AWNING", "%s %s armed", awning_max_ms_ ? "Leased" : "Bounded jog",
             extend ? "extend" : "retract");
  }

  void publish_awning_motion_(const std::string &request_id, const char *status) {
    awning_motion_result_.request_id = request_id;
    awning_motion_result_.status = status;
    awning_motion_result_.revision++;
  }

  void release_awning_(const char *status) {
    if (awning_phase_ == AwningJogPhase::RELEASE) return;
    awning_stop_status_ = status;
    awning_release_ms_ = millis();
    awning_phase_ = AwningJogPhase::RELEASE;
  }

  bool awning_inputs_safe_() const {
    const auto &state = coach_state();
    const auto &out = state.outputs;
    const uint32_t now = millis();
    return out.valid && state.power.valid && !state.power.ignition_on &&
        now - out.last_seen_ms <= 250 && now - state.power.last_seen_ms <= 500 &&
        !out.sofa_extend && !out.sofa_retract && !out.bedroom_slide_extend &&
        !out.bedroom_slide_retract && !out.wardrobe_slide_extend &&
        !out.wardrobe_slide_retract;
  }

  void finish_awning_jog_(const char *result, const char *status) {
    if (wireless_tp_mute_asserted_) release_wireless_tp_mute_("awning motion finished");
    pid5e_fast_loop_.stop();
    awning_phase_ = AwningJogPhase::IDLE;
    if (awning_max_ms_) publish_awning_motion_(awning_motion_result_.request_id, status);
    awning_max_ms_ = 0;
    ESP_LOGI("LIN_AWNING", "Awning result: %s", result);
  }

  void expire_awning_jog_() {
    const uint32_t now = millis();
    const uint32_t age = now - awning_started_ms_;
    if (awning_phase_ == AwningJogPhase::START && age >= 400)
      finish_awning_jog_("no start slot; no motion command transmitted", "not_sent");
    if (awning_phase_ == AwningJogPhase::HOLD) {
      const uint32_t jog_limit = awning_five_second_test_ ? 5000 : 350;
      if (!awning_max_ms_ && age >= jog_limit) {
        release_awning_("stopped");
        awning_release_ms_ = awning_started_ms_ + jog_limit;
      }
      if (awning_max_ms_ && age >= awning_max_ms_) release_awning_("duration_complete");
      if (awning_max_ms_ && now - awning_renewed_ms_ >= 800) release_awning_("lease_expired");
      if (awning_max_ms_ && !awning_motion_observed_ && age >= 350) release_awning_("no_motion");
      if (!awning_inputs_safe_()) release_awning_("unsafe");
    }
    if (awning_phase_ == AwningJogPhase::RELEASE && now - awning_release_ms_ >= 350) {
      awning_jog_locked_ = true;
      finish_awning_jog_("release slot missed; use factory control and verify stop", "stop_unconfirmed");
    }
    if (awning_phase_ == AwningJogPhase::VERIFY && now - awning_release_ms_ >= 450) {
      awning_jog_locked_ = true;
      finish_awning_jog_("stop not confirmed by PID32; use factory control", "stop_unconfirmed");
    }
  }

  bool send_awning_jog_slot_(esphome::uart::UARTComponent *uart) {
    // Check the deadline again at the actual transmit slot.
    expire_awning_jog_();
    if (awning_phase_ == AwningJogPhase::IDLE || awning_phase_ == AwningJogPhase::VERIFY)
      return false;
    const auto &out = coach_state().outputs;
    const bool opposite = awning_extend_ ? out.awning_retract : out.awning_extend;
    if (awning_phase_ == AwningJogPhase::START &&
        (!awning_inputs_safe_() || out.awning_extend || out.awning_retract)) {
      finish_awning_jog_("unsafe or stale coach state before start; no motion command transmitted", "refused");
      return false;
    }
    if (awning_phase_ == AwningJogPhase::HOLD && (!awning_inputs_safe_() || opposite))
      release_awning_("unsafe");

    const auto phase = awning_phase_;
    uint8_t opcode = awning_extend_ ? 0x0A : 0x09;
    if (phase == AwningJogPhase::HOLD) opcode |= 0x40;
    if (phase == AwningJogPhase::RELEASE) opcode = awning_extend_ ? 0x3F : 0x09;
    const uint8_t response[] = {opcode, 0, classic_checksum_(0x5E, opcode, 0)};
    uart->write_array(response, sizeof(response));
    uart->flush();
    if (phase == AwningJogPhase::START) {
      awning_phase_ = AwningJogPhase::HOLD;
      awning_started_ms_ = millis();
    } else if (phase == AwningJogPhase::HOLD) {
      ++awning_hold_slots_;
      if (!awning_max_ms_ && !awning_five_second_test_ &&
          awning_hold_slots_ >= (awning_extend_ ? 1 : 3)) release_awning_("stopped");
    } else {
      delayMicroseconds(100);
      release_wireless_tp_mute_("awning release sent");
      awning_release_ms_ = millis();
      awning_phase_ = AwningJogPhase::VERIFY;
    }
    if (phase != AwningJogPhase::HOLD || awning_hold_slots_ == 1 ||
        (!awning_max_ms_ && !awning_five_second_test_))
      ESP_LOGI("LIN_AWNING", "TX %02X 00 %02X", opcode, response[2]);
    return true;
  }

  void observe_awning_jog_pid32_() {
    const auto &out = coach_state().outputs;
    if (awning_phase_ == AwningJogPhase::HOLD || awning_phase_ == AwningJogPhase::RELEASE) {
      const bool moving = awning_extend_ ? out.awning_extend : out.awning_retract;
      if (moving && !awning_motion_observed_) {
        awning_motion_observed_ = true;
        if (awning_max_ms_ && awning_phase_ == AwningJogPhase::HOLD)
          publish_awning_motion_(awning_motion_result_.request_id, "moving");
      }
      if (!moving && awning_motion_observed_ && (awning_max_ms_ || awning_five_second_test_))
        release_awning_("motion_ended");
    } else if (awning_phase_ == AwningJogPhase::VERIFY && !out.awning_extend && !out.awning_retract) {
      const bool ended_without_motion = !awning_motion_observed_ &&
          std::strcmp(awning_stop_status_, "duration_complete") == 0;
      finish_awning_jog_(awning_motion_observed_ ? "motion and stop confirmed by PID32"
                                                : "idle confirmed; motion not observed",
                         ended_without_motion ? "no_motion" : awning_stop_status_);
    }
  }

  bool tx_capture_suppress_pid5e_echo_ = false;
  static constexpr uint32_t OUTPUT_FRESH_MS = 1000;
  static constexpr uint32_t OUTPUT_SLOT_TIMEOUT_MS = 3500;
  static constexpr uint32_t OUTPUT_CONFIRM_TIMEOUT_MS = 1500;
  OutputCommandResult output_command_result_;
  std::string output_command_request_;
  uint8_t output_command_opcode_ = 0;
  bool output_command_desired_ = false;
  bool output_command_sent_ = false;
  uint32_t output_command_started_ms_ = 0;

  bool output_for_opcode_(uint8_t opcode) const {
    const auto &outputs = coach_state().outputs;
    if (opcode == 0x04) return outputs.water_heater;
    if (opcode == 0x07) return outputs.water_pump;
    return outputs.awning_light;
  }

  void publish_output_command_(const std::string &request_id, const char *status) {
    output_command_result_.request_id = request_id;
    output_command_result_.status = status;
    output_command_result_.revision++;
    ESP_LOGI("LIN_TX", "Desired output request %s: %s", request_id.c_str(), status);
  }

  void finish_output_command_(const char *status) {
    publish_output_command_(output_command_request_, status);
    output_command_request_.clear();
  }

  void discard_unsent_output_command_(const char *status) {
    pid5e_commands_.pop_front();
    if (wireless_tp_mute_asserted_) release_wireless_tp_mute_("desired output canceled");
    pid5e_fast_loop_.stop();
    finish_output_command_(status);
  }

  void expire_output_command_() {
    if (output_command_request_.empty()) return;
    const uint32_t age = millis() - output_command_started_ms_;
    if (!output_command_sent_ && age >= OUTPUT_SLOT_TIMEOUT_MS) {
      discard_unsent_output_command_("not_sent");
    } else if (output_command_sent_ && age >= OUTPUT_CONFIRM_TIMEOUT_MS) {
      finish_output_command_("unconfirmed");
    }
  }

  void arm_pid5e_command_() {
    pid5e_phase_started_ms_ = millis();
    pid5e_fast_loop_.start();
    assert_wireless_tp_mute_("queued PID5E command claim");
    auto &state = coach_state();
    state.lin_tx_enabled = true;
    state.lin_tx_last_result = "queued; waiting for a fresh PID5E header";
    ESP_LOGW("LIN_TX", "Queued %s; waiting for fresh PID5E header; depth=%u",
             pid5e_commands_.front().label.c_str(), (unsigned) pid5e_commands_.size());
  }

  void expire_pid5e_command_() {
    if (awning_phase_ != AwningJogPhase::IDLE ||
        (!wireless_tp_mute_asserted_ && !pid5e_waiting_for_idle_) ||
        millis() - pid5e_phase_started_ms_ < 3000) return;

    if (wireless_tp_mute_asserted_) release_wireless_tp_mute_("PID5E header timeout");
    pid5e_fast_loop_.stop();
    pid5e_waiting_for_idle_ = false;
    ESP_LOGW("LIN_TX", "PID5E header or idle timeout; discarded %u queued command(s)",
             (unsigned) pid5e_commands_.size());
    pid5e_commands_.clear();
    if (!output_command_request_.empty() && !output_command_sent_) {
      finish_output_command_("not_sent");
    }
    coach_state().lin_tx_last_result = "PID5E header or idle timeout";
  }

  // Raw byte-stream header detector for the low-latency PID5E response test.
  uint8_t stream_prev2_ = 0xFF;
  uint8_t stream_prev1_ = 0xFF;

  bool tx_capture_active_ = false;
  uint32_t tx_capture_started_ms_ = 0;
  uint8_t tx_capture_expected_[3] = {0, 0, 0};
  std::vector<uint8_t> tx_capture_bytes_;

  // Build 010.1: raw PID1F response timing sniff.
  // This runs before the frame parser so we can see how quickly the touchscreen
  // answers after the PMM publishes BREAK/SYNC/PID 1F.
  uint8_t raw_window_[3] = {0, 0, 0};
  bool pid1f_timing_active_ = false;
  uint32_t pid1f_timing_start_us_ = 0;
  uint32_t pid1f_timing_last_us_ = 0;
  std::vector<uint8_t> pid1f_timing_bytes_;
  std::vector<uint32_t> pid1f_timing_offsets_;

  bool pending_pid1f_ = false;
  uint32_t pending_pid1f_ms_ = 0;
  uint8_t pending_pid1f_b0_ = 0;
  uint8_t pending_pid1f_b1_ = 0;
  std::string pending_pid1f_raw_;
  std::string pending_pid1f_label_;
  OutputState pending_outputs_before_;
  HVACZoneState pending_hvac_z1_before_;
  HVACZoneState pending_hvac_z2_before_;
  CoachTelemetryState pending_telemetry_before_;
  bool pending_pid32_resolved_ = false;
  bool pending_pid37_resolved_ = false;
  bool pending_pidba_resolved_ = false;
  std::vector<std::string> pid1f_history_;

  bool pending_pid5e_ = false;
  uint32_t pending_pid5e_ms_ = 0;
  uint8_t pending_pid5e_b0_ = 0;
  uint8_t pending_pid5e_b1_ = 0;
  std::string pending_pid5e_raw_;
  std::string pending_pid5e_label_;
  OutputState pending_pid5e_outputs_before_;
  HVACZoneState pending_pid5e_hvac_z1_before_;
  HVACZoneState pending_pid5e_hvac_z2_before_;
  CoachTelemetryState pending_pid5e_telemetry_before_;
  bool pending_pid5e_pid32_resolved_ = false;
  bool pending_pid5e_pid37_resolved_ = false;
  bool pending_pid5e_pidba_resolved_ = false;
  std::vector<std::string> pid5e_history_;

  // Build 010.3: all-PID flight recorder. Manual trigger freezes a compact
  // frame list so HA/BLE/mobile actions can be diffed against idle traffic.
  bool flight_recorder_active_ = false;
  uint32_t flight_recorder_start_ms_ = 0;
  uint32_t flight_recorder_duration_ms_ = 10000;
  uint32_t flight_recorder_frame_count_ = 0;
  std::string flight_recorder_label_;
  struct FlightFrame {
    uint32_t age_ms;
    Frame frame;
  };
  static constexpr size_t FLIGHT_FRAME_LIMIT = 1200;
  std::vector<FlightFrame> flight_recorder_frames_;
  size_t flight_dump_index_ = 0;
  uint32_t last_flight_dump_ms_ = 0;

  uint8_t classic_checksum_(uint8_t pid, uint8_t b0, uint8_t b1) const {
    uint16_t sum = pid + b0 + b1;
    while (sum > 0xFF) sum = (sum & 0xFF) + (sum >> 8);
    return (uint8_t)(0xFF - sum);
  }


  std::string bytes_to_hex_(const std::vector<uint8_t> &bytes) const {
    char tmp[4];
    std::string out;
    for (uint8_t b : bytes) {
      snprintf(tmp, sizeof(tmp), "%02X", b);
      if (!out.empty()) out += " ";
      out += tmp;
    }
    return out;
  }

  bool capture_contains_expected_() const {
    if (tx_capture_bytes_.size() < 3) return false;
    for (size_t i = 0; i + 2 < tx_capture_bytes_.size(); i++) {
      if (tx_capture_bytes_[i] == tx_capture_expected_[0] &&
          tx_capture_bytes_[i + 1] == tx_capture_expected_[1] &&
          tx_capture_bytes_[i + 2] == tx_capture_expected_[2]) {
        return true;
      }
    }
    return false;
  }


  void publish_pid1f_response_timing_(const char *reason) {
    CoachState &state = coach_state();
    char result[320];
    std::string bytes = bytes_to_hex_(pid1f_timing_bytes_);
    if (bytes.empty()) bytes = "<none>";

    std::string offsets;
    char tmp[32];
    for (size_t i = 0; i < pid1f_timing_offsets_.size(); i++) {
      snprintf(tmp, sizeof(tmp), "%luus", (unsigned long) pid1f_timing_offsets_[i]);
      if (!offsets.empty()) offsets += ", ";
      offsets += tmp;
    }
    if (offsets.empty()) offsets = "<none>";

    snprintf(result, sizeof(result), "%s; after 00 55 1F bytes=%s; offsets=%s",
             reason, bytes.c_str(), offsets.c_str());
    state.pid1f_response_timing = result;
    ESP_LOGW("LIN_TIMING", "%s", result);
    pid1f_timing_active_ = false;
  }

  void sniff_pid1f_response_timing_(uint8_t b) {
    const uint32_t now = micros();

    if (pid1f_timing_active_) {
      pid1f_timing_bytes_.push_back(b);
      pid1f_timing_offsets_.push_back(now - pid1f_timing_start_us_);
      pid1f_timing_last_us_ = now;
      if (pid1f_timing_bytes_.size() >= 9) {
        publish_pid1f_response_timing_("pid1f_timing_capture_full_9_bytes");
      }
      return;
    }

    raw_window_[0] = raw_window_[1];
    raw_window_[1] = raw_window_[2];
    raw_window_[2] = b;

    if (raw_window_[0] == 0x00 && raw_window_[1] == 0x55 && raw_window_[2] == 0x1F) {
      pid1f_timing_active_ = true;
      pid1f_timing_start_us_ = now;
      pid1f_timing_last_us_ = now;
      pid1f_timing_bytes_.clear();
      pid1f_timing_offsets_.clear();
    }
  }

  void expire_pid1f_response_timing_if_needed_() {
    if (!pid1f_timing_active_) return;
    if ((uint32_t)(micros() - pid1f_timing_last_us_) > 12000) {
      publish_pid1f_response_timing_("pid1f_timing_timeout_12ms");
    }
  }

  void publish_post_slot_capture_(const char *reason) {
    CoachState &state = coach_state();
    const bool echo = capture_contains_expected_();
    char result[260];
    std::string capture = bytes_to_hex_(tx_capture_bytes_);
    if (capture.empty()) capture = "<no RX bytes captured>";
    snprintf(result, sizeof(result), "%s; echo=%s; bytes=%s", reason, echo ? "yes" : "no", capture.c_str());
    state.lin_tx_post_slot_capture = result;
    state.lin_tx_echo_seen = echo;
    if (!echo && std::string(reason).find("bad_frame") != std::string::npos) state.lin_tx_collision_suspected = true;
    ESP_LOGW("LIN_TX", "Post-slot capture: %s", result);
    tx_capture_active_ = false;
  }

  void begin_post_slot_capture_(uint8_t b0, uint8_t b1, uint8_t checksum) {
    CoachState &state = coach_state();
    tx_capture_active_ = true;
    tx_capture_suppress_pid5e_echo_ = false;
    tx_capture_started_ms_ = millis();
    tx_capture_expected_[0] = b0;
    tx_capture_expected_[1] = b1;
    tx_capture_expected_[2] = checksum;
    tx_capture_bytes_.clear();
    state.lin_tx_post_slot_capture = "capture active; waiting for RX echo/post-slot bytes";
    state.lin_tx_echo_seen = false;
    state.lin_tx_collision_suspected = false;
  }

  void capture_post_slot_byte_(uint8_t b) {
    if (!tx_capture_active_) return;
    tx_capture_bytes_.push_back(b);
    if (tx_capture_bytes_.size() >= 48) {
      publish_post_slot_capture_("capture_full_48_bytes");
      return;
    }
    if (millis() - tx_capture_started_ms_ > 100) {
      publish_post_slot_capture_("capture_timeout_100ms");
      return;
    }
  }

  bool maybe_send_pending_pid5e_fast_response_(esphome::uart::UARTComponent *uart, uint8_t current_byte) {
    const bool header_seen = (stream_prev2_ == 0x00 && stream_prev1_ == 0x55 && current_byte == 0x5E);

    // Always advance the raw stream detector, even when no command is queued.
    stream_prev2_ = stream_prev1_;
    stream_prev1_ = current_byte;

    if (!wireless_tp_mute_asserted_ || !header_seen || uart->available()) return false;
    if (awning_phase_ != AwningJogPhase::IDLE) return send_awning_jog_slot_(uart);
    if (pid5e_commands_.empty()) return false;

    const bool desired_output = !output_command_request_.empty() && !output_command_sent_;
    if (desired_output) {
      if (millis() - coach_state().outputs.last_seen_ms > OUTPUT_FRESH_MS) {
        discard_unsent_output_command_("not_sent");
        return false;
      }
      if (output_for_opcode_(output_command_opcode_) == output_command_desired_) {
        discard_unsent_output_command_("confirmed");
        return false;
      }
    }

    auto &command = pid5e_commands_.front();
    const uint8_t checksum = classic_checksum_(0x5E, command.b0, command.b1);
    const uint8_t response[] = {command.b0, command.b1, checksum};

    if (command.delay_us > 0) delayMicroseconds(command.delay_us);
    uart->write_array(response, sizeof(response));
    uart->flush();
    delayMicroseconds(100);
    release_wireless_tp_mute_("PID5E fast TX complete");
    pid5e_fast_loop_.stop();

    auto &state = coach_state();
    const bool awning_toggle = command.b0 == 0x00 && command.b1 == 0x00;
    begin_post_slot_capture_(command.b0, command.b1, checksum);
    tx_capture_suppress_pid5e_echo_ = awning_toggle || desired_output;
    if (!awning_toggle && !desired_output) command_intent_.observe_pid5e(command.b0, command.b1);
    if (desired_output) {
      output_command_sent_ = true;
      output_command_started_ms_ = millis();
    }
    state.lin_tx_count++;
    char attempt[160];
    snprintf(attempt, sizeof(attempt), "%s / PID5E TX %02X %02X %02X",
             command.label.c_str(), command.b0, command.b1, checksum);
    state.lin_tx_last_attempt = attempt;
    state.lin_tx_last_result = "wrote PID5E response; waiting for bus feedback";
    ESP_LOGW("LIN_TX", "PID5E TX #%lu: %s", (unsigned long) state.lin_tx_count, attempt);

    pid5e_commands_.pop_front();
    // Give the factory module a response slot between injected toggles.
    pid5e_waiting_for_idle_ = true;
    pid5e_phase_started_ms_ = millis();
    return true;
  }

  void maybe_send_pending_pid1f_slot_response_(esphome::uart::UARTComponent *uart, uint8_t pid) {
    if (!pending_slot_tx_) return;

    CoachState &state = coach_state();
    const uint32_t age = millis() - pending_slot_queued_ms_;
    if (age > 3000) {
      char result[160];
      snprintf(result, sizeof(result), "slot TX expired; no PID1F header seen within %lums", (unsigned long) age);
      state.lin_tx_last_result = result;
      pending_slot_tx_ = false;
      ESP_LOGW("LIN_TX", "%s", result);
      return;
    }

    if (pid != 0x1F) return;

    const uint8_t checksum = classic_checksum_(0x1F, pending_slot_b0_, pending_slot_b1_);
    const uint8_t response[] = {pending_slot_b0_, pending_slot_b1_, checksum};

    state.lin_tx_count++;
    char attempt[180];
    snprintf(attempt, sizeof(attempt), "%s / PID1F slot response %02X %02X %02X delay=%uus",
             pending_slot_label_.c_str(), pending_slot_b0_, pending_slot_b1_, checksum, (unsigned) pending_slot_delay_us_);
    state.lin_tx_last_attempt = attempt;
    char result[160];
    snprintf(result, sizeof(result), "slot_header_seen; delay=%uus; wrote PID1F payload/checksum; capturing post-slot RX", (unsigned) pending_slot_delay_us_);
    state.lin_tx_last_result = result;
    begin_post_slot_capture_(pending_slot_b0_, pending_slot_b1_, checksum);

    // Respond in the PID1F response slot only. The configurable delay lets us
    // sweep timing to learn whether the factory touchscreen is winning or
    // colliding in this response slot.
    if (pending_slot_delay_us_ > 0) delayMicroseconds(pending_slot_delay_us_);
    uart->write_array(response, sizeof(response));
    uart->flush();
    command_intent_.observe_pid1f(pending_slot_b0_, pending_slot_b1_);

    ESP_LOGW("LIN_TX", "PID1F slot response #%lu: %s", (unsigned long) state.lin_tx_count, attempt);
    pending_slot_tx_ = false;
  }


  void record_flight_frame_(const Frame &f) {
    if (!flight_recorder_active_) return;
    const uint32_t age = millis() - flight_recorder_start_ms_;
    if (age > flight_recorder_duration_ms_) {
      finish_flight_recorder_("duration complete");
      return;
    }
    flight_recorder_frame_count_++;
    if (flight_recorder_frames_.size() < FLIGHT_FRAME_LIMIT) {
      flight_recorder_frames_.push_back({age, f});
    }
  }

  void finish_flight_recorder_(const char *reason) {
    if (!flight_recorder_active_) return;
    flight_recorder_active_ = false;
    CoachState &state = coach_state();

    char status[180];
    snprintf(status, sizeof(status), "complete; %s; %s; frames_seen=%lu; retained=%u",
             flight_recorder_label_.c_str(), reason ? reason : "done",
             (unsigned long) flight_recorder_frame_count_, (unsigned) flight_recorder_frames_.size());
    state.flight_recorder_status = status;

    state.flight_recorder_capture = status;
    ESP_LOGW("LIN_FLIGHT", "%s", status);
  }

  void dump_flight_chunk_() {
    if (flight_recorder_active_ || awning_phase_ != AwningJogPhase::IDLE || wireless_tp_mute_asserted_ ||
        flight_dump_index_ >= flight_recorder_frames_.size()) return;
    const uint32_t now = millis();
    if (now - last_flight_dump_ms_ < 20) return;
    last_flight_dump_ms_ = now;

    // Pace output so a completed capture does not block LIN reception.
    std::string chunk;
    char line[80];
    for (size_t i = 0; i < 5 && flight_dump_index_ < flight_recorder_frames_.size(); i++) {
      const auto &item = flight_recorder_frames_[flight_dump_index_++];
      snprintf(line, sizeof(line), "+%lums PID%02X %s",
               (unsigned long) item.age_ms, item.frame.pid, raw_string(item.frame).c_str());
      if (!chunk.empty()) chunk += '\n';
      chunk += line;
    }
    ESP_LOGW("LIN_FLIGHT", "%s", chunk.c_str());
    if (flight_dump_index_ == flight_recorder_frames_.size()) {
      ESP_LOGW("LIN_FLIGHT", "capture dump complete; %u frames",
               (unsigned) flight_recorder_frames_.size());
      flight_recorder_frames_.clear();
      flight_dump_index_ = 0;
    }
  }

  void expire_flight_recorder_if_needed_() {
    if (!flight_recorder_active_) return;
    if (millis() - flight_recorder_start_ms_ > flight_recorder_duration_ms_ + 50) {
      finish_flight_recorder_("timer expired");
    }
  }

  void handle_frame(const Frame &f) {
    CoachState &state = coach_state();
    const std::string raw = raw_string(f);
    state.observe_valid_frame(f.pid, raw);
    record_flight_frame_(f);
    if (f.pid == 0x1F) state.pid1f_last_command = raw;
    else if (f.pid == 0x5E) state.pid5e_last_frame = raw;
    else if (f.pid == 0x32) state.pid32_last_frame = raw;
    else if (f.pid == 0x37) state.pid37_last_frame = raw;
    else if (f.pid == 0x76) state.pid76_last_frame = raw;
    else if (f.pid == 0x9C) state.pid9c_last_frame = raw;
    else if (f.pid == 0xBA) state.pidba_last_frame = raw;
    else if (f.pid == 0xDD) state.piddd_last_frame = raw;
    else if (f.pid == 0xEC) state.pidec_last_frame = raw;

    count_frame(f);
    maybe_report_counts();

    if (f.pid == 0x1F) remember_pid1f_command_(f);
    if (f.pid == 0x5E) remember_pid5e_command_(f);
    if (f.pid == 0x1F && f.len >= 6) command_intent_.observe_pid1f(f.bytes[3], f.bytes[4]);
    if (f.pid == 0x5E && f.len >= 6) {
      const bool local_echo = tx_capture_suppress_pid5e_echo_ &&
          millis() - tx_capture_started_ms_ <= 100 &&
          f.bytes[3] == tx_capture_expected_[0] && f.bytes[4] == tx_capture_expected_[1];
      if (!local_echo) command_intent_.observe_pid5e(f.bytes[3], f.bytes[4]);
      if (pid5e_waiting_for_idle_ && f.bytes[3] == 0x3F && f.bytes[4] == 0x00) {
        pid5e_waiting_for_idle_ = false;
        if (!pid5e_commands_.empty()) arm_pid5e_command_();
      }
    }

    switch (f.pid) {
      case 0x32:
        pid32_.decode(f);
        observe_awning_jog_pid32_();
        if (output_command_sent_ && !output_command_request_.empty() &&
            output_for_opcode_(output_command_opcode_) == output_command_desired_) {
          finish_output_command_("confirmed");
        }
        correlate_pid32_after_command_();
        correlate_pid32_after_pid5e_command_();
        break;
      case 0x37:
        pid37_.decode(f);
        correlate_pid37_after_command_(f);
        correlate_pid37_after_pid5e_command_(f);
        break;
      case 0xBA:
        ba_.decode(f);
        correlate_pidba_after_command_();
        correlate_pidba_after_pid5e_command_();
        break;
      case 0x9C: scheduler_.decode_9c(f); break;
      case 0xDD: scheduler_.decode_dd(f); break;
      case 0xEC: ec_.decode(f); break;
      case 0x1F: pid1f_.decode(f); break;
      case 0x5E: scheduler_.decode_5e(f); break;
      case 0x76: break;
      default: break;
    }

    expire_pending_pid1f_if_needed_();
    expire_pending_pid5e_if_needed_();
  }

  bool is_pid1f_idle_(const Frame &f) const {
    if (f.pid != 0x1F || f.len < 6) return true;
    const uint8_t b0 = f.bytes[3];
    const uint8_t b1 = f.bytes[4];
    return (b0 == 0x3F && b1 == 0x00);
  }

  void remember_pid1f_command_(const Frame &f) {
    if (f.pid != 0x1F || f.len < 6) return;
    if (is_pid1f_idle_(f)) return;

    CoachState &state = coach_state();
    const uint8_t b0 = f.bytes[3];
    const uint8_t b1 = f.bytes[4];
    const char *label = PID1FDecoder::label_for_payload(b0, b1);

    pending_pid1f_ = true;
    pending_pid1f_ms_ = millis();
    pending_pid1f_b0_ = b0;
    pending_pid1f_b1_ = b1;
    pending_pid1f_raw_ = raw_string(f);
    pending_pid1f_label_ = label;
    pending_outputs_before_ = state.outputs;
    pending_hvac_z1_before_ = state.hvac_zone_1;
    pending_hvac_z2_before_ = state.hvac_zone_2;
    pending_telemetry_before_ = state.telemetry;
    pending_pid32_resolved_ = false;
    pending_pid37_resolved_ = false;
    pending_pidba_resolved_ = false;

    char buf[240];
    snprintf(buf, sizeof(buf), "%02X %02X | %s | awaiting PID32/PID37/PIDBA", b0, b1, label);
    state.pid1f_last_non_idle_command = buf;
    state.pid1f_last_learned_command = buf;
    push_pid1f_history_(buf);
    ESP_LOGI("PID1F_LEARN", "%s | raw=%s", buf, pending_pid1f_raw_.c_str());
  }

  void correlate_pid32_after_command_() {
    if (!pending_pid1f_ || pending_pid32_resolved_) return;
    CoachState &state = coach_state();
    if (!state.outputs.valid) return;

    const uint32_t age = millis() - pending_pid1f_ms_;
    if (age > 1500) return;

    const std::string diff = describe_output_diff(pending_outputs_before_, state.outputs);
    if (diff == "no PID32 output change observed") return;

    char learned[320];
    snprintf(learned, sizeof(learned), "%02X %02X | %s | PID32: %s | %lums",
             pending_pid1f_b0_, pending_pid1f_b1_, pending_pid1f_label_.c_str(), diff.c_str(), (unsigned long) age);
    state.pid1f_last_learned_command = learned;
    push_pid1f_history_(learned);
    ESP_LOGI("PID1F_LEARN", "%s", learned);
    pending_pid32_resolved_ = true;
    pending_pid1f_ = false;
  }

  void correlate_pid37_after_command_(const Frame &f) {
    if (!pending_pid1f_ || pending_pid37_resolved_) return;
    if (f.pid != 0x37 || f.len < 12) return;

    CoachState &state = coach_state();
    const uint8_t zone = f.bytes[3];
    const uint32_t age = millis() - pending_pid1f_ms_;
    if (age > 1500) return;

    std::string diff;
    if (zone == 1) diff = describe_hvac_diff(pending_hvac_z1_before_, state.hvac_zone_1);
    else if (zone == 2) diff = describe_hvac_diff(pending_hvac_z2_before_, state.hvac_zone_2);
    else return;

    if (diff == "no PID37 HVAC change observed") return;

    char learned[360];
    snprintf(learned, sizeof(learned), "%02X %02X | %s | PID37: %s | %lums",
             pending_pid1f_b0_, pending_pid1f_b1_, pending_pid1f_label_.c_str(), diff.c_str(), (unsigned long) age);
    state.pid1f_last_learned_command = learned;
    push_pid1f_history_(learned);
    ESP_LOGI("PID1F_LEARN", "%s", learned);
    pending_pid37_resolved_ = true;

    // Keep the pending command open briefly for a possible PID32 change too, but
    // do not let it linger and get overwritten by normal cyclic HVAC activity.
    if (age > 250 || pending_pid32_resolved_) pending_pid1f_ = false;
  }


  void correlate_pidba_after_command_() {
    if (!pending_pid1f_ || pending_pidba_resolved_) return;
    CoachState &state = coach_state();
    if (!state.telemetry.valid) return;

    const uint32_t age = millis() - pending_pid1f_ms_;
    if (age > 10000) return;

    const std::string diff = describe_generator_diff(pending_telemetry_before_, state.telemetry);
    if (diff == "no PIDBA generator change observed") return;

    char learned[360];
    snprintf(learned, sizeof(learned), "%02X %02X | %s | PIDBA: %s | %lums",
             pending_pid1f_b0_, pending_pid1f_b1_, pending_pid1f_label_.c_str(), diff.c_str(), (unsigned long) age);
    state.pid1f_last_learned_command = learned;
    push_pid1f_history_(learned);
    ESP_LOGI("PID1F_LEARN", "%s", learned);
    pending_pidba_resolved_ = true;
    pending_pid1f_ = false;
  }

  void expire_pending_pid1f_if_needed_() {
    if (!pending_pid1f_) return;
    const uint32_t age = millis() - pending_pid1f_ms_;
    if (age <= 10000) return;

    CoachState &state = coach_state();
    char timeout_buf[180];
    snprintf(timeout_buf, sizeof(timeout_buf), "%02X %02X | no downstream change",
             pending_pid1f_b0_, pending_pid1f_b1_);
    state.pid1f_last_learned_command = timeout_buf;
    // Discovery timeout spam is intentionally not logged or added to history.
    pending_pid1f_ = false;
  }

  std::string compact_state_line_(const std::string &line, size_t max_len = 240) const {
    std::string out = line;
    for (char &c : out) {
      if (c == '\n' || c == '\r') c = ' ';
    }
    if (out.size() > max_len) {
      out.resize(max_len - 3);
      out += "...";
    }
    return out;
  }

  void push_pid1f_history_(const std::string &line) {
    CoachState &state = coach_state();
    if (line.find("awaiting") != std::string::npos) {
      // Keep transient awaiting states out of the history; they still show in
      // Last Non-Idle Command.
      return;
    }
    // Home Assistant states are limited to 255 characters. Keep this text
    // sensor as a compact last-interesting-result indicator, not a rolling log.
    state.pid1f_command_history = compact_state_line_(line);
  }



  bool is_pid5e_idle_(const Frame &f) const {
    if (f.pid != 0x5E || f.len < 6) return true;
    const uint8_t b0 = f.bytes[3];
    const uint8_t b1 = f.bytes[4];
    return (b0 == 0x3F && b1 == 0x00);
  }

  bool is_pid5e_housekeeping_(uint8_t b0, uint8_t b1) const {
    return (b0 == 0xB0 && b1 == 0x02) || (b0 == 0xB1 && b1 == 0x01);
  }

  const char *label_pid5e_payload_(uint8_t b0, uint8_t b1) const {
    if (b0 == 0x3F && b1 == 0x00) return "idle / bridge ready";
    if (b0 == 0xB0 && b1 == 0x02) return "wireless tp housekeeping pulse B0 02";
    if (b0 == 0xB1 && b1 == 0x01) return "wireless tp housekeeping pulse B1 01";
    if (b1 == 0x00) return "wireless tp candidate command / status";
    return "unmapped wireless tp bridge payload";
  }

  void remember_pid5e_command_(const Frame &f) {
    if (f.pid != 0x5E || f.len < 6) return;
    if (is_pid5e_idle_(f)) return;

    CoachState &state = coach_state();
    const uint8_t b0 = f.bytes[3];
    const uint8_t b1 = f.bytes[4];
    const char *label = label_pid5e_payload_(b0, b1);

    // B0 02 and B1 01 are now confirmed Wireless TP housekeeping pulses.
    // They are useful to recognize, but they are not commands and should not
    // start 10-second correlation windows or produce "no change" log spam.
    if (is_pid5e_housekeeping_(b0, b1)) {
      char hb[220];
      snprintf(hb, sizeof(hb), "%02X %02X | %s", b0, b1, label);
      state.pid5e_last_non_idle_command = hb;
      return;
    }

    pending_pid5e_ = true;
    pending_pid5e_ms_ = millis();
    pending_pid5e_b0_ = b0;
    pending_pid5e_b1_ = b1;
    pending_pid5e_raw_ = raw_string(f);
    pending_pid5e_label_ = label;
    pending_pid5e_outputs_before_ = state.outputs;
    pending_pid5e_hvac_z1_before_ = state.hvac_zone_1;
    pending_pid5e_hvac_z2_before_ = state.hvac_zone_2;
    pending_pid5e_telemetry_before_ = state.telemetry;
    pending_pid5e_pid32_resolved_ = false;
    pending_pid5e_pid37_resolved_ = false;
    pending_pid5e_pidba_resolved_ = false;

    char buf[260];
    snprintf(buf, sizeof(buf), "%02X %02X | %s | awaiting PID32/PID37/PIDBA", b0, b1, label);
    state.pid5e_last_non_idle_command = buf;
    state.pid5e_last_learned_command = buf;
    push_pid5e_history_(buf);
    ESP_LOGI("PID5E_LEARN", "%s | raw=%s", buf, pending_pid5e_raw_.c_str());
  }

  void correlate_pid32_after_pid5e_command_() {
    if (!pending_pid5e_ || pending_pid5e_pid32_resolved_) return;
    CoachState &state = coach_state();
    if (!state.outputs.valid) return;

    const uint32_t age = millis() - pending_pid5e_ms_;
    if (age > 1500) return;

    const std::string diff = describe_output_diff(pending_pid5e_outputs_before_, state.outputs);
    if (diff == "no PID32 output change observed") return;

    char learned[360];
    snprintf(learned, sizeof(learned), "%02X %02X | %s | PID32: %s | %lums",
             pending_pid5e_b0_, pending_pid5e_b1_, pending_pid5e_label_.c_str(), diff.c_str(), (unsigned long) age);
    state.pid5e_last_learned_command = learned;
    push_pid5e_history_(learned);
    ESP_LOGI("PID5E_LEARN", "%s", learned);
    pending_pid5e_pid32_resolved_ = true;
    pending_pid5e_ = false;
  }

  void correlate_pid37_after_pid5e_command_(const Frame &f) {
    if (!pending_pid5e_ || pending_pid5e_pid37_resolved_) return;
    if (f.pid != 0x37 || f.len < 12) return;

    CoachState &state = coach_state();
    const uint8_t zone = f.bytes[3];
    const uint32_t age = millis() - pending_pid5e_ms_;
    if (age > 1500) return;

    std::string diff;
    if (zone == 1) diff = describe_hvac_diff(pending_pid5e_hvac_z1_before_, state.hvac_zone_1);
    else if (zone == 2) diff = describe_hvac_diff(pending_pid5e_hvac_z2_before_, state.hvac_zone_2);
    else return;

    if (diff == "no PID37 HVAC change observed") return;

    char learned[380];
    snprintf(learned, sizeof(learned), "%02X %02X | %s | PID37: %s | %lums",
             pending_pid5e_b0_, pending_pid5e_b1_, pending_pid5e_label_.c_str(), diff.c_str(), (unsigned long) age);
    state.pid5e_last_learned_command = learned;
    push_pid5e_history_(learned);
    ESP_LOGI("PID5E_LEARN", "%s", learned);
    pending_pid5e_pid37_resolved_ = true;
    if (age > 250 || pending_pid5e_pid32_resolved_) pending_pid5e_ = false;
  }

  void correlate_pidba_after_pid5e_command_() {
    if (!pending_pid5e_ || pending_pid5e_pidba_resolved_) return;
    CoachState &state = coach_state();
    if (!state.telemetry.valid) return;

    const uint32_t age = millis() - pending_pid5e_ms_;
    if (age > 10000) return;

    const std::string diff = describe_generator_diff(pending_pid5e_telemetry_before_, state.telemetry);
    if (diff == "no PIDBA generator change observed") return;

    char learned[380];
    snprintf(learned, sizeof(learned), "%02X %02X | %s | PIDBA: %s | %lums",
             pending_pid5e_b0_, pending_pid5e_b1_, pending_pid5e_label_.c_str(), diff.c_str(), (unsigned long) age);
    state.pid5e_last_learned_command = learned;
    push_pid5e_history_(learned);
    ESP_LOGI("PID5E_LEARN", "%s", learned);
    pending_pid5e_pidba_resolved_ = true;
    pending_pid5e_ = false;
  }

  void expire_pending_pid5e_if_needed_() {
    if (!pending_pid5e_) return;
    const uint32_t age = millis() - pending_pid5e_ms_;
    if (age <= 10000) return;

    CoachState &state = coach_state();
    char timeout_buf[180];
    snprintf(timeout_buf, sizeof(timeout_buf), "%02X %02X | no downstream change",
             pending_pid5e_b0_, pending_pid5e_b1_);
    state.pid5e_last_learned_command = timeout_buf;
    // Discovery timeout spam is intentionally not logged or added to history.
    pending_pid5e_ = false;
  }

  void push_pid5e_history_(const std::string &line) {
    CoachState &state = coach_state();
    if (line.find("awaiting") != std::string::npos) return;
    // Home Assistant states are limited to 255 characters. Keep this text
    // sensor as a compact last-interesting-result indicator, not a rolling log.
    state.pid5e_command_history = compact_state_line_(line);
  }


  std::string buffer_tail_string_(size_t count) {
    std::string out;
    char part[8];
    size_t start = 0;
    if (buf_.size() > count) start = buf_.size() - count;
    for (size_t i = start; i < buf_.size(); i++) {
      snprintf(part, sizeof(part), "%02X ", buf_[i]);
      out += part;
    }
    return out;
  }

  void count_frame(const Frame &f) {
    if (f.pid == 0xBA) count_ba_++;
    else if (f.pid == 0x76) count_76_++;
    else if (f.pid == 0x32) count_32_++;
    else if (f.pid == 0x37 && f.bytes[3] == 1) count_37_z1_++;
    else if (f.pid == 0x37 && f.bytes[3] == 2) count_37_z2_++;
    else if (f.pid == 0x9C) count_9c_++;
    else if (f.pid == 0xDD) count_dd_++;
    else if (f.pid == 0xEC) count_ec_++;
    else if (f.pid == 0x1F) count_1f_++;
    else if (f.pid == 0x5E) count_5e_++;
  }

  void maybe_report_counts() {
    uint32_t now = millis();
    if (last_report_ms_ == 0) last_report_ms_ = now;
    if (now - last_report_ms_ < 10000) return;

    ESP_LOGD(
      "LIN_COUNTS",
      "10s: BA=%u 76=%u 32=%u 37Z1=%u 37Z2=%u 9C=%u DD=%u EC=%u 1F=%u 5E=%u BAD=%u UNKNOWN=%u",
      count_ba_, count_76_, count_32_, count_37_z1_, count_37_z2_,
      count_9c_, count_dd_, count_ec_, count_1f_, count_5e_, count_bad_, count_unknown_
    );

    count_ba_ = count_76_ = count_32_ = count_37_z1_ = count_37_z2_ = 0;
    count_9c_ = count_dd_ = count_ec_ = count_1f_ = count_5e_ = 0;
    count_bad_ = count_unknown_ = 0;
    last_report_ms_ = now;
  }
};

inline Analyzer &global_analyzer() {
  static Analyzer analyzer;
  return analyzer;
}

}  // namespace precision_plex
