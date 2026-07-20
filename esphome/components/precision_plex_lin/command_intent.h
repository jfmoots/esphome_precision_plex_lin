#pragma once

#include "coach_state.h"
#include "esphome/core/log.h"

namespace precision_plex {

// Converts the two fast command channels into one edge-driven event stream.
// PID32 remains the authoritative output bitmap; these events only provide the
// requested state while waiting for the next slow PID32 rotation.
class CommandIntentObserver {
 public:
  void observe_pid1f(uint8_t opcode, uint8_t argument) {
    observe_(pid1f_, "pid1f", opcode, argument, false);
  }

  void observe_pid5e(uint8_t opcode, uint8_t argument) {
    observe_(pid5e_, "pid5e", opcode, argument, true);
  }

 private:
  struct ChannelState {
    bool have_last = false;
    uint8_t last_opcode = 0;
    uint8_t last_argument = 0;
    std::string active_toggle_key;
    std::string active_motion_key;
  };

  ChannelState pid1f_;
  ChannelState pid5e_;

  static const char *toggle_key_(uint8_t opcode, uint8_t argument) {
    if (argument != 0x00) return nullptr;
    if (opcode == 0x00) return "awning_light";
    if (opcode == 0x04) return "water_heater";
    if (opcode == 0x06) return "tank_heater";
    if (opcode == 0x07) return "water_pump";
    return nullptr;
  }

  static const char *motion_key_(uint8_t opcode, uint8_t argument) {
    if (argument != 0x00) return nullptr;
    switch (opcode & 0xBF) {  // request 0x0x and accepted/active 0x4x are one intent
      case 0x09: return "awning_in";
      case 0x0A: return "awning_out";
      case 0x0F: return "sofa_slide_in";
      case 0x10: return "sofa_slide_out";
      case 0x11: return "wardrobe_slide_in";
      case 0x12: return "wardrobe_slide_out";
      case 0x13: return "bed_slide_in";
      case 0x14: return "bed_slide_out";
      default: return nullptr;
    }
  }

  static void emit_(const char *source, const char *key, const char *action,
                    const char *phase, uint8_t opcode, uint8_t argument) {
    CommandIntentState &intent = coach_state().command_intent;
    intent.sequence++;
    intent.revision++;
    intent.last_seen_ms = millis();
    intent.source = source;
    intent.key = key;
    intent.action = action;
    intent.phase = phase;
    intent.opcode = opcode;
    intent.argument = argument;
    ESP_LOGD("LIN_INTENT", "%s #%lu %s %s (%s) raw=%02X %02X", source,
             static_cast<unsigned long>(intent.sequence), action, key, phase,
             opcode, argument);
  }

  void observe_(ChannelState &channel, const char *source, uint8_t opcode,
                uint8_t argument, bool ignore_pid5e_housekeeping) {
    if (channel.have_last && channel.last_opcode == opcode &&
        channel.last_argument == argument) {
      return;
    }
    channel.have_last = true;
    channel.last_opcode = opcode;
    channel.last_argument = argument;

    if (ignore_pid5e_housekeeping &&
        ((opcode == 0xB0 && argument == 0x02) ||
         (opcode == 0xB1 && argument == 0x01))) {
      return;
    }

    if (opcode == 0x3F && argument == 0x00) {
      channel.active_toggle_key.clear();
      if (!channel.active_motion_key.empty()) {
        const std::string stopped = channel.active_motion_key;
        channel.active_motion_key.clear();
        emit_(source, stopped.c_str(), "motion_stop", "release", opcode, argument);
      }
      return;
    }

    if (const char *key = toggle_key_(opcode, argument)) {
      if (channel.active_toggle_key == key) return;
      channel.active_toggle_key = key;
      emit_(source, key, "toggle", "press", opcode, argument);
      return;
    }

    if (const char *key = motion_key_(opcode, argument)) {
      channel.active_toggle_key.clear();
      if (channel.active_motion_key == key) return;
      channel.active_motion_key = key;
      emit_(source, key, "motion_start", (opcode & 0x40) ? "active" : "request",
            opcode, argument);
    }
  }
};

}  // namespace precision_plex
