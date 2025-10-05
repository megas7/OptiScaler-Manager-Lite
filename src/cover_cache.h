#pragma once

#include <string>

#include <windows.h>

namespace optiscaler {

class CoverCache {
 public:
  static std::wstring PathForExe(const std::wstring& exe_path);
  static HBITMAP LoadForExe(const std::wstring& exe_path, int width, int height);
  static bool SaveImageFileForExe(const std::wstring& image_path,
                                  const std::wstring& exe_path,
                                  int width,
                                  int height);
};

}  // namespace optiscaler
