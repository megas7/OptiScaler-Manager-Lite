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
  std::wstring supportSummary;
  int compatProfileIndex = -1;
  std::vector<std::wstring> compatBadges;
  std::vector<std::wstring> compatKnownIssues;
  std::vector<std::wstring> compatNotes;
  std::vector<std::wstring> compatInputs;
  std::wstring compatDll;
  std::wstring compatTestedVersion;
  bool compatRequiresFakenvapi = false;
  bool compatFgSupported = true;
}; 

}  // namespace optiscaler
