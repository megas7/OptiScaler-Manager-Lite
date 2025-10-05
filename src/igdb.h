#pragma once

#include <optional>
#include <string>
#include <vector>

namespace optiscaler {

struct IgdbGame {
  std::wstring name;
  std::wstring summary;
  std::vector<std::wstring> genres;
  std::vector<std::wstring> platforms;
  std::wstring coverImageId;
  std::wstring releaseDate;
  double rating = 0.0;
};

class IGDB {
 public:
  static bool EnsureAccessToken(const std::wstring& client_id,
                                const std::wstring& client_secret,
                                std::wstring& token,
                                std::wstring& expires_utc_iso,
                                std::wstring& error_out);
  static std::optional<IgdbGame> SearchOne(const std::wstring& client_id,
                                           const std::wstring& token,
                                           const std::wstring& name);
  static std::wstring ImageUrlCoverBig(const std::wstring& image_id);
  static std::wstring CacheImage(const std::wstring& url);
};

}  // namespace optiscaler
