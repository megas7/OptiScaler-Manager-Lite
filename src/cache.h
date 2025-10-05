#pragma once

#include <string>

namespace optiscaler {

class Cache {
 public:
  static std::wstring AppDataRoot();
  static bool EnsureDirectory(const std::wstring& path);
  static bool WriteText(const std::wstring& path, const std::wstring& text);
  static bool ReadText(const std::wstring& path, std::wstring& text_out);
};

}  // namespace optiscaler
