#pragma once
#include "protocol.h"

namespace precision_plex {

class PID1FDecoder {
 public:
  void decode(const Frame &f) {
    const std::string payload = payload_string(f);
    if (payload == last_payload_) return;
    last_payload_ = payload;

    const uint8_t b0 = f.bytes[3];
    const uint8_t b1 = f.bytes[4];
    const char *label = label_for_static_(b0, b1);

    ESP_LOGI(
      "PID_1F",
      "Touchscreen channel: payload=%02X %02X | %s | raw=%s",
      b0,
      b1,
      label,
      raw_string(f).c_str()
    );
  }

 public:
  static const char *label_for_payload(uint8_t b0, uint8_t b1) { return label_for_static_(b0, b1); }

 private:
  std::string last_payload_;

  static const char *label_for_static_(uint8_t b0, uint8_t b1) {
    // Confirmed by captures:
    // 3F 00: normal idle when touchscreen is connected and ready.
    // Latched output toggles are followed by PID32 state confirmation.
    // Motion commands show a request phase (1x) and accepted/active phase (5x),
    // then PID32 motion bits confirm movement.
    // 81/90-93: touchscreen boot / registration sequence observed during reconnect.
    if (b0 == 0x3F && b1 == 0x00) return "idle / ready";

    if (b0 == 0x00 && b1 == 0x00) return "output toggle: awning light";
    if (b0 == 0x04 && b1 == 0x00) return "output toggle: water heater";
    if (b0 == 0x06 && b1 == 0x00) return "output toggle: tank heater";
    if (b0 == 0x07 && b1 == 0x00) return "output toggle: water pump";

    // Confirmed motion commands. PID32 motion bits validate accepted/active movement.
    if (b0 == 0x49 && b1 == 0x00) return "motion command: patio awning retract";
    if (b0 == 0x4A && b1 == 0x00) return "motion command: patio awning extend";
    if (b0 == 0x4F && b1 == 0x00) return "motion command: sofa retract";
    if (b0 == 0x50 && b1 == 0x00) return "motion command: sofa extend";
    if (b0 == 0x51 && b1 == 0x00) return "motion command: wardrobe retract";
    if (b0 == 0x52 && b1 == 0x00) return "motion command: wardrobe extend";
    if (b0 == 0x53 && b1 == 0x00) return "motion command: bedroom retract";
    if (b0 == 0x54 && b1 == 0x00) return "motion command: bedroom extend";

    // HVAC touchscreen commands observed on rear zone. The second byte appears
    // to carry an HVAC action code. These labels are still discovery labels and
    // should be correlated with the next PID37 status update.
    if (b0 == 0x3E && b1 == 0x91) return "HVAC command: fan setting changed while mode off / low candidate";
    if (b0 == 0x3E && b1 == 0x95) return "HVAC command: mode -> heat pump candidate";
    if (b0 == 0x3E && b1 == 0x96) return "HVAC command: mode -> off / cooldown candidate";
    if (b0 == 0x3E && b1 == 0x97) return "HVAC command: mode -> cool candidate";
    if (b0 == 0x3E && b1 == 0x9A) return "HVAC command: fan off candidate";

    if (b0 == 0x81 && b1 == 0x01) return "touchscreen boot/register step 1";
    if (b0 == 0x81 && b1 == 0x02) return "touchscreen boot/register step 2";
    if (b0 == 0x81 && b1 == 0x03) return "touchscreen boot/register step 3";

    if (b0 == 0x90 && b1 == 0x00) return "touchscreen boot/status 0x90";
    if (b0 == 0x91 && b1 == 0x00) return "touchscreen boot/status 0x91";
    if (b0 == 0x92 && b1 == 0x00) return "touchscreen boot/status 0x92";
    if (b0 == 0x93 && b1 == 0x00) return "touchscreen boot/status 0x93";

    // This malformed-looking form is seen while the touchscreen is physically
    // absent or rebooting: the PMM schedule still has the 1F slot, but the
    // touchscreen is not answering with a full valid 1F payload/checksum.
    if (b0 == 0x00 && b1 == 0x55) return "incomplete/no touchscreen response signature";

    return "unmapped touchscreen command/status";
  }
};

}  // namespace precision_plex
