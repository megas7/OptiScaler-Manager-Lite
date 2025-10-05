#include "launcher.h"

#include <shellapi.h>
#include <windows.h>

namespace optiscaler {

namespace {

bool LaunchSteamApp(uint32_t app_id) {
  wchar_t buffer[64];
  swprintf(buffer, 64, L"steam://run/%u", app_id);
  HINSTANCE result = ShellExecuteW(nullptr, L"open", buffer, nullptr, nullptr, SW_SHOWNORMAL);
  return reinterpret_cast<INT_PTR>(result) > 32;
}

bool LaunchExecutable(const std::wstring& exe_path) {
  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  std::wstring command = exe_path;
  if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
    return false;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return true;
}

}  // namespace

bool Launcher::Run(const GameEntry& game, std::wstring& error_out) {
  error_out.clear();
  if (game.source == L"steam" && game.steamAppId.has_value()) {
    if (LaunchSteamApp(game.steamAppId.value())) {
      return true;
    }
    error_out = L"Failed to launch via Steam URI.";
    return false;
  }
  if (game.exe.empty()) {
    error_out = L"Game executable path missing.";
    return false;
  }
  if (LaunchExecutable(game.exe)) {
    return true;
  }
  error_out = L"Failed to launch executable.";
  return false;
}

}  // namespace optiscaler
