#include "gameconfig.h"

#include <windows.h>

#include <string>

#include "cache.h"
#include "nlohmann/json.hpp"

namespace optiscaler {

namespace {

using nlohmann::json;

std::string Utf8FromWide(const std::wstring& value) {
  if (value.empty()) {
    return {};
  }
  int count = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  std::string utf8(static_cast<size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), utf8.data(), count, nullptr, nullptr);
  return utf8;
}

std::wstring WideFromUtf8(const std::string& value) {
  if (value.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
  std::wstring wide(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), count);
  return wide;
}

std::wstring ConfigPath() {
  std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return {};
  }
  return root + L"\\gameconfig.json";
}

}  // namespace

bool GameConfig::Load() {
  overrides_.clear();
  const std::wstring path = ConfigPath();
  if (path.empty()) {
    return false;
  }
  std::wstring raw;
  if (!Cache::ReadText(path, raw)) {
    return false;
  }

  try {
    nlohmann::json doc = nlohmann::json::parse(Utf8FromWide(raw));
    if (!doc.contains("games")) {
      return true;
    }
    const auto& games = doc["games"];
    if (!games.is_object()) {
      return true;
    }
    for (auto it = games.begin(); it != games.end(); ++it) {
      GameInjectionSettings settings;
      const auto& obj = it.value();
      if (obj.contains("enabled")) {
        settings.enabled = obj.value("enabled", true);
      }
      if (obj.contains("files")) {
        for (const auto& entry : obj["files"]) {
          if (entry.is_string()) {
            settings.files.push_back(WideFromUtf8(entry.get<std::string>()));
          }
        }
      }
      overrides_[WideFromUtf8(it.key())] = std::move(settings);
    }
  } catch (const nlohmann::json::exception&) {
    overrides_.clear();
    return false;
  }
  return true;
}

bool GameConfig::Save() const {
  const std::wstring path = ConfigPath();
  if (path.empty()) {
    return false;
  }
  std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return false;
  }
  Cache::EnsureDirectory(root);

  nlohmann::json doc;
  nlohmann::json games = nlohmann::json::object();
  for (const auto& pair : overrides_) {
    nlohmann::json entry;
    entry["enabled"] = pair.second.enabled;
    nlohmann::json files = nlohmann::json::array();
    for (const auto& file : pair.second.files) {
      files.push_back(Utf8FromWide(file));
    }
    entry["files"] = files;
    games[Utf8FromWide(pair.first)] = entry;
  }
  doc["games"] = games;

  const std::wstring payload = WideFromUtf8(doc.dump(2));
  return Cache::WriteText(path, payload);
}

GameInjectionSettings GameConfig::GetSettingsFor(const std::wstring& exe_path) const {
  auto it = overrides_.find(exe_path);
  if (it == overrides_.end()) {
    return {};
  }
  return it->second;
}

void GameConfig::SetSettingsFor(const std::wstring& exe_path, const GameInjectionSettings& settings) {
  overrides_[exe_path] = settings;
}

void GameConfig::RemoveMissing(const std::unordered_set<std::wstring>& present_paths) {
  for (auto it = overrides_.begin(); it != overrides_.end();) {
    if (present_paths.find(it->first) == present_paths.end()) {
      it = overrides_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace optiscaler
