#pragma once

#include <optional>
#include <string>

namespace optiscaler {

struct SteamStoreInfo {
  std::wstring description;
  std::wstring website;
};

class SteamStore {
 public:
  static std::optional<SteamStoreInfo> Fetch(uint32_t app_id);
};

}  // namespace optiscaler
