#pragma once

#include <string>
#include <vector>

#include "renderer_factory.h"

namespace optiscaler {

struct SettingsData {
  std::wstring optiScalerDirectory;
  bool autoUpdate = false;
  RendererPreference rendererPreference = RendererPreference::kAuto;
  std::wstring igdbClientId;
  std::wstring igdbClientSecret;
  std::wstring igdbToken;
  std::wstring igdbTokenExpiresUtc;
  std::vector<std::wstring> customScanFolders;
};

class Settings {
 public:
  static std::wstring FilePath();
  static bool Load(SettingsData& data);
  static bool Save(const SettingsData& data);
};

}  // namespace optiscaler

