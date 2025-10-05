#include "steam_cover.h"

namespace optiscaler {

std::optional<std::wstring> SteamCover::GridImagePath(uint32_t /*app_id*/) {
  // TODO: Derive Steam grid image path from library folders.
  return std::nullopt;
}

}  // namespace optiscaler
