#include "settings.h"

#include <windows.h>

#include <string>

#include "cache.h"
#include "nlohmann/json.hpp"

namespace optiscaler {

namespace {

using nlohmann::json;

std::string Utf8FromWide(const std::wstring& input) {
  if (input.empty()) {
    return {};
  }
  int count = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
  std::string utf8(static_cast<size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), utf8.data(), count, nullptr, nullptr);
  return utf8;
}

std::wstring WideFromUtf8(const std::string& input) {
  if (input.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), nullptr, 0);
  std::wstring wide(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), wide.data(), count);
  return wide;
}

void EnsureDefaults(SettingsData& settings) {
  if (settings.defaultInjectionFiles.empty()) {
    settings.defaultInjectionFiles = {L"OptiScaler\\OptiScaler.dll", L"OptiScaler\\OptiScaler.ini"};
  }
}

json SerializeStrings(const std::vector<std::wstring>& values) {
  json arr = json::array();
  for (const auto& value : values) {
    arr.push_back(Utf8FromWide(value));
  }
  return arr;
}

std::vector<std::wstring> DeserializeStrings(const json& arr) {
  std::vector<std::wstring> values;
  if (!arr.is_array()) {
    return values;
  }
  for (const auto& item : arr) {
    if (item.is_string()) {
      values.push_back(WideFromUtf8(item.get<std::string>()));
    }
  }
  return values;
}

}  // namespace

std::wstring SettingsManager::SettingsPath() {
  std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return {};
  }
  return root + L"\\settings.json";
}

bool SettingsManager::Load(SettingsData& settings) {
  EnsureDefaults(settings);
  const std::wstring path = SettingsPath();
  if (path.empty()) {
    return false;
  }
  std::wstring raw;
  if (!Cache::ReadText(path, raw)) {
    return false;
  }

  std::string utf8 = Utf8FromWide(raw);
  try {
    json doc = json::parse(utf8);
    settings.optiScalerInstallDir = WideFromUtf8(doc.value("optiScalerInstallDir", std::string()));
    settings.fallbackOptiScalerDir = WideFromUtf8(doc.value("fallbackOptiScalerDir", std::string()));
    settings.autoUpdateEnabled = doc.value("autoUpdateEnabled", settings.autoUpdateEnabled);
    settings.globalInjectionEnabled = doc.value("globalInjectionEnabled", settings.globalInjectionEnabled);
    if (doc.contains("defaultInjectionFiles")) {
      settings.defaultInjectionFiles = DeserializeStrings(doc["defaultInjectionFiles"]);
    }
    if (doc.contains("customScanFolders")) {
      settings.customScanFolders = DeserializeStrings(doc["customScanFolders"]);
    }
    EnsureDefaults(settings);
  } catch (const json::exception&) {
    return false;
  }
  return true;
}

bool SettingsManager::Save(const SettingsData& settings) {
  SettingsData tmp = settings;
  EnsureDefaults(tmp);
  const std::wstring path = SettingsPath();
  if (path.empty()) {
    return false;
  }
  std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return false;
  }
  Cache::EnsureDirectory(root);

  json doc;
  doc["optiScalerInstallDir"] = Utf8FromWide(tmp.optiScalerInstallDir);
  doc["fallbackOptiScalerDir"] = Utf8FromWide(tmp.fallbackOptiScalerDir);
  doc["autoUpdateEnabled"] = tmp.autoUpdateEnabled;
  doc["globalInjectionEnabled"] = tmp.globalInjectionEnabled;
  doc["defaultInjectionFiles"] = SerializeStrings(tmp.defaultInjectionFiles);
  doc["customScanFolders"] = SerializeStrings(tmp.customScanFolders);

  const std::wstring payload = WideFromUtf8(doc.dump(2));
  return Cache::WriteText(path, payload);
}

}  // namespace optiscaler
