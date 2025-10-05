#include "compatibility.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>

#include "cache.h"

#include "third_party/json.hpp"

#pragma comment(lib, "winhttp.lib")

namespace optiscaler {
namespace {

using json = nlohmann::json;

std::wstring WideFromUtf8(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  if (count <= 0) {
    return {};
  }
  std::wstring wide(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), count);
  return wide;
}

std::string Utf8FromWide(const std::wstring& wide) {
  if (wide.empty()) {
    return {};
  }
  int count = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
  if (count <= 0) {
    return {};
  }
  std::string utf8(static_cast<size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), count, nullptr, nullptr);
  return utf8;
}

std::wstring Trim(const std::wstring& value) {
  const wchar_t* whitespace = L" \t\r\n";
  size_t begin = value.find_first_not_of(whitespace);
  if (begin == std::wstring::npos) {
    return L"";
  }
  size_t end = value.find_last_not_of(whitespace);
  return value.substr(begin, end - begin + 1);
}

std::wstring ToLower(const std::wstring& value) {
  std::wstring result = value;
  std::transform(result.begin(), result.end(), result.begin(), [](wchar_t ch) { return towlower(ch); });
  return result;
}

std::wstring SlugFromTitle(const std::wstring& title) {
  std::wstring slug;
  slug.reserve(title.size());
  for (wchar_t ch : title) {
    if (ch == L' ') {
      slug.push_back(L'-');
    } else if (std::iswalnum(ch) || ch == L'-' || ch == L'_') {
      slug.push_back(ch);
    }
  }
  return slug;
}

std::wstring Iso8601NowUtc() {
  SYSTEMTIME st = {};
  GetSystemTime(&st);
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
  FILETIME fileTime = {};
  if (!SystemTimeToFileTime(&st, &fileTime)) {
    return false;
  }
  *out = fileTime;
  return true;
}

struct HttpResponse {
  DWORD status = 0;
  std::wstring etag;
  std::wstring lastModified;
  std::string body;
};

std::wstring HeaderValue(HINTERNET request, DWORD infoLevel) {
  DWORD size = 0;
  WinHttpQueryHeaders(request, infoLevel, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &size, WINHTTP_NO_HEADER_INDEX);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
    return L"";
  }
  std::wstring value(size / sizeof(wchar_t), L'\0');
  if (!WinHttpQueryHeaders(request, infoLevel, WINHTTP_HEADER_NAME_BY_INDEX, value.data(), &size,
                           WINHTTP_NO_HEADER_INDEX)) {
    return L"";
  }
  if (!value.empty() && value.back() == L'\0') {
    value.pop_back();
  }
  return value;
}

bool PerformHttpGet(const std::wstring& url, const std::wstring& etag, const std::wstring& lastModified,
                    HttpResponse& response) {
  URL_COMPONENTSW comps = {};
  comps.dwStructSize = sizeof(comps);
  std::wstring urlCopy = url;
  if (!WinHttpCrackUrl(urlCopy.c_str(), static_cast<DWORD>(urlCopy.size()), 0, &comps)) {
    return false;
  }
  std::wstring host(comps.lpszHostName, comps.dwHostNameLength);
  std::wstring path(comps.lpszUrlPath, comps.dwUrlPathLength);
  std::wstring scheme(comps.lpszScheme, comps.dwSchemeLength);
  bool secure = (scheme == L"https");

  HINTERNET session = WinHttpOpen(L"OptiScalerMgrLite/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                                  WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) {
    return false;
  }
  WinHttpSetTimeouts(session, 5000, 5000, 15000, 30000);

  HINTERNET connection = WinHttpConnect(session, host.c_str(), comps.nPort, 0);
  if (!connection) {
    WinHttpCloseHandle(session);
    return false;
  }

  HINTERNET request = WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                                         WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
  if (!request) {
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return false;
  }

  if (!etag.empty()) {
    std::wstring header = L"If-None-Match: " + etag;
    WinHttpAddRequestHeaders(request, header.c_str(), static_cast<DWORD>(header.size()), WINHTTP_ADDREQ_FLAG_ADD);
  }
  if (!lastModified.empty()) {
    std::wstring header = L"If-Modified-Since: " + lastModified;
    WinHttpAddRequestHeaders(request, header.c_str(), static_cast<DWORD>(header.size()), WINHTTP_ADDREQ_FLAG_ADD);
  }

  bool ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (ok) {
    ok = WinHttpReceiveResponse(request, nullptr);
  }
  if (!ok) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return false;
  }

  DWORD statusCode = 0;
  DWORD statusSize = sizeof(statusCode);
  if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &statusCode, &statusSize,
                           WINHTTP_NO_HEADER_INDEX)) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return false;
  }
  response.status = statusCode;
  response.etag = HeaderValue(request, WINHTTP_QUERY_ETAG);
  response.lastModified = HeaderValue(request, WINHTTP_QUERY_LAST_MODIFIED);

  if (statusCode == HTTP_STATUS_NOT_MODIFIED) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return true;
  }

  std::string body;
  DWORD available = 0;
  do {
    if (!WinHttpQueryDataAvailable(request, &available)) {
      break;
    }
    if (available == 0) {
      break;
    }
    std::string chunk(available, '\0');
    DWORD read = 0;
    if (!WinHttpReadData(request, chunk.data(), available, &read)) {
      break;
    }
    chunk.resize(read);
    body.append(chunk);
  } while (available > 0);

  response.body = std::move(body);

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);
  return true;
}

bool HttpGetWithRetry(const std::wstring& url, const std::wstring& etag, const std::wstring& lastModified,
                      HttpResponse& response) {
  const int maxAttempts = 3;
  for (int attempt = 0; attempt < maxAttempts; ++attempt) {
    if (!PerformHttpGet(url, etag, lastModified, response)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1500 * (attempt + 1)));
      continue;
    }
    if (response.status == HTTP_STATUS_OK || response.status == HTTP_STATUS_NOT_MODIFIED) {
      return true;
    }
    if (response.status == HTTP_STATUS_DENIED || response.status == HTTP_STATUS_FORBIDDEN ||
        response.status == HTTP_STATUS_TOO_MANY_REQUESTS) {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000 * (attempt + 1)));
      continue;
    }
    return false;
  }
  return false;
}

struct IndexEntry {
  std::wstring title;
  std::wstring slug;
};

std::vector<std::wstring> SplitLines(const std::wstring& text) {
  std::vector<std::wstring> lines;
  std::wstring current;
  for (wchar_t ch : text) {
    if (ch == L'\r') {
      continue;
    }
    if (ch == L'\n') {
      lines.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  lines.push_back(current);
  return lines;
}

std::vector<IndexEntry> ParseIndex(const std::wstring& text) {
  std::vector<IndexEntry> entries;
  for (const auto& line : SplitLines(text)) {
    std::wstring trimmed = Trim(line);
    if (trimmed.empty() || trimmed[0] == L'#') {
      continue;
    }
    size_t linkPos = trimmed.find(L"[[");
    if (linkPos != std::wstring::npos) {
      size_t endPos = trimmed.find(L"]]", linkPos);
      if (endPos != std::wstring::npos) {
        std::wstring title = trimmed.substr(linkPos + 2, endPos - linkPos - 2);
        if (!title.empty() && ToLower(title) != L"template") {
          entries.push_back({title, SlugFromTitle(title)});
        }
        continue;
      }
    }
    size_t adocLink = trimmed.find(L"link:");
    if (adocLink != std::wstring::npos) {
      size_t bracket = trimmed.find(L'[', adocLink);
      size_t endBracket = trimmed.find(L']', bracket);
      if (bracket != std::wstring::npos && endBracket != std::wstring::npos && endBracket > bracket) {
        std::wstring target = trimmed.substr(adocLink + 5, bracket - (adocLink + 5));
        std::wstring title = trimmed.substr(bracket + 1, endBracket - bracket - 1);
        if (ToLower(title) != L"template" && !target.empty()) {
          size_t slash = target.find_last_of(L"/");
          std::wstring slug = (slash == std::wstring::npos) ? target : target.substr(slash + 1);
          entries.push_back({title, slug});
        }
      }
      continue;
    }
  }
  return entries;
}

std::vector<std::wstring> ParseBulletList(const std::wstring& text) {
  std::vector<std::wstring> items;
  for (const auto& line : SplitLines(text)) {
    std::wstring trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.rfind(L"* ", 0) == 0 || trimmed.rfind(L"- ", 0) == 0) {
      trimmed = Trim(trimmed.substr(2));
    }
    if (!trimmed.empty()) {
      items.push_back(trimmed);
    }
  }
  return items;
}

std::map<std::wstring, std::wstring> ParseTable(const std::wstring& body, std::vector<std::wstring>& warnings) {
  std::map<std::wstring, std::wstring> fields;
  std::wstring currentKey;
  for (const auto& line : SplitLines(body)) {
    std::wstring trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    if (trimmed.rfind(L"|===", 0) == 0) {
      continue;
    }
    if (trimmed.rfind(L"|", 0) == 0) {
      std::wstring payload = Trim(trimmed.substr(1));
      size_t bar = payload.find(L"|");
      if (bar != std::wstring::npos) {
        currentKey = Trim(payload.substr(0, bar));
        std::wstring value = Trim(payload.substr(bar + 1));
        if (!currentKey.empty()) {
          auto it = fields.find(currentKey);
          if (it == fields.end()) {
            fields.emplace(currentKey, value);
          } else {
            it->second.append(L"\n");
            it->second.append(value);
          }
        }
      } else if (!currentKey.empty()) {
        fields[currentKey].append(L"\n");
        fields[currentKey].append(payload);
      }
    } else if (!currentKey.empty()) {
      fields[currentKey].append(L"\n");
      fields[currentKey].append(trimmed);
    }
  }
  if (fields.empty()) {
    warnings.push_back(L"Table parse produced no fields.");
  }
  return fields;
}

std::map<std::wstring, std::wstring> ParseIniPairs(const std::wstring& text) {
  std::map<std::wstring, std::wstring> result;
  for (const auto& line : SplitLines(text)) {
    std::wstring trimmed = Trim(line);
    if (trimmed.empty()) {
      continue;
    }
    size_t equals = trimmed.find(L'=');
    if (equals == std::wstring::npos) {
      continue;
    }
    std::wstring key = Trim(trimmed.substr(0, equals));
    std::wstring value = Trim(trimmed.substr(equals + 1));
    if (!key.empty()) {
      result[key] = value;
    }
  }
  return result;
}

void ExtractInputs(const std::wstring& text, std::vector<std::wstring>& inputs, CompatNotes& notes) {
  for (const auto& line : SplitLines(text)) {
    std::wstring token = Trim(line);
    if (token.empty()) {
      continue;
    }
    if (token.find(L",") != std::wstring::npos) {
      std::wstringstream ss(token);
      std::wstring part;
      while (std::getline(ss, part, L',')) {
        std::wstring trimmed = Trim(part);
        if (trimmed.empty()) {
          continue;
        }
        std::wstring lower = ToLower(trimmed);
        bool notSupported = lower.find(L"not supported") != std::wstring::npos;
        if (notSupported) {
          notes.knownIssues.push_back(trimmed);
        }
        if (lower.find(L"dlss") != std::wstring::npos) {
          inputs.push_back(L"DLSS");
        } else if (lower.find(L"fsr3") != std::wstring::npos) {
          inputs.push_back(L"FSR3");
        } else if (lower.find(L"fsr2") != std::wstring::npos) {
          inputs.push_back(L"FSR2");
        } else if (lower.find(L"xess") != std::wstring::npos) {
          inputs.push_back(L"XeSS");
        }
      }
      continue;
    }
    std::wstring lower = ToLower(token);
    bool notSupported = lower.find(L"not supported") != std::wstring::npos;
    if (notSupported) {
      notes.knownIssues.push_back(token);
    }
    if (lower.find(L"dlss") != std::wstring::npos) {
      inputs.push_back(L"DLSS");
    } else if (lower.find(L"fsr3") != std::wstring::npos) {
      inputs.push_back(L"FSR3");
    } else if (lower.find(L"fsr2") != std::wstring::npos) {
      inputs.push_back(L"FSR2");
    } else if (lower.find(L"xess") != std::wstring::npos) {
      inputs.push_back(L"XeSS");
    }
  }
}

void ScanForKeywords(const std::wstring& text, CompatNotes& notes) {
  std::wstring lower = ToLower(text);
  if (lower.find(L"fakenvapi") != std::wstring::npos || lower.find(L"inputs hidden") != std::wstring::npos) {
    notes.requiresFakenvapi = true;
  }
  if (lower.find(L"overlay") != std::wstring::npos) {
    notes.overlaysToDisable.push_back(text);
  }
  if (lower.find(L"dlssg-to-fsr3") != std::wstring::npos || lower.find(L"frame generation") != std::wstring::npos) {
    notes.extraSteps.push_back(text);
  }
  if (lower.find(L"fg") != std::wstring::npos && lower.find(L"not") != std::wstring::npos &&
      lower.find(L"supported") != std::wstring::npos) {
    notes.optiFgSupported = false;
  }
}

void PopulateFromFields(const std::wstring& pageName, const std::map<std::wstring, std::wstring>& fields,
                        CompatibilityProfile& profile) {
  auto getField = [&](const std::wstring& key) -> std::wstring {
    auto it = fields.find(key);
    if (it != fields.end()) {
      return Trim(it->second);
    }
    return L"";
  };

  profile.pageName = pageName;
  std::wstring explicitName = getField(L"Game");
  profile.gameName = explicitName.empty() ? pageName : explicitName;
  std::wstring version = getField(L"Version");
  if (version.empty()) {
    version = getField(L"Tested Version");
  }
  profile.testedVersion = version;
  profile.os = getField(L"OS");
  profile.gpu = getField(L"GPU");
  profile.tags = ParseBulletList(getField(L"Tags"));
  profile.settings.requiredIni = ParseIniPairs(getField(L"Settings"));
  profile.settings.fgIni = ParseIniPairs(getField(L"FG-Settings"));
  ExtractInputs(getField(L"Inputs"), profile.inputs, profile.notes);
  std::vector<std::wstring> knownIssues = ParseBulletList(getField(L"Known Issues"));
  for (const auto& issue : knownIssues) {
    profile.notes.knownIssues.push_back(issue);
    ScanForKeywords(issue, profile.notes);
  }
  std::vector<std::wstring> notes = ParseBulletList(getField(L"Notes"));
  for (const auto& note : notes) {
    profile.notes.extraSteps.push_back(note);
    ScanForKeywords(note, profile.notes);
  }
  if (profile.notes.overlaysToDisable.empty()) {
    std::vector<std::wstring> overlays = ParseBulletList(getField(L"Overlays"));
    for (const auto& overlay : overlays) {
      profile.notes.overlaysToDisable.push_back(overlay);
    }
  }
  std::wstring filename = getField(L"Filename");
  if (!filename.empty()) {
    profile.dll = InstallDllFromFileName(filename);
  }
  profile.lastFetchedUtc = Iso8601NowUtc();
}

CompatibilityProfile ParseProfileFromBody(const std::wstring& pageName, const std::wstring& body,
                                          std::vector<std::wstring>& warnings) {
  CompatibilityProfile profile;
  profile.pageName = pageName;
  profile.gameName = pageName;
  auto fields = ParseTable(body, warnings);
  if (!fields.empty()) {
    PopulateFromFields(pageName, fields, profile);
  }
  if (profile.exeHints.empty()) {
    profile.exeHints.push_back(NormalizeExeName(profile.gameName));
  }
  return profile;
}

std::wstring ReadOverridesText() {
  std::wstring path = CompatibilityOverridesPath();
  std::wstring contents;
  if (Cache::ReadText(path, contents)) {
    return contents;
  }
  return L"";
}

}  // namespace

std::wstring CompatibilityCacheDirectory() {
  std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return L"";
  }
  std::filesystem::path dir = std::filesystem::path(root) / L"cache" / L"compat";
  Cache::EnsureDirectory(dir.wstring());
  return dir.wstring();
}

std::wstring CompatibilityCachePath() {
  std::filesystem::path path = std::filesystem::path(CompatibilityCacheDirectory()) / L"compat_profiles.json";
  return path.wstring();
}

std::wstring CompatibilityMetaPath() {
  std::filesystem::path path = std::filesystem::path(CompatibilityCacheDirectory()) / L"compat_profiles.meta.json";
  return path.wstring();
}

std::wstring CompatibilityOverridesPath() {
  std::filesystem::path path = std::filesystem::path(Cache::AppDataRoot()) / L"compat_overrides.json";
  return path.wstring();
}

std::wstring CompatibilityLogPath() {
  std::filesystem::path path = std::filesystem::path(Cache::AppDataRoot()) / L"logs" / L"compat_sync.log";
  Cache::EnsureDirectory(path.parent_path().wstring());
  return path.wstring();
}

std::wstring InstallDllToFileName(InstallDll dll) {
  switch (dll) {
    case InstallDll::kDxgi:
      return L"dxgi.dll";
    case InstallDll::kWinmm:
      return L"winmm.dll";
    case InstallDll::kDinput8:
      return L"dinput8.dll";
    default:
      return L"opti_scaler.dll";
  }
}

InstallDll InstallDllFromFileName(const std::wstring& name) {
  std::wstring lower = ToLower(name);
  if (lower.find(L"dxgi") != std::wstring::npos) {
    return InstallDll::kDxgi;
  }
  if (lower.find(L"winmm") != std::wstring::npos) {
    return InstallDll::kWinmm;
  }
  if (lower.find(L"dinput8") != std::wstring::npos) {
    return InstallDll::kDinput8;
  }
  return InstallDll::kOther;
}

std::wstring NormalizeToken(const std::wstring& value) {
  std::wstring lower = ToLower(value);
  std::wstring normalized;
  for (wchar_t ch : lower) {
    if (std::iswalnum(ch)) {
      normalized.push_back(ch);
    }
  }
  return normalized;
}

std::wstring NormalizeExeName(const std::wstring& path) {
  size_t slash = path.find_last_of(L"\\/");
  std::wstring name = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
  return NormalizeToken(name);
}

bool LoadCompatibilityCache(CompatibilityCache& cache) {
  cache.profiles.clear();
  cache.meta = {};
  std::wstring path = CompatibilityCachePath();
  std::wstring jsonText;
  if (!Cache::ReadText(path, jsonText) || jsonText.empty()) {
    return false;
  }
  try {
    json doc = json::parse(Utf8FromWide(jsonText));
    if (doc.contains("profiles")) {
      for (const auto& item : doc["profiles"]) {
        CompatibilityProfile profile;
        profile.pageName = WideFromUtf8(item.value("pageName", ""));
        profile.gameName = WideFromUtf8(item.value("gameName", ""));
        if (item.contains("steamAppId") && !item["steamAppId"].is_null()) {
          profile.steamAppId = item["steamAppId"].get<uint32_t>();
        }
        profile.dll = InstallDllFromFileName(WideFromUtf8(item.value("dll", "dxgi.dll")));
        for (const auto& input : item.value("inputs", std::vector<std::string>{})) {
          profile.inputs.push_back(WideFromUtf8(input));
        }
        for (const auto& exe : item.value("exeHints", std::vector<std::string>{})) {
          profile.exeHints.push_back(WideFromUtf8(exe));
        }
        for (const auto& folder : item.value("folderHints", std::vector<std::string>{})) {
          profile.folderHints.push_back(WideFromUtf8(folder));
        }
        profile.testedVersion = WideFromUtf8(item.value("testedVersion", ""));
        profile.os = WideFromUtf8(item.value("os", ""));
        profile.gpu = WideFromUtf8(item.value("gpu", ""));
        if (item.contains("settings")) {
          const auto& settings = item["settings"];
          for (const auto& kv : settings.value("requiredIni", json::object())) {
            profile.settings.requiredIni[WideFromUtf8(kv.first)] = WideFromUtf8(kv.second.get<std::string>());
          }
          for (const auto& kv : settings.value("fgIni", json::object())) {
            profile.settings.fgIni[WideFromUtf8(kv.first)] = WideFromUtf8(kv.second.get<std::string>());
          }
        }
        if (item.contains("notes")) {
          const auto& notes = item["notes"];
          profile.notes.requiresFakenvapi = notes.value("requiresFakenvapi", false);
          profile.notes.optiFgSupported = notes.value("optiFgSupported", true);
          for (const auto& overlay : notes.value("overlaysToDisable", std::vector<std::string>{})) {
            profile.notes.overlaysToDisable.push_back(WideFromUtf8(overlay));
          }
          for (const auto& issue : notes.value("knownIssues", std::vector<std::string>{})) {
            profile.notes.knownIssues.push_back(WideFromUtf8(issue));
          }
          for (const auto& step : notes.value("extraSteps", std::vector<std::string>{})) {
            profile.notes.extraSteps.push_back(WideFromUtf8(step));
          }
        }
        for (const auto& tag : item.value("tags", std::vector<std::string>{})) {
          profile.tags.push_back(WideFromUtf8(tag));
        }
        profile.lastFetchedUtc = WideFromUtf8(item.value("lastFetchedUtc", ""));
        for (const auto& warn : item.value("parseWarnings", std::vector<std::string>{})) {
          profile.parseWarnings.push_back(WideFromUtf8(warn));
        }
    cache.profiles.push_back(std::move(profile));
  }
}
    cache.meta.fetchedUtc = WideFromUtf8(doc.value("fetchedUtc", ""));
    cache.meta.sourceUrl = WideFromUtf8(doc.value("sourceUrl", ""));
    cache.meta.etag = WideFromUtf8(doc.value("etag", ""));
    cache.meta.lastModified = WideFromUtf8(doc.value("lastModified", ""));
    cache.meta.count = cache.profiles.size();
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool SaveCompatibilityCache(const CompatibilityCache& cache) {
  json doc;
  doc["fetchedUtc"] = Utf8FromWide(cache.meta.fetchedUtc);
  doc["sourceUrl"] = Utf8FromWide(cache.meta.sourceUrl);
  doc["etag"] = Utf8FromWide(cache.meta.etag);
  doc["lastModified"] = Utf8FromWide(cache.meta.lastModified);
  json profiles = json::array();
  for (const auto& profile : cache.profiles) {
    json item;
    item["pageName"] = Utf8FromWide(profile.pageName);
    item["gameName"] = Utf8FromWide(profile.gameName);
    if (profile.steamAppId.has_value()) {
      item["steamAppId"] = profile.steamAppId.value();
    } else {
      item["steamAppId"] = nullptr;
    }
    item["dll"] = Utf8FromWide(InstallDllToFileName(profile.dll));
    json inputs = json::array();
    for (const auto& input : profile.inputs) {
      inputs.push_back(Utf8FromWide(input));
    }
    item["inputs"] = std::move(inputs);
    json exeHints = json::array();
    for (const auto& exe : profile.exeHints) {
      exeHints.push_back(Utf8FromWide(exe));
    }
    item["exeHints"] = std::move(exeHints);
    json folderHints = json::array();
    for (const auto& folder : profile.folderHints) {
      folderHints.push_back(Utf8FromWide(folder));
    }
    item["folderHints"] = std::move(folderHints);
    item["testedVersion"] = Utf8FromWide(profile.testedVersion);
    item["os"] = Utf8FromWide(profile.os);
    item["gpu"] = Utf8FromWide(profile.gpu);
    json settings;
    json requiredIni = json::object();
    for (const auto& kv : profile.settings.requiredIni) {
      requiredIni[Utf8FromWide(kv.first)] = Utf8FromWide(kv.second);
    }
    json fgIni = json::object();
    for (const auto& kv : profile.settings.fgIni) {
      fgIni[Utf8FromWide(kv.first)] = Utf8FromWide(kv.second);
    }
    settings["requiredIni"] = std::move(requiredIni);
    settings["fgIni"] = std::move(fgIni);
    item["settings"] = std::move(settings);
    json notes;
    notes["requiresFakenvapi"] = profile.notes.requiresFakenvapi;
    notes["optiFgSupported"] = profile.notes.optiFgSupported;
    json overlays = json::array();
    for (const auto& overlay : profile.notes.overlaysToDisable) {
      overlays.push_back(Utf8FromWide(overlay));
    }
    notes["overlaysToDisable"] = std::move(overlays);
    json issues = json::array();
    for (const auto& issue : profile.notes.knownIssues) {
      issues.push_back(Utf8FromWide(issue));
    }
    notes["knownIssues"] = std::move(issues);
    json steps = json::array();
    for (const auto& step : profile.notes.extraSteps) {
      steps.push_back(Utf8FromWide(step));
    }
    notes["extraSteps"] = std::move(steps);
    item["notes"] = std::move(notes);
    json tags = json::array();
    for (const auto& tag : profile.tags) {
      tags.push_back(Utf8FromWide(tag));
    }
    item["tags"] = std::move(tags);
    item["lastFetchedUtc"] = Utf8FromWide(profile.lastFetchedUtc);
    json warnings = json::array();
    for (const auto& warn : profile.parseWarnings) {
      warnings.push_back(Utf8FromWide(warn));
    }
    item["parseWarnings"] = std::move(warnings);
    profiles.push_back(std::move(item));
  }
  doc["profiles"] = std::move(profiles);
  std::wstring text = WideFromUtf8(doc.dump(2));
  if (!Cache::WriteText(CompatibilityCachePath(), text)) {
    return false;
  }

  json meta;
  meta["fetchedUtc"] = Utf8FromWide(cache.meta.fetchedUtc);
  meta["sourceUrl"] = Utf8FromWide(cache.meta.sourceUrl);
  meta["etag"] = Utf8FromWide(cache.meta.etag);
  meta["lastModified"] = Utf8FromWide(cache.meta.lastModified);
  meta["count"] = cache.meta.count;
  return Cache::WriteText(CompatibilityMetaPath(), WideFromUtf8(meta.dump(2)));
}

bool LoadCompatibilityOverrides(std::vector<CompatibilityProfile>& overrides) {
  overrides.clear();
  std::wstring text = ReadOverridesText();
  if (text.empty()) {
    return false;
  }
  try {
    json doc = json::parse(Utf8FromWide(text));
    if (!doc.is_array()) {
      return false;
    }
    for (const auto& item : doc) {
      CompatibilityProfile profile;
      profile.pageName = WideFromUtf8(item.value("pageName", ""));
      profile.gameName = WideFromUtf8(item.value("gameName", ""));
      if (item.contains("steamAppId") && !item["steamAppId"].is_null()) {
        profile.steamAppId = item["steamAppId"].get<uint32_t>();
      }
      profile.dll = InstallDllFromFileName(WideFromUtf8(item.value("dll", "dxgi.dll")));
      for (const auto& exe : item.value("exeHints", std::vector<std::string>{})) {
        profile.exeHints.push_back(WideFromUtf8(exe));
      }
      for (const auto& input : item.value("inputs", std::vector<std::string>{})) {
        profile.inputs.push_back(WideFromUtf8(input));
      }
      overrides.push_back(std::move(profile));
    }
    return !overrides.empty();
  } catch (const std::exception&) {
    return false;
  }
}

void MergeOverrides(std::vector<CompatibilityProfile>& baseProfiles,
                    const std::vector<CompatibilityProfile>& overrides,
                    std::vector<std::wstring>& logLines) {
  std::unordered_map<std::wstring, size_t> indexByName;
  for (size_t i = 0; i < baseProfiles.size(); ++i) {
    indexByName[NormalizeToken(baseProfiles[i].gameName)] = i;
  }
  for (const auto& override : overrides) {
    std::wstring key = NormalizeToken(override.gameName);
    auto it = indexByName.find(key);
    if (it != indexByName.end()) {
      baseProfiles[it->second] = override;
      logLines.push_back(L"Override applied for " + override.gameName);
    } else {
      baseProfiles.push_back(override);
      logLines.push_back(L"Override appended for " + override.gameName);
    }
  }
}

bool RefreshCompatibilityFromNetwork(const std::wstring& indexUrl, CompatibilityCache& cache,
                                     std::vector<std::wstring>& logLines, std::wstring& statusMessage,
                                     bool allowCachedHeaders) {
  HttpResponse indexResponse;
  std::wstring etag = allowCachedHeaders ? cache.meta.etag : L"";
  std::wstring lastModified = allowCachedHeaders ? cache.meta.lastModified : L"";
  if (!HttpGetWithRetry(indexUrl, etag, lastModified, indexResponse)) {
    statusMessage = L"Failed to download compatibility index.";
    return false;
  }
  if (indexResponse.status == HTTP_STATUS_NOT_MODIFIED) {
    statusMessage = L"Compatibility index unchanged.";
    return true;
  }

  std::wstring body = WideFromUtf8(indexResponse.body);
  if (body.empty()) {
    statusMessage = L"Compatibility index download failed.";
    return false;
  }

  auto entries = ParseIndex(body);
  if (entries.empty()) {
    statusMessage = L"No entries discovered in compatibility index.";
    return false;
  }

  std::vector<CompatibilityProfile> profiles;
  for (const auto& entry : entries) {
    std::wstring slug = entry.slug;
    if (slug.empty()) {
      continue;
    }
    std::wstring baseUrl = L"https://raw.githubusercontent.com/wiki/optiscaler/OptiScaler/" + slug;
    std::vector<std::wstring> candidates = {baseUrl + L".adoc", baseUrl + L".md",
                                            L"https://github.com/optiscaler/OptiScaler/wiki/" + slug};
    std::wstring pageBody;
    std::vector<std::wstring> warnings;
    for (size_t i = 0; i < candidates.size(); ++i) {
      HttpResponse resp;
      if (!HttpGetWithRetry(candidates[i], L"", L"", resp)) {
        continue;
      }
      if (resp.status != HTTP_STATUS_OK) {
        continue;
      }
      pageBody = WideFromUtf8(resp.body);
      if (!pageBody.empty()) {
        break;
      }
    }
    if (pageBody.empty()) {
      logLines.push_back(L"Failed to download compatibility entry for " + entry.title);
      continue;
    }
    CompatibilityProfile profile = ParseProfileFromBody(entry.title, pageBody, warnings);
    if (!warnings.empty()) {
      profile.parseWarnings = warnings;
      for (const auto& warn : warnings) {
        logLines.push_back(L"Parse warning for " + entry.title + L": " + warn);
      }
    }
    profile.lastFetchedUtc = Iso8601NowUtc();
    profiles.push_back(std::move(profile));
  }

  cache.profiles = std::move(profiles);
  cache.meta.fetchedUtc = Iso8601NowUtc();
  cache.meta.sourceUrl = indexUrl;
  cache.meta.etag = indexResponse.etag;
  cache.meta.lastModified = indexResponse.lastModified;
  cache.meta.count = cache.profiles.size();
  statusMessage = L"Fetched " + std::to_wstring(cache.meta.count) + L" compatibility profiles.";
  return cache.meta.count > 0;
}

const CompatibilityProfile* MatchCompatibilityProfile(const CompatibilityCache& cache,
                                                      const std::wstring& exePath,
                                                      const std::wstring& gameName,
                                                      std::optional<uint32_t> steamAppId) {
  std::wstring normalizedExe = NormalizeExeName(exePath);
  std::wstring normalizedName = NormalizeToken(gameName);
  for (const auto& profile : cache.profiles) {
    if (steamAppId.has_value() && profile.steamAppId.has_value() &&
        steamAppId.value() == profile.steamAppId.value()) {
      return &profile;
    }
    for (const auto& hint : profile.exeHints) {
      if (!hint.empty() && hint == normalizedExe) {
        return &profile;
      }
    }
    if (!normalizedName.empty() && NormalizeToken(profile.gameName) == normalizedName) {
      return &profile;
    }
  }
  return nullptr;
}

void LogCompatibilityEvent(const std::vector<std::wstring>& lines) {
  std::wstring path = CompatibilityLogPath();
  HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return;
  }
  SetFilePointer(file, 0, nullptr, FILE_END);
  for (const auto& line : lines) {
    std::wstring entry = line + L"\r\n";
    std::string utf8 = Utf8FromWide(entry);
    DWORD written = 0;
    WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
  }
  CloseHandle(file);
}

bool LoadCompatibilityMeta(CompatibilityCacheMeta& meta) {
  meta = {};
  std::wstring text;
  if (!Cache::ReadText(CompatibilityMetaPath(), text) || text.empty()) {
    return false;
  }
  try {
    json doc = json::parse(Utf8FromWide(text));
    meta.fetchedUtc = WideFromUtf8(doc.value("fetchedUtc", ""));
    meta.sourceUrl = WideFromUtf8(doc.value("sourceUrl", ""));
    meta.etag = WideFromUtf8(doc.value("etag", ""));
    meta.lastModified = WideFromUtf8(doc.value("lastModified", ""));
    meta.count = doc.value("count", static_cast<size_t>(0));
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool SaveCompatibilityMeta(const CompatibilityCacheMeta& meta) {
  json doc;
  doc["fetchedUtc"] = Utf8FromWide(meta.fetchedUtc);
  doc["sourceUrl"] = Utf8FromWide(meta.sourceUrl);
  doc["etag"] = Utf8FromWide(meta.etag);
  doc["lastModified"] = Utf8FromWide(meta.lastModified);
  doc["count"] = meta.count;
  return Cache::WriteText(CompatibilityMetaPath(), WideFromUtf8(doc.dump(2)));
}

bool ShouldPromptCompatibilityRefresh(const CompatibilityCacheMeta& meta, int maxAgeDays) {
  if (meta.fetchedUtc.empty()) {
    return true;
  }
  FILETIME fetched = {};
  if (!ParseIso8601(meta.fetchedUtc, &fetched)) {
    return true;
  }
  SYSTEMTIME nowSystem = {};
  GetSystemTime(&nowSystem);
  FILETIME nowFile = {};
  if (!SystemTimeToFileTime(&nowSystem, &nowFile)) {
    return true;
  }
  ULARGE_INTEGER now64;
  now64.LowPart = nowFile.dwLowDateTime;
  now64.HighPart = nowFile.dwHighDateTime;
  ULARGE_INTEGER fetched64;
  fetched64.LowPart = fetched.dwLowDateTime;
  fetched64.HighPart = fetched.dwHighDateTime;
  const ULONGLONG maxDelta = static_cast<ULONGLONG>(maxAgeDays) * 24ull * 60ull * 60ull * 10000000ull;
  return (now64.QuadPart - fetched64.QuadPart) > maxDelta;
}

}  // namespace optiscaler

