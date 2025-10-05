#pragma once

#include <string>

#include <windows.h>

namespace optiscaler {

class LocalMeta {
 public:
  static HBITMAP IconAsCover(const std::wstring& exe_path, int width, int height);
};

}  // namespace optiscaler
