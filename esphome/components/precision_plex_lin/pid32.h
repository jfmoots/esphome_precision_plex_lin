#pragma once
#include "protocol.h"
#include "coach_state.h"

namespace precision_plex {

class PID32Decoder {
 public:
  void decode(const Frame &f) {
    const auto raw = raw_string(f);
    CoachState &state = coach_state();
    state.pid32_last_frame = raw;

    uint8_t b3 = f.bytes[3];
    uint8_t b4 = f.bytes[4];
    uint8_t b5 = f.bytes[5];
    uint8_t b8 = f.bytes[8];

    state.outputs.valid = true;
    state.outputs.last_seen_ms = millis();
    state.outputs.awning_light = b3 & 0x01;
    state.outputs.water_heater = b3 & 0x10;
    state.outputs.tank_heater = b3 & 0x40;
    state.outputs.water_pump = b3 & 0x80;

    state.outputs.sofa_extend = b5 & 0x01;
    state.outputs.sofa_retract = b4 & 0x80;
    state.outputs.awning_retract = b4 & 0x02;
    state.outputs.awning_extend = b4 & 0x04;
    state.outputs.bedroom_slide_extend = b5 & 0x10;
    state.outputs.bedroom_slide_retract = b5 & 0x08;
    state.outputs.wardrobe_slide_retract = b5 & 0x02;
    state.outputs.wardrobe_slide_extend = b5 & 0x04;
    state.outputs.generator_running_bit = b8 & 0x40;
    state.outputs.raw_frame = raw;

    const std::string payload = payload_string(f);
    if (payload == last_payload_) return;
    last_payload_ = payload;
    state.outputs.revision++;

    ESP_LOGI("PID32", "changed raw=%s", raw.c_str());

    ESP_LOGI(
      "OUTPUTS",
      "AwningLight=%s WaterHeater=%s TankHeater=%s WaterPump=%s",
      onoff(state.outputs.awning_light), onoff(state.outputs.water_heater), onoff(state.outputs.tank_heater), onoff(state.outputs.water_pump)
    );

    ESP_LOGI(
      "MOTION",
      "SofaExt=%s SofaRet=%s AwnRet=%s AwnExt=%s BedExt=%s BedRet=%s WardRet=%s WardExt=%s",
      onoff(state.outputs.sofa_extend), onoff(state.outputs.sofa_retract),
      onoff(state.outputs.awning_retract), onoff(state.outputs.awning_extend),
      onoff(state.outputs.bedroom_slide_extend), onoff(state.outputs.bedroom_slide_retract),
      onoff(state.outputs.wardrobe_slide_retract), onoff(state.outputs.wardrobe_slide_extend)
    );

    ESP_LOGI("GENERATOR", "RunningBit=%s b8=0x%02X", onoff(state.outputs.generator_running_bit), b8);
  }

 private:
  std::string last_payload_;
};

}  // namespace precision_plex
