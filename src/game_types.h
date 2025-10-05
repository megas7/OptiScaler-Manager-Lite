#pragma once

#include <optional>
#include <string>
#include <vector>

#include <windows.h>

namespace optiscaler {

struct GameEntry {
  std::wstring name;
  std::wstring exe;
  std::wstring folder;
  std::wstring source;
  std::optional<uint32_t> steamAppId;
  HBITMAP coverBmp = nullptr;
  bool injectEnabled = false;
  std::vector<std::wstring> plannedFiles;
};

}  // namespace optiscaler
