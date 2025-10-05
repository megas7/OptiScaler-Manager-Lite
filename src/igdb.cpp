#include "igdb.h"

#include <windows.h>
#include <winhttp.h>

#include <filesystem>
#include <string>

#include "cache.h"
#include <nlohmann/json.hpp>

namespace optiscaler {

bool IGDB::EnsureAccessToken(const std::wstring& /*client_id*/,
                             const std::wstring& /*client_secret*/,
                             std::wstring& token,
                             std::wstring& expires_utc_iso,
                             std::wstring& error_out) {
  token.clear();
  expires_utc_iso.clear();
  error_out = L"IGDB token flow not yet implemented.";
  return false;
}

std::optional<IgdbGame> IGDB::SearchOne(const std::wstring& /*client_id*/,
                                        const std::wstring& /*token*/,
                                        const std::wstring& name) {
  IgdbGame game;
  game.name = name;
  return std::optional<IgdbGame>(std::in_place, std::move(game));
}

std::wstring IGDB::ImageUrlCoverBig(const std::wstring& image_id) {
  if (image_id.empty()) {
    return {};
  }
  return L"https://images.igdb.com/igdb/image/upload/t_cover_big/" + image_id + L".jpg";
}

std::wstring IGDB::CacheImage(const std::wstring& /*url*/) {
  // TODO: Download covers via WinHTTP and persist in cache.
  return {};
}

}  // namespace optiscaler
