#pragma once

#include <optional>
#include <string>

namespace optiscaler {

class SteamCover {
 public:
  static std::optional<std::wstring> GridImagePath(uint32_t app_id);
};

}  // namespace optiscaler
