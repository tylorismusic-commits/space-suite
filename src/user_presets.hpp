// Space Suite — user preset files
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include "params.hpp"

namespace space {

inline constexpr const char* kUserPresetExt = ".space-suite";

struct UserPreset {
  std::string name;
  std::vector<double> values;
  int page = -1;  // which editor page the preset belongs to, -1 = any
};

inline std::filesystem::path userPresetDir() {
  const char* xdg = std::getenv("XDG_CONFIG_HOME");
  std::filesystem::path base;
  if (xdg && *xdg) {
    base = xdg;
  } else {
    const char* home = std::getenv("HOME");
    base = home && *home ? std::filesystem::path(home) / ".config"
                         : std::filesystem::path(".");
  }
  return base / "SpaceSuite" / "presets";
}

inline std::filesystem::path userPresetExportDir() {
  const char* home = std::getenv("HOME");
  const std::filesystem::path base =
      home && *home ? std::filesystem::path(home) : std::filesystem::path(".");
  return base / "Documents" / "Space Suite Presets";
}

inline std::string sanitizePresetName(const std::string& raw) {
  std::string out;
  for (char c : raw) {
    const unsigned char u = static_cast<unsigned char>(c);
    if (std::isalnum(u) || c == ' ' || c == '-' || c == '_' || c == '.' ||
        c == '(' || c == ')') {
      out.push_back(c);
    } else {
      out.push_back('_');
    }
  }
  const size_t a = out.find_first_not_of(' ');
  if (a == std::string::npos) return {};
  const size_t b = out.find_last_not_of(' ');
  out = out.substr(a, b - a + 1);
  if (out.size() > 60) out.resize(60);
  return out;
}

inline void sanitizePresetValues(std::vector<double>& values) {
  for (uint32_t i = 0; i < values.size() && i < P_COUNT; ++i) {
    if (!std::isfinite(values[i])) values[i] = kParamSpecs[i].def;
    values[i] = std::min(std::max(values[i], kParamSpecs[i].min),
                         kParamSpecs[i].max);
  }
}

inline std::string uniquePresetName(const std::filesystem::path& dir,
                                    const std::string& raw) {
  std::string base = sanitizePresetName(raw);
  if (base.empty()) base = "Preset";
  if (base.size() > 50) base.resize(50);
  std::error_code ec;
  std::string candidate = base;
  int suffix = 2;
  while (std::filesystem::exists(dir / (candidate + kUserPresetExt), ec)) {
    ec.clear();
    candidate = base + " (" + std::to_string(suffix++) + ")";
  }
  return candidate;
}

inline bool readUserPresetFile(const std::filesystem::path& path,
                               UserPreset& preset) {
  std::ifstream in(path);
  if (!in) return false;
  std::string header;
  std::getline(in, header);
  if (header.rfind("SpaceSuitePreset ", 0) != 0) return false;

  preset = UserPreset{};
  preset.values.assign(P_COUNT, 0.0);
  for (uint32_t i = 0; i < P_COUNT; ++i) {
    preset.values[i] = kParamSpecs[i].def;
  }
  std::string line;
  bool haveValue = false;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    if (line.rfind("name ", 0) == 0) {
      preset.name = sanitizePresetName(line.substr(5));
      continue;
    }
    if (line.rfind("page ", 0) == 0) {
      preset.page = std::atoi(line.c_str() + 5);
      continue;
    }
    const size_t space = line.find(' ');
    if (space == std::string::npos) continue;
    const int index = std::atoi(line.substr(0, space).c_str());
    if (index < 0 || index >= static_cast<int>(P_COUNT)) continue;
    char* end = nullptr;
    const double value = std::strtod(line.c_str() + space + 1, &end);
    if (end == line.c_str() + space + 1) continue;
    preset.values[static_cast<size_t>(index)] = value;
    haveValue = true;
  }
  if (preset.name.empty()) {
    preset.name = sanitizePresetName(path.stem().string());
  }
  if (preset.name.empty() || !haveValue) return false;
  preset.page = std::min(std::max(preset.page, -1), 2);
  sanitizePresetValues(preset.values);
  return true;
}

inline bool writeUserPreset(const std::filesystem::path& dir,
                            const std::string& rawName,
                            const std::vector<double>& values, int page = -1) {
  const std::string name = sanitizePresetName(rawName);
  if (name.empty()) return false;
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) return false;

  const std::filesystem::path finalPath = dir / (name + kUserPresetExt);
  const std::filesystem::path tempPath =
      dir / (name + std::string(kUserPresetExt) + ".tmp");
  std::ofstream out(tempPath, std::ios::trunc);
  if (!out) return false;
  out << "SpaceSuitePreset 1\n";
  out << "name " << name << "\n";
  if (page >= 0 && page <= 2) out << "page " << page << "\n";
  out.precision(17);
  for (uint32_t i = 0; i < values.size(); ++i) {
    out << i << ' ' << values[i] << '\n';
  }
  out.close();
  if (!out) return false;
  std::filesystem::rename(tempPath, finalPath, ec);
  if (ec) {
    std::filesystem::remove(finalPath, ec);
    ec.clear();
    std::filesystem::rename(tempPath, finalPath, ec);
  }
  return !ec;
}

inline bool saveUserPreset(const std::filesystem::path& dir,
                           const std::string& rawName, const Shared& shared,
                           int page) {
  std::vector<double> values;
  values.reserve(P_COUNT);
  for (uint32_t i = 0; i < P_COUNT; ++i) values.push_back(shared.get(i));
  return writeUserPreset(dir, rawName, values, page);
}

inline bool deleteUserPreset(const std::filesystem::path& dir,
                             const std::string& rawName) {
  const std::string name = sanitizePresetName(rawName);
  if (name.empty()) return false;
  std::error_code ec;
  return std::filesystem::remove(dir / (name + kUserPresetExt), ec) && !ec;
}

inline bool renameUserPreset(const std::filesystem::path& dir,
                             const std::string& oldName,
                             const std::string& newName,
                             const std::vector<double>& values, int page) {
  const std::string oldClean = sanitizePresetName(oldName);
  const std::string newClean = sanitizePresetName(newName);
  if (oldClean.empty() || newClean.empty()) return false;
  if (oldClean != newClean) {
    std::error_code ec;
    if (std::filesystem::exists(dir / (newClean + kUserPresetExt), ec) && !ec) {
      return false;
    }
  }
  if (!writeUserPreset(dir, newClean, values, page)) return false;
  if (oldClean != newClean) deleteUserPreset(dir, oldClean);
  return true;
}

inline std::vector<UserPreset> loadUserPresets(
    const std::filesystem::path& dir) {
  std::vector<UserPreset> out;
  std::error_code ec;
  if (!std::filesystem::is_directory(dir, ec)) return out;
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (ec) break;
    if (!entry.is_regular_file(ec) || ec) continue;
    if (entry.path().extension() == kUserPresetExt) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  for (const auto& path : files) {
    UserPreset preset;
    if (readUserPresetFile(path, preset)) out.push_back(std::move(preset));
  }
  std::sort(out.begin(), out.end(),
            [](const UserPreset& a, const UserPreset& b) {
              std::string al = a.name;
              std::string bl = b.name;
              std::transform(al.begin(), al.end(), al.begin(),
                             [](unsigned char c) {
                               return static_cast<char>(std::tolower(c));
                             });
              std::transform(bl.begin(), bl.end(), bl.begin(),
                             [](unsigned char c) {
                               return static_cast<char>(std::tolower(c));
                             });
              return al < bl;
            });
  return out;
}

} // namespace space
