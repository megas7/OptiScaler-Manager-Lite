#include "gameconfig.h"

namespace optiscaler {

bool GameConfig::Load() {
  // TODO: Load configuration from disk.
  overrides_.clear();
  return true;
}

bool GameConfig::Save() const {
  // TODO: Persist configuration to disk.
  return true;
}

void GameConfig::SetGameOverride(const std::wstring& exe_path, bool enabled) {
  overrides_[exe_path] = enabled;
}

bool GameConfig::GetGameOverride(const std::wstring& exe_path) const {
  auto it = overrides_.find(exe_path);
  if (it == overrides_.end()) {
    return false;
  }
  return it->second;
}

}  // namespace optiscaler
