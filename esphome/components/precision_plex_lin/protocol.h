#pragma once

#include <Arduino.h>
#include <string>
#include <vector>
#include <cstring>
#include "esphome.h"

namespace precision_plex {

struct Frame {
  uint8_t pid = 0;
  int len = 0;
  uint8_t bytes[16] = {0};
};

inline const char *onoff(bool value) {
  return value ? "ON" : "OFF";
}

inline std::string payload_string(const Frame &f) {
  std::string out;
  char part[8];
  for (int i = 3; i < f.len - 1; i++) {
    snprintf(part, sizeof(part), "%02X ", f.bytes[i]);
    out += part;
  }
  return out;
}

inline std::string raw_string(const Frame &f) {
  std::string out;
  char part[8];
  for (int i = 0; i < f.len; i++) {
    snprintf(part, sizeof(part), "%02X ", f.bytes[i]);
    out += part;
  }
  return out;
}

inline bool checksum_ok(const Frame &f) {
  uint16_t sum = 0;
  for (int i = 2; i < f.len - 1; i++) {
    sum += f.bytes[i];
    if (sum > 0xFF) {
      sum = (sum & 0xFF) + 1;
    }
  }
  uint8_t calc = (uint8_t)(0xFF - sum);
  return calc == f.bytes[f.len - 1];
}

inline int expected_len(uint8_t pid) {
  if (pid == 0x5E || pid == 0x9C || pid == 0xDD || pid == 0x1F) return 6;
  if (pid == 0xEC) return 8;
  if (pid == 0xBA || pid == 0x76 || pid == 0x32 || pid == 0x37) return 12;
  return 0;
}

}  // namespace precision_plex
