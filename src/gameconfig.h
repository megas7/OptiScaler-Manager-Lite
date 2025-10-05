#pragma once

#include <map>
#include <string>
#include <unordered_set>

#include "game_types.h"

namespace optiscaler {

struct GameInjectionSettings {
  bool enabled = true;
  std::vector<std::wstring> files;
};

class GameConfig {
 public:
  bool Load();
  bool Save() const;

  GameInjectionSettings GetSettingsFor(const std::wstring& exe_path) const;
  void SetSettingsFor(const std::wstring& exe_path, const GameInjectionSettings& settings);
  void RemoveMissing(const std::unordered_set<std::wstring>& present_paths);

 private:
  std::map<std::wstring, GameInjectionSettings> overrides_;
};

}  // namespace optiscaler
