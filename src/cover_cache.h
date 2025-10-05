#pragma once

#include <string>

#include <windows.h>

namespace optiscaler {

class CoverCache {
 public:
  static std::wstring PathForExe(const std::wstring& exe_path);
  static HBITMAP LoadForExe(const std::wstring& exe_path, int width, int height);
  static bool SaveForExe(HBITMAP bitmap, const std::wstring& exe_path);
};

}  // namespace optiscaler
