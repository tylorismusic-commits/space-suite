// Space Suite — typed parameter values
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "params.hpp"

namespace space {

inline std::string lowerText(std::string s) {
  for (char& c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

inline std::string trimText(const std::string& s) {
  const size_t a = s.find_first_not_of(" \t");
  if (a == std::string::npos) return {};
  const size_t b = s.find_last_not_of(" \t");
  return s.substr(a, b - a + 1);
}

inline std::string formatParamValue(const Shared&, uint32_t id, double value) {
  const ParamSpec& s = kParamSpecs[id];
  if (s.labels.count > 0) {
    const int i = std::clamp(static_cast<int>(std::lround(value - s.min)), 0,
                             s.labels.count - 1);
    return s.labels.names[i];
  }
  char buf[64];
  if (s.toggled) {
    return value > 0.5 ? "On" : "Off";
  }
  if (id == P_MIX || id == P_REVERB_SIZE || id == P_REVERB_TONE ||
      id == P_REVERB_WIDTH || id == P_REVERB_DUCK || id == P_REVERB_MIX ||
      id == P_DELAY_FEEDBACK || id == P_DELAY_TONE || id == P_DELAY_MOD ||
      id == P_DELAY_WIDTH || id == P_DELAY_DUCK || id == P_DELAY_MIX ||
      id == P_MOD_DEPTH || id == P_MOD_FEEDBACK || id == P_MOD_WIDTH ||
      id == P_MOD_MIX) {
    std::snprintf(buf, sizeof(buf), "%.0f%%", value);
    return buf;
  }
  if (id == P_REVERB_DECAY) {
    std::snprintf(buf, sizeof(buf), "%.2f s", value);
    return buf;
  }
  if (id == P_REVERB_PREDELAY || id == P_DELAY_TIME) {
    std::snprintf(buf, sizeof(buf), "%.0f ms", value);
    return buf;
  }
  if (id == P_DELAY_DIV) {
    return kDivLabels[std::clamp(static_cast<int>(std::lround(value)), 0, 8)];
  }
  if (id == P_MOD_RATE) {
    std::snprintf(buf, sizeof(buf), "%.2f Hz", value);
    return buf;
  }
  if (id == P_BPM) {
    std::snprintf(buf, sizeof(buf), "%.0f BPM", value);
    return buf;
  }
  if (s.unit && std::strcmp(s.unit, "dB") == 0) {
    std::snprintf(buf, sizeof(buf), "%+.1f dB", value);
    return buf;
  }
  std::snprintf(buf, sizeof(buf), "%.2f", value);
  return buf;
}

inline std::string formatEditValue(const Shared& shared, uint32_t id,
                                   double value) {
  (void)shared;
  return formatParamValue(shared, id, value);
}

inline bool parseParamValue(const Shared&, uint32_t id,
                            const std::string& raw, double* out) {
  const ParamSpec& s = kParamSpecs[id];
  const std::string text = lowerText(trimText(raw));
  if (text.empty()) return false;

  for (int i = 0; i < s.labels.count; ++i) {
    if (text == lowerText(s.labels.names[i])) {
      *out = s.min + i;
      return true;
    }
  }
  if (text == "on" || text == "true" || text == "yes") {
    *out = 1.0;
    return true;
  }
  if (text == "off" || text == "false" || text == "no") {
    *out = 0.0;
    return true;
  }

  char* end = nullptr;
  double v = std::strtod(text.c_str(), &end);
  if (end == text.c_str()) return false;
  while (end && *end == ' ') ++end;
  if (end && *end == '%') {
    // Values are already in percent for percent parameters.
  } else if (end && (text.find("ms") != std::string::npos)) {
    // ms parameters are already in ms.
  } else if (end && (text.find("hz") != std::string::npos)) {
    // Hz parameters are already in Hz.
  } else if (end && (text.find("db") != std::string::npos)) {
    // dB parameters are already in dB.
  } else if (end && (text.find('s') != std::string::npos) &&
             id == P_REVERB_DECAY) {
    // seconds are already seconds
  }
  v = std::min(std::max(v, s.min), s.max);
  *out = v;
  return true;
}

} // namespace space
