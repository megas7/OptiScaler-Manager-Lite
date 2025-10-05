#include "igdb.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "cache.h"
#include <nlohmann/json.hpp>

#pragma comment(lib, "winhttp.lib")

namespace optiscaler {
namespace {

using json = nlohmann::json;

#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 0x00000080
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 0x00000200
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0x00002000
#endif
#ifndef WINHTTP_DECOMPRESSION_FLAG_GZIP
#define WINHTTP_DECOMPRESSION_FLAG_GZIP 0x00000001
#endif
#ifndef WINHTTP_DECOMPRESSION_FLAG_DEFLATE
#define WINHTTP_DECOMPRESSION_FLAG_DEFLATE 0x00000002
#endif

std::string Utf8FromWide(const std::wstring& wide) {
  if (wide.empty()) {
    return {};
  }
  int count = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
  std::string utf8(static_cast<size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), count, nullptr, nullptr);
  return utf8;
}

std::wstring WideFromUtf8(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  std::wstring wide(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), count);
  return wide;
}

std::wstring PercentEncode(const std::wstring& input) {
  std::wstring encoded;
  for (wchar_t ch : input) {
    if ((ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || ch == L'-' || ch == L'_' ||
        ch == L'.' || ch == L'~') {
      encoded.push_back(ch);
    } else {
      wchar_t buffer[4];
      swprintf(buffer, _countof(buffer), L"%%%02X", static_cast<unsigned int>(static_cast<unsigned char>(ch)));
      encoded.append(buffer);
    }
  }
  return encoded;
}

std::wstring QuoteAndEscape(const std::wstring& value) {
  std::wstring escaped;
  escaped.reserve(value.size() + 2);
  escaped.push_back(L'"');
  for (wchar_t ch : value) {
    if (ch == L'"' || ch == L'\\') {
      escaped.push_back(L'\\');
    }
    escaped.push_back(ch);
  }
  escaped.push_back(L'"');
  return escaped;
}

class ScopedHInternet {
 public:
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

bool CrackUrl(const std::wstring& url, std::wstring& host, std::wstring& path, INTERNET_PORT& port, bool& secure) {
  URL_COMPONENTSW components = {};
  components.dwStructSize = sizeof(components);
  std::wstring copy = url;
  if (!WinHttpCrackUrl(copy.c_str(), static_cast<DWORD>(copy.size()), 0, &components)) {
    return false;
  }
  host.assign(components.lpszHostName, components.dwHostNameLength);
  path.assign(components.lpszUrlPath, components.dwUrlPathLength);
  if (components.dwExtraInfoLength > 0 && components.lpszExtraInfo) {
    path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
  }
  std::wstring scheme(components.lpszScheme, components.dwSchemeLength);
  secure = (scheme == L"https");
  port = components.nPort;
  return true;
}

bool PerformHttpRequest(const std::wstring& method,
                        const std::wstring& url,
                        const std::vector<std::pair<std::wstring, std::wstring>>& headers,
                        const std::string& body,
                        DWORD& status_out,
                        std::string& response_out) {
  std::wstring host;
  std::wstring path;
  INTERNET_PORT port = 0;
  bool secure = false;
  if (!CrackUrl(url, host, path, port, secure)) {
    return false;
  }

  ScopedHInternet session(WinHttpOpen(L"OptiScalerMgrLite/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0));
  if (!session) {
    return false;
  }

  DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |
                    WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
  protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
  WinHttpSetOption(session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
  WinHttpSetTimeouts(session.get(), 5000, 5000, 15000, 30000);

  ScopedHInternet connection(WinHttpConnect(session.get(), host.c_str(), port, 0));
  if (!connection) {
    return false;
  }

  ScopedHInternet request(
      WinHttpOpenRequest(connection.get(), method.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0));
  if (!request) {
    return false;
  }

  DWORD decompression = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
  WinHttpSetOption(request.get(), WINHTTP_OPTION_DECOMPRESSION, &decompression, sizeof(decompression));

  constexpr DWORD kHeaderFlags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE;
  for (const auto& header : headers) {
    std::wstring headerLine = header.first + L": " + header.second;
    WinHttpAddRequestHeaders(request.get(), headerLine.c_str(), static_cast<DWORD>(headerLine.size()), kHeaderFlags);
  }

  LPCVOID optional = body.empty() ? WINHTTP_NO_REQUEST_DATA : body.data();
  DWORD optionalLength = body.empty() ? 0 : static_cast<DWORD>(body.size());
  if (!WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, const_cast<LPVOID>(optional), optionalLength,
                          optionalLength, 0)) {
    return false;
  }
  if (!WinHttpReceiveResponse(request.get(), nullptr)) {
    return false;
  }

  DWORD statusCode = 0;
  DWORD statusSize = sizeof(statusCode);
  if (!WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &statusCode,
                           &statusSize, WINHTTP_NO_HEADER_INDEX)) {
    return false;
  }
  status_out = statusCode;

  response_out.clear();
  DWORD available = 0;
  do {
    if (!WinHttpQueryDataAvailable(request.get(), &available)) {
      break;
    }
    if (available == 0) {
      break;
    }
    std::string chunk(available, '\0');
    DWORD read = 0;
    if (!WinHttpReadData(request.get(), chunk.data(), available, &read)) {
      break;
    }
    chunk.resize(read);
    response_out.append(chunk);
  } while (available > 0);

  return true;
}

uint64_t HashWide(const std::wstring& value) {
  constexpr uint64_t kOffset = 1469598103934665603ull;
  constexpr uint64_t kPrime = 1099511628211ull;
  uint64_t hash = kOffset;
  for (wchar_t ch : value) {
    hash ^= static_cast<uint64_t>(towlower(ch));
    hash *= kPrime;
  }
  return hash;
}

bool WriteBinaryFile(const std::wstring& path, const std::string& data) {
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }
  DWORD written = 0;
  BOOL ok = TRUE;
  if (!data.empty()) {
    ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
  }
  CloseHandle(file);
  return ok == TRUE;
}

std::wstring FormatIso8601FromFileTime(const FILETIME& file_time) {
  SYSTEMTIME st = {};
  if (!FileTimeToSystemTime(&file_time, &st)) {
    return {};
  }
  wchar_t buffer[64];
  swprintf(buffer, _countof(buffer), L"%04u-%02u-%02uT%02u:%02u:%02uZ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
           st.wSecond);
  return buffer;
}

std::wstring FormatUnixDate(uint64_t seconds) {
  if (seconds == 0) {
    return {};
  }
  ULARGE_INTEGER li;
  li.QuadPart = seconds * 10000000ull;
  li.QuadPart += 116444736000000000ull;  // Unix epoch offset.
  FILETIME ft;
  ft.dwLowDateTime = li.LowPart;
  ft.dwHighDateTime = li.HighPart;
  SYSTEMTIME st = {};
  if (!FileTimeToSystemTime(&ft, &st)) {
    return {};
  }
  wchar_t buffer[32];
  swprintf(buffer, _countof(buffer), L"%04u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
  return buffer;
}

}  // namespace

bool IGDB::EnsureAccessToken(const std::wstring& client_id,
                             const std::wstring& client_secret,
                             std::wstring& token,
                             std::wstring& expires_utc_iso,
                             std::wstring& error_out) {
  token.clear();
  expires_utc_iso.clear();
  if (client_id.empty() || client_secret.empty()) {
    error_out = L"Client ID or secret not configured.";
    return false;
  }

  std::wstring url = L"https://id.twitch.tv/oauth2/token?client_id=" + PercentEncode(client_id) +
                     L"&client_secret=" + PercentEncode(client_secret) + L"&grant_type=client_credentials";
  DWORD status = 0;
  std::string response;
  std::vector<std::pair<std::wstring, std::wstring>> headers = {
      {L"Content-Type", L"application/x-www-form-urlencoded"},
      {L"Accept", L"application/json"},
  };
  if (!PerformHttpRequest(L"POST", url, headers, {}, status, response)) {
    error_out = L"Network request failed while contacting Twitch.";
    return false;
  }
  if (status != HTTP_STATUS_OK) {
    std::wstringstream ss;
    ss << L"Twitch token request returned HTTP " << status;
    error_out = ss.str();
    return false;
  }

  try {
    json doc = json::parse(response);
    std::string access = doc.value("access_token", "");
    int expires_in = doc.value("expires_in", 0);
    if (access.empty() || expires_in <= 0) {
      error_out = L"Token response missing required fields.";
      return false;
    }
    token = WideFromUtf8(access);
    FILETIME now = {};
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER li;
    li.LowPart = now.dwLowDateTime;
    li.HighPart = now.dwHighDateTime;
    const ULONGLONG slack = 60ull * 10000000ull;  // refresh 60s early
    li.QuadPart += (static_cast<ULONGLONG>(expires_in) * 10000000ull) - slack;
    FILETIME future = {};
    future.dwLowDateTime = li.LowPart;
    future.dwHighDateTime = li.HighPart;
    expires_utc_iso = FormatIso8601FromFileTime(future);
    error_out.clear();
    return true;
  } catch (const std::exception&) {
    error_out = L"Failed to parse Twitch token response.";
    return false;
  }
}

std::optional<IgdbGame> IGDB::SearchOne(const std::wstring& client_id,
                                        const std::wstring& token,
                                        const std::wstring& name) {
  if (client_id.empty() || token.empty() || name.empty()) {
    return std::nullopt;
  }

  std::wstring url = L"https://api.igdb.com/v4/games";
  DWORD status = 0;
  std::wstring query = L"fields name,summary,cover.image_id,platforms.name,genres.name,first_release_date,total_rating; "
                       L"search " + QuoteAndEscape(name) + L"; limit 1;";
  std::string body = Utf8FromWide(query);
  std::vector<std::pair<std::wstring, std::wstring>> headers = {
      {L"Client-ID", client_id},
      {L"Authorization", L"Bearer " + token},
      {L"Accept", L"application/json"},
      {L"Content-Type", L"application/json"},
  };
  std::string response;
  if (!PerformHttpRequest(L"POST", url, headers, body, status, response)) {
    return std::nullopt;
  }
  if (status == HTTP_STATUS_UNAUTHORIZED) {
    return std::nullopt;
  }
  if (status != HTTP_STATUS_OK) {
    return std::nullopt;
  }

  try {
    json doc = json::parse(response);
    if (!doc.is_array() || doc.empty()) {
      return std::nullopt;
    }
    const json& entry = doc.front();
    IgdbGame game;
    game.name = WideFromUtf8(entry.value("name", std::string()));
    game.summary = WideFromUtf8(entry.value("summary", std::string()));
    if (entry.contains("genres") && entry["genres"].is_array()) {
      for (const auto& genre : entry["genres"]) {
        if (genre.is_object()) {
          game.genres.push_back(WideFromUtf8(genre.value("name", std::string())));
        }
      }
    }
    if (entry.contains("platforms") && entry["platforms"].is_array()) {
      for (const auto& platform : entry["platforms"]) {
        if (platform.is_object()) {
          game.platforms.push_back(WideFromUtf8(platform.value("name", std::string())));
        }
      }
    }
    if (entry.contains("cover") && entry["cover"].is_object()) {
      game.coverImageId = WideFromUtf8(entry["cover"].value("image_id", std::string()));
    }
    if (entry.contains("first_release_date")) {
      uint64_t stamp = 0;
      if (entry["first_release_date"].is_number_unsigned()) {
        stamp = entry["first_release_date"].get<uint64_t>();
      } else if (entry["first_release_date"].is_number_integer()) {
        stamp = static_cast<uint64_t>(entry["first_release_date"].get<int64_t>());
      }
      game.releaseDate = FormatUnixDate(stamp);
    }
    if (entry.contains("total_rating") && entry["total_rating"].is_number()) {
      game.rating = entry["total_rating"].get<double>();
    }
    return game;
  } catch (const std::exception&) {
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
  std::filesystem::path dir = std::filesystem::path(root) / L"cache" / L"images";
  Cache::EnsureDirectory(dir.wstring());
  uint64_t hash = HashWide(url);
  std::wstring extension = L".jpg";
  size_t dot = url.find_last_of(L'.');
  size_t slash = url.find_last_of(L'/');
  if (dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash) && dot + 1 < url.size()) {
    std::wstring ext = url.substr(dot);
    if (ext.size() <= 8) {
      extension = ext;
    }
  }
  wchar_t filename[32];
  swprintf(filename, _countof(filename), L"%016llx%s", static_cast<unsigned long long>(hash), extension.c_str());
  std::filesystem::path path = dir / filename;
  if (std::filesystem::exists(path)) {
    return path.wstring();
  }

  DWORD status = 0;
  std::string response;
  if (!PerformHttpRequest(L"GET", url, {}, {}, status, response)) {
    return {};
  }
  if (status != HTTP_STATUS_OK || response.empty()) {
    return {};
  }
  if (!WriteBinaryFile(path.wstring(), response)) {
    return {};
  }
  return path.wstring();
}

}  // namespace optiscaler
