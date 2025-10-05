#include "igdb.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "cache.h"
#include <json.hpp>

#pragma comment(lib, "winhttp.lib")

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 0x00000004
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000008
#endif

namespace optiscaler {
namespace {

using json = nlohmann::json;

std::string Utf8FromWide(const std::wstring& input) {
  if (input.empty()) {
    return {};
  }
  int count = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
  std::string utf8(static_cast<size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), utf8.data(), count, nullptr, nullptr);
  return utf8;
}

std::wstring WideFromUtf8(const std::string& input) {
  if (input.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), nullptr, 0);
  std::wstring wide(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), wide.data(), count);
  return wide;
}

std::wstring FormatIso8601(const FILETIME& file_time) {
  SYSTEMTIME st = {};
  FileTimeToSystemTime(&file_time, &st);
  wchar_t buffer[64];
  swprintf(buffer, _countof(buffer), L"%04u-%02u-%02uT%02u:%02u:%02uZ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
           st.wSecond);
  return buffer;
}

bool ParseIso8601(const std::wstring& iso, FILETIME* out) {
  if (!out || iso.size() < 19) {
    return false;
  }
  SYSTEMTIME st = {};
  wchar_t tz = L'Z';
  int matched = swscanf(iso.c_str(), L"%hu-%hu-%huT%hu:%hu:%hu%c", &st.wYear, &st.wMonth, &st.wDay, &st.wHour, &st.wMinute,
                        &st.wSecond, &tz);
  if (matched < 6) {
    return false;
  }
  st.wMilliseconds = 0;
  return SystemTimeToFileTime(&st, out) == TRUE;
}

FILETIME AddSeconds(const FILETIME& ft, uint32_t seconds) {
  ULARGE_INTEGER value;
  value.LowPart = ft.dwLowDateTime;
  value.HighPart = ft.dwHighDateTime;
  value.QuadPart += static_cast<unsigned long long>(seconds) * 10000000ull;
  FILETIME result;
  result.dwLowDateTime = value.LowPart;
  result.dwHighDateTime = value.HighPart;
  return result;
}

bool IsTokenFresh(const std::wstring& expiry_iso) {
  if (expiry_iso.empty()) {
    return false;
  }
  FILETIME expiry = {};
  if (!ParseIso8601(expiry_iso, &expiry)) {
    return false;
  }
  FILETIME now;
  GetSystemTimeAsFileTime(&now);
  ULARGE_INTEGER expiry_value;
  expiry_value.LowPart = expiry.dwLowDateTime;
  expiry_value.HighPart = expiry.dwHighDateTime;
  ULARGE_INTEGER now_value;
  now_value.LowPart = now.dwLowDateTime;
  now_value.HighPart = now.dwHighDateTime;
  const unsigned long long margin = 60ull * 10000000ull;  // 60 seconds in 100-ns units
  return expiry_value.QuadPart > now_value.QuadPart + margin;
}

std::string UrlEncode(const std::wstring& value) {
  static const char hex[] = "0123456789ABCDEF";
  std::string utf8 = Utf8FromWide(value);
  std::string encoded;
  encoded.reserve(utf8.size() * 3);
  for (unsigned char c : utf8) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
        c == '~') {
      encoded.push_back(static_cast<char>(c));
    } else {
      encoded.push_back('%');
      encoded.push_back(hex[(c >> 4) & 0xF]);
      encoded.push_back(hex[c & 0xF]);
    }
  }
  return encoded;
}

struct ScopedHInternet {
  ScopedHInternet() = default;
  explicit ScopedHInternet(HINTERNET handle) : handle_(handle) {}
  ScopedHInternet(const ScopedHInternet&) = delete;
  ScopedHInternet& operator=(const ScopedHInternet&) = delete;
  ScopedHInternet(ScopedHInternet&& other) noexcept : handle_(other.release()) {}
  ScopedHInternet& operator=(ScopedHInternet&& other) noexcept {
    if (this != &other) {
      reset(other.release());
    }
    return *this;
  }
  ~ScopedHInternet() { reset(); }

  void reset(HINTERNET handle = nullptr) {
    if (handle_) {
      WinHttpCloseHandle(handle_);
    }
    handle_ = handle;
  }

  [[nodiscard]] HINTERNET get() const { return handle_; }
  explicit operator bool() const { return handle_ != nullptr; }
  HINTERNET release() {
    HINTERNET temp = handle_;
    handle_ = nullptr;
    return temp;
  }

 private:
  HINTERNET handle_ = nullptr;
};

bool HttpRequest(const std::wstring& method,
                 const std::wstring& url,
                 const std::vector<std::pair<std::wstring, std::wstring>>& headers,
                 const std::string& request_body,
                 std::vector<uint8_t>& response_body,
                 DWORD& status_code) {
  status_code = 0;
  std::wstring url_copy = url;
  URL_COMPONENTSW comps = {};
  comps.dwStructSize = sizeof(comps);
  if (!WinHttpCrackUrl(url_copy.c_str(), static_cast<DWORD>(url_copy.size()), 0, &comps)) {
    return false;
  }
  std::wstring host(comps.lpszHostName, comps.dwHostNameLength);
  std::wstring path(comps.lpszUrlPath, comps.dwUrlPathLength);
  if (comps.dwExtraInfoLength > 0 && comps.lpszExtraInfo) {
    path.append(comps.lpszExtraInfo, comps.dwExtraInfoLength);
  }
  const bool secure = comps.nScheme == INTERNET_SCHEME_HTTPS;

  ScopedHInternet session(WinHttpOpen(L"OptiScalerMgrLite/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0));
  if (!session) {
    return false;
  }
  DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1;
  WinHttpSetOption(session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
  WinHttpSetTimeouts(session.get(), 5000, 5000, 15000, 30000);

  ScopedHInternet connection(WinHttpConnect(session.get(), host.c_str(), comps.nPort, 0));
  if (!connection) {
    return false;
  }

  ScopedHInternet request(WinHttpOpenRequest(connection.get(), method.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0));
  if (!request) {
    return false;
  }

  for (const auto& header : headers) {
    std::wstring header_line = header.first + L": " + header.second;
    WinHttpAddRequestHeaders(request.get(), header_line.c_str(), -1,
                             WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
  }

  const void* body_ptr = request_body.empty() ? WINHTTP_NO_REQUEST_DATA : request_body.data();
  const DWORD body_size = request_body.empty() ? 0 : static_cast<DWORD>(request_body.size());
  if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, const_cast<void*>(body_ptr), body_size,
                          body_size, 0)) {
    return false;
  }
  if (!WinHttpReceiveResponse(request.get(), nullptr)) {
    return false;
  }

  DWORD status = 0;
  DWORD status_length = sizeof(status);
  if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status,
                           &status_length, WINHTTP_NO_HEADER_INDEX)) {
    return false;
  }
  status_code = status;

  response_body.clear();
  DWORD available = 0;
  do {
    if (!WinHttpQueryDataAvailable(request.get(), &available)) {
      return false;
    }
    if (available == 0) {
      break;
    }
    size_t start = response_body.size();
    response_body.resize(start + available);
    DWORD read = 0;
    if (!WinHttpReadData(request.get(), response_body.data() + start, available, &read)) {
      return false;
    }
    response_body.resize(start + read);
  } while (available > 0);

  return true;
}

std::wstring EscapeQuery(const std::wstring& value) {
  std::wstring escaped;
  escaped.reserve(value.size());
  for (wchar_t ch : value) {
    if (ch == L'"') {
      escaped.push_back(L'\\');
    }
    escaped.push_back(ch);
  }
  return escaped;
}

uint64_t HashString(const std::wstring& value) {
  constexpr uint64_t kOffset = 1469598103934665603ull;
  constexpr uint64_t kPrime = 1099511628211ull;
  uint64_t hash = kOffset;
  for (wchar_t ch : value) {
    hash ^= static_cast<uint64_t>(towlower(ch));
    hash *= kPrime;
  }
  return hash;
}

}  // namespace

bool IGDB::EnsureAccessToken(const std::wstring& client_id,
                             const std::wstring& client_secret,
                             std::wstring& token,
                             std::wstring& expires_utc_iso,
                             std::wstring& error_out) {
  if (client_id.empty() || client_secret.empty()) {
    error_out = L"IGDB client credentials are not configured.";
    return false;
  }

  if (!token.empty() && IsTokenFresh(expires_utc_iso)) {
    error_out.clear();
    return true;
  }

  std::vector<std::pair<std::wstring, std::wstring>> headers = {
      {L"Content-Type", L"application/x-www-form-urlencoded"},
      {L"Accept", L"application/json"}};

  std::string body = "client_id=" + UrlEncode(client_id);
  body.append("&client_secret=");
  body.append(UrlEncode(client_secret));
  body.append("&grant_type=client_credentials");

  std::vector<uint8_t> response;
  DWORD status = 0;
  if (!HttpRequest(L"POST", L"https://id.twitch.tv/oauth2/token", headers, body, response, status)) {
    error_out = L"Failed to contact Twitch OAuth endpoint.";
    return false;
  }
  if (status != HTTP_STATUS_OK) {
    error_out = L"Twitch OAuth request failed with status " + std::to_wstring(status) + L".";
    return false;
  }

  std::string text(response.begin(), response.end());
  try {
    json doc = json::parse(text);
    std::string access_token = doc.value("access_token", std::string());
    if (access_token.empty()) {
      error_out = L"OAuth response missing access_token.";
      return false;
    }
    int expires_in = doc.value("expires_in", 0);
    FILETIME now;
    GetSystemTimeAsFileTime(&now);
    if (expires_in > 60) {
      expires_in -= 60;  // refresh a minute early
    }
    FILETIME expiry = AddSeconds(now, static_cast<uint32_t>(std::max(expires_in, 0)));
    token = WideFromUtf8(access_token);
    expires_utc_iso = FormatIso8601(expiry);
    error_out.clear();
    return true;
  } catch (const json::exception&) {
    error_out = L"Failed to parse OAuth response.";
    return false;
  }
}

std::optional<IgdbGame> IGDB::SearchOne(const std::wstring& client_id,
                                        const std::wstring& token,
                                        const std::wstring& name) {
  if (client_id.empty() || token.empty() || name.empty()) {
    return std::nullopt;
  }

  std::vector<std::pair<std::wstring, std::wstring>> headers = {
      {L"Client-ID", client_id},
      {L"Authorization", L"Bearer " + token},
      {L"Accept", L"application/json"},
      {L"Content-Type", L"text/plain"}};

  std::wstring query = L"fields name,summary,total_rating,cover.image_id,genres.name,platforms.name; search \"";
  query.append(EscapeQuery(name));
  query.append(L"\"; limit 1;");
  std::string body = Utf8FromWide(query);

  std::vector<uint8_t> response;
  DWORD status = 0;
  if (!HttpRequest(L"POST", L"https://api.igdb.com/v4/games", headers, body, response, status) ||
      status != HTTP_STATUS_OK) {
    return std::nullopt;
  }

  std::string text(response.begin(), response.end());
  try {
    json doc = json::parse(text);
    if (!doc.is_array() || doc.empty()) {
      return std::nullopt;
    }
    const json& entry = doc.front();
    IgdbGame game;
    game.name = WideFromUtf8(entry.value("name", std::string()));
    if (entry.contains("summary") && entry["summary"].is_string()) {
      game.summary = WideFromUtf8(entry["summary"].get<std::string>());
    }
    if (entry.contains("total_rating") && entry["total_rating"].is_number()) {
      game.totalRating = entry["total_rating"].get<double>();
    }
    if (entry.contains("cover") && entry["cover"].is_object()) {
      const json& cover = entry["cover"];
      if (cover.contains("image_id") && cover["image_id"].is_string()) {
        game.imageId = WideFromUtf8(cover["image_id"].get<std::string>());
        game.coverUrl = ImageUrlCoverBig(game.imageId);
      }
    }
    if (entry.contains("genres") && entry["genres"].is_array()) {
      for (const auto& genre : entry["genres"]) {
        if (genre.is_object()) {
          std::string name_utf8 = genre.value("name", std::string());
          if (!name_utf8.empty()) {
            game.genres.push_back(WideFromUtf8(name_utf8));
          }
        } else if (genre.is_string()) {
          game.genres.push_back(WideFromUtf8(genre.get<std::string>()));
        }
      }
    }
    if (entry.contains("platforms") && entry["platforms"].is_array()) {
      for (const auto& platform : entry["platforms"]) {
        if (platform.is_object()) {
          std::string name_utf8 = platform.value("name", std::string());
          if (!name_utf8.empty()) {
            game.platforms.push_back(WideFromUtf8(name_utf8));
          }
        } else if (platform.is_string()) {
          game.platforms.push_back(WideFromUtf8(platform.get<std::string>()));
        }
      }
    }
    return game;
  } catch (const json::exception&) {
    return std::nullopt;
  }
}

std::wstring IGDB::ImageUrlCoverBig(const std::wstring& image_id) {
  if (image_id.empty()) {
    return {};
  }
  return L"https://images.igdb.com/igdb/image/upload/t_cover_big/" + image_id + L".jpg";
}

std::wstring IGDB::CacheImage(const std::wstring& url) {
  if (url.empty()) {
    return {};
  }
  std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return {};
  }
  std::filesystem::path dir = std::filesystem::path(root) / L"cache" / L"igdb";
  Cache::EnsureDirectory(dir.wstring());

  uint64_t hash = HashString(url);
  wchar_t filename[32];
  swprintf(filename, _countof(filename), L"%016llx.jpg", static_cast<unsigned long long>(hash));
  std::filesystem::path path = dir / filename;
  std::error_code ec;
  if (std::filesystem::exists(path, ec)) {
    return path.wstring();
  }

  std::vector<uint8_t> response;
  DWORD status = 0;
  if (!HttpRequest(L"GET", url, {}, std::string(), response, status) || status != HTTP_STATUS_OK || response.empty()) {
    return {};
  }

  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return {};
  }
  DWORD written = 0;
  BOOL ok = WriteFile(file, response.data(), static_cast<DWORD>(response.size()), &written, nullptr);
  CloseHandle(file);
  if (!ok || written != response.size()) {
    DeleteFileW(path.c_str());
    return {};
  }
  return path.wstring();
}

}  // namespace optiscaler
