#include "steam_store.h"

namespace optiscaler {

std::optional<SteamStoreInfo> SteamStore::Fetch(uint32_t /*app_id*/) {
  // TODO: Query Steam Store API for supplemental metadata.
  return std::nullopt;
}

}  // namespace optiscaler
