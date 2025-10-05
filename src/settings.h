#pragma once

#include <string>
#include <vector>

namespace optiscaler {

struct SettingsData {
  std::wstring optiScalerInstallDir;
  std::wstring fallbackOptiScalerDir;
  bool autoUpdateEnabled = false;
  bool globalInjectionEnabled = true;
  std::vector<std::wstring> defaultInjectionFiles;
  std::vector<std::wstring> customScanFolders;
};

class SettingsManager {
 public:
  static bool Load(SettingsData& settings);
  static bool Save(const SettingsData& settings);
  static std::wstring SettingsPath();
};

}  // namespace optiscaler
