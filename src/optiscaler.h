#pragma once

#include <string>
#include <vector>

#include "game_types.h"

namespace optiscaler {

class OptiScalerManager {
 public:
  bool SetInstallDirectory(const std::wstring& path);
  std::wstring InstallDirectory() const;
  bool AutoUpdateEnabled() const;
  void SetAutoUpdateEnabled(bool enabled);
  void SetFallbackPackageDirectory(const std::wstring& path);
  std::wstring FallbackPackageDirectory() const;
  bool ApplyInjection(const GameEntry& game, std::wstring& error_out);
  bool CheckForUpdates(std::wstring& error_out);

 private:
  std::wstring install_dir_;
  bool auto_update_enabled_ = false;
  std::wstring fallback_dir_;
};

}  // namespace optiscaler
