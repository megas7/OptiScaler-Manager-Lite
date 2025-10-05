#pragma once

#include <map>
#include <string>

#include "game_types.h"

namespace optiscaler {

class GameConfig {
 public:
  bool Load();
  bool Save() const;
  void SetGameOverride(const std::wstring& exe_path, bool enabled);
  bool GetGameOverride(const std::wstring& exe_path) const;

 private:
  std::map<std::wstring, bool> overrides_;
};

}  // namespace optiscaler
