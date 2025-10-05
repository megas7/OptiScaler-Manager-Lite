#include "optiscaler.h"

namespace optiscaler {

bool OptiScalerManager::SetInstallDirectory(const std::wstring& path) {
  install_dir_ = path;
  return true;
}

std::wstring OptiScalerManager::InstallDirectory() const {
  return install_dir_;
}

bool OptiScalerManager::AutoUpdateEnabled() const {
  return auto_update_enabled_;
}

void OptiScalerManager::SetAutoUpdateEnabled(bool enabled) {
  auto_update_enabled_ = enabled;
}

void OptiScalerManager::SetFallbackPackageDirectory(const std::wstring& path) {
  fallback_dir_ = path;
}

std::wstring OptiScalerManager::FallbackPackageDirectory() const {
  return fallback_dir_;
}

bool OptiScalerManager::ApplyInjection(const GameEntry& /*game*/, std::wstring& error_out) {
  error_out = L"Injection pipeline not yet implemented.";
  return false;
}

bool OptiScalerManager::CheckForUpdates(std::wstring& error_out) {
  if (!auto_update_enabled_) {
    error_out = L"Auto-update disabled.";
    return false;
  }
  error_out = L"Update check not yet implemented.";
  return false;
}

}  // namespace optiscaler
