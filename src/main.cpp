#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <thread>
#include <vector>

#include <shlobj.h>

#include "compatibility.h"
#include "cover_cache.h"
#include "gameconfig.h"
#include "game_types.h"
#include "igdb.h"
#include "launcher.h"
#include "localmeta.h"
#include "optiscaler.h"
#include "renderer_factory.h"
#include "resource.h"
#include "scanner.h"
#include "settings.h"
#include "systeminfo.h"

#pragma comment(lib, "comctl32.lib")

namespace optiscaler {

constexpr wchar_t kWindowClass[] = L"OptiScalerMgrLiteWindow";
constexpr int kTileWidth = 200;
constexpr int kTileHeight = 300;
constexpr int kTileSpacing = 24;
constexpr int kGridMargin = 16;
constexpr int kDetailsWidth = 360;
constexpr wchar_t kCompatibilityIndexUrl[] =
    L"https://raw.githubusercontent.com/wiki/optiscaler/OptiScaler/Compatibility-List.md";
constexpr int kCompatibilityRefreshDays = 7;
constexpr UINT WM_APP_COMPAT_REFRESHED = WM_APP + 21;
constexpr UINT WM_APP_IGDB_RESULT = WM_APP + 22;

struct AppState {
  std::vector<GameEntry> games;
  size_t selected_index = 0;
  std::unique_ptr<IRenderer> renderer;
  HWND status_bar = nullptr;
  SettingsData settings;
  GameConfig game_config;
  OptiScalerManager opti_manager;
  SystemInfoData system_info;
  std::vector<RECT> tile_bounds;
  int grid_columns = 1;
  bool require_rescan_after_settings = false;
  CompatibilityCache compat_cache;
  bool compat_refresh_in_progress = false;
  bool compat_loaded = false;
  std::wstring compat_status;
  bool igdb_fetch_in_progress = false;
  std::wstring igdb_status;
};

struct CompatRefreshResult {
  bool success = false;
  std::wstring status;
  std::vector<std::wstring> logLines;
  CompatibilityCache cache;
};

struct IgdbFetchResult {
  size_t gameIndex = static_cast<size_t>(-1);
  bool success = false;
  bool final = false;
  bool coverUpdated = false;
  bool tokenUpdated = false;
  std::wstring status;
  IgdbGame gameData;
  std::wstring coverPath;
  std::wstring token;
  std::wstring tokenExpiry;
};

void PerformScan(HWND hwnd, AppState* state);
void EnsureCompatibilityLoaded(AppState* state);
void PopulateCompatibilityForGame(AppState* state, GameEntry& game, const CompatibilityProfile* profile);
std::vector<std::wstring> BuildCompatibilityPlan(const CompatibilityProfile* profile, const SettingsData& settings,
                                                 const std::wstring& gameFolder);
void BeginCompatibilityRefresh(HWND hwnd, AppState* state, bool force_refresh);
void HandleCompatibilityRefreshed(HWND hwnd, AppState* state, LPARAM lparam);
void BeginIgdbPrefetch(HWND hwnd, AppState* state);
void HandleIgdbResult(HWND hwnd, AppState* state, LPARAM lparam);

std::wstring Trim(const std::wstring& value) {
  const wchar_t* whitespace = L" \t\r\n";
  size_t begin = value.find_first_not_of(whitespace);
  if (begin == std::wstring::npos) {
    return L"";
  }
  size_t end = value.find_last_not_of(whitespace);
  return value.substr(begin, end - begin + 1);
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
  std::vector<std::wstring> lines;
  std::wstring current;
  for (wchar_t ch : text) {
    if (ch == L'\r') {
      continue;
    }
    if (ch == L'\n') {
      if (!current.empty()) {
        lines.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    lines.push_back(current);
  }
  return lines;
}

std::wstring JoinLines(const std::vector<std::wstring>& lines) {
  std::wstring joined;
  for (const auto& line : lines) {
    if (line.empty()) {
      continue;
    }
    if (!joined.empty()) {
      joined.append(L"\r\n");
    }
    joined.append(line);
  }
  return joined;
}

std::wstring JoinWithComma(const std::vector<std::wstring>& values) {
  std::wstring joined;
  for (const auto& value : values) {
    if (value.empty()) {
      continue;
    }
    if (!joined.empty()) {
      joined.append(L", ");
    }
    joined.append(value);
  }
  return joined;
}

bool ParseIso8601Utc(const std::wstring& iso, FILETIME* out) {
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
  FILETIME ft = {};
  if (!SystemTimeToFileTime(&st, &ft)) {
    return false;
  }
  *out = ft;
  return true;
}

bool IsIsoExpired(const std::wstring& iso) {
  FILETIME expiry = {};
  if (!ParseIso8601Utc(iso, &expiry)) {
    return true;
  }
  FILETIME now = {};
  GetSystemTimeAsFileTime(&now);
  ULARGE_INTEGER now64;
  now64.LowPart = now.dwLowDateTime;
  now64.HighPart = now.dwHighDateTime;
  ULARGE_INTEGER exp64;
  exp64.LowPart = expiry.dwLowDateTime;
  exp64.HighPart = expiry.dwHighDateTime;
  return now64.QuadPart >= exp64.QuadPart;
}

std::wstring BuildSupportSummary(const GameEntry& game) {
  const std::wstring source_lower = game.source;
  if (source_lower == L"steam") {
    return L"Official support: Steam overlay, cloud saves, controller remapping.";
  }
  if (source_lower == L"epic") {
    return L"Official support: Epic Online Services launcher integration.";
  }
  if (source_lower == L"xbox") {
    return L"Official support: Microsoft Store / Xbox services.";
  }
  return L"Official support: Custom installation detected.";
}

void FreeGameAssets(std::vector<GameEntry>* games) {
  if (!games) {
    return;
  }
  for (auto& game : *games) {
    if (game.coverBmp) {
      DeleteObject(game.coverBmp);
      game.coverBmp = nullptr;
    }
  }
}

void DrawMultiline(IRenderer* renderer, const std::wstring& text, int x, int y, COLORREF color, int line_height = 16) {
  if (!renderer) {
    return;
  }
  size_t start = 0;
  size_t end = 0;
  int offset = 0;
  while (start < text.size()) {
    end = text.find(L'\n', start);
    std::wstring line = text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
    if (!line.empty()) {
      renderer->DrawText(line, x, y + offset, color);
    }
    if (end == std::wstring::npos) {
      break;
    }
    start = end + 1;
    offset += line_height;
  }
}

void UpdateStatusBar(AppState* state, const std::wstring& text);

void EnsureCompatibilityLoaded(AppState* state) {
  if (!state || state->compat_loaded) {
    return;
  }

  CompatibilityCache cache;
  if (LoadCompatibilityCache(cache)) {
    state->compat_cache = std::move(cache);
  } else {
    state->compat_cache.profiles.clear();
    state->compat_cache.meta = {};
  }

  CompatibilityCacheMeta meta;
  if (LoadCompatibilityMeta(meta)) {
    state->compat_cache.meta = meta;
    if (ShouldPromptCompatibilityRefresh(meta, kCompatibilityRefreshDays)) {
      state->compat_status =
          L"Compatibility data is older than a week. Use Tools → Refresh Compatibility List to update.";
    } else if (!meta.fetchedUtc.empty()) {
      state->compat_status = L"Compatibility data last fetched " + meta.fetchedUtc;
    }
  } else if (state->compat_cache.profiles.empty()) {
    state->compat_status = L"No compatibility data yet. Use Tools → Refresh Compatibility List.";
  }

  std::vector<CompatibilityProfile> overrides;
  std::vector<std::wstring> overrideLogs;
  if (LoadCompatibilityOverrides(overrides)) {
    MergeOverrides(state->compat_cache.profiles, overrides, overrideLogs);
    state->compat_cache.meta.count = state->compat_cache.profiles.size();
    if (!overrideLogs.empty()) {
      LogCompatibilityEvent(overrideLogs);
    }
  }

  state->compat_cache.meta.count = state->compat_cache.profiles.size();
  state->compat_loaded = true;
}

std::vector<std::wstring> BuildCompatibilityPlan(const CompatibilityProfile* profile, const SettingsData& settings,
                                                 const std::wstring& gameFolder) {
  std::vector<std::wstring> plan;
  if (!profile) {
    return plan;
  }

  std::wstring dllFile = InstallDllToFileName(profile->dll);
  if (!dllFile.empty()) {
    std::wstring source = settings.optiScalerInstallDir;
    if (!source.empty() && source.back() != L'\\' && source.back() != L'/') {
      source.push_back(L'\\');
    }
    std::wstring destination = gameFolder;
    if (!destination.empty() && destination.back() != L'\\' && destination.back() != L'/') {
      destination.push_back(L'\\');
    }
    std::wstring line = L"Copy ";
    if (!source.empty()) {
      line.append(source).append(dllFile);
    } else {
      line.append(dllFile);
    }
    line.append(L" -> ");
    if (!destination.empty()) {
      line.append(destination);
    }
    line.append(dllFile);
    plan.push_back(line);
  }

  if (!profile->settings.requiredIni.empty()) {
    plan.push_back(L"Update OptiScaler.ini with required settings");
  }
  if (!profile->settings.fgIni.empty()) {
    plan.push_back(L"Apply FG settings in OptiScaler.ini");
  }
  return plan;
}

void PopulateCompatibilityForGame(AppState* state, GameEntry& game, const CompatibilityProfile* profile) {
  game.compatProfileIndex = -1;
  game.compatBadges.clear();
  game.compatKnownIssues.clear();
  game.compatNotes.clear();
  game.compatInputs.clear();
  game.compatDll.clear();
  game.compatTestedVersion.clear();
  game.compatRequiresFakenvapi = false;
  game.compatFgSupported = true;

  if (!state || !profile) {
    return;
  }

  if (!state->compat_cache.profiles.empty()) {
    const CompatibilityProfile* base = state->compat_cache.profiles.data();
    game.compatProfileIndex = static_cast<int>(profile - base);
  } else {
    game.compatProfileIndex = 0;
  }

  game.compatInputs = profile->inputs;
  game.compatDll = InstallDllToFileName(profile->dll);
  game.compatTestedVersion = profile->testedVersion;
  game.compatRequiresFakenvapi = profile->notes.requiresFakenvapi;
  game.compatFgSupported = profile->notes.optiFgSupported;
  game.compatKnownIssues = profile->notes.knownIssues;
  game.compatNotes = profile->notes.extraSteps;
  for (const auto& overlay : profile->notes.overlaysToDisable) {
    game.compatNotes.push_back(L"Disable overlay: " + overlay);
  }
  for (const auto& kv : profile->settings.requiredIni) {
    game.compatNotes.push_back(L"OptiScaler.ini: " + kv.first + L"=" + kv.second);
  }
  for (const auto& kv : profile->settings.fgIni) {
    game.compatNotes.push_back(L"FG ini: " + kv.first + L"=" + kv.second);
  }
  for (const auto& warning : profile->parseWarnings) {
    game.compatNotes.push_back(L"Parser warning: " + warning);
  }

  if (!game.compatDll.empty()) {
    game.compatBadges.push_back(L"DLL: " + game.compatDll);
  }
  if (!game.compatInputs.empty()) {
    std::wstring inputLine = L"Inputs: ";
    for (size_t i = 0; i < game.compatInputs.size(); ++i) {
      if (i > 0) {
        inputLine.append(L", ");
      }
      inputLine.append(game.compatInputs[i]);
    }
    game.compatBadges.push_back(inputLine);
  }
  if (!profile->notes.optiFgSupported) {
    game.compatBadges.push_back(L"FG: Unsupported");
  } else if (!profile->settings.fgIni.empty()) {
    game.compatBadges.push_back(L"FG: Needs custom INI");
  } else {
    game.compatBadges.push_back(L"FG: Supported");
  }
  if (profile->notes.requiresFakenvapi) {
    game.compatBadges.push_back(L"Needs Fakenvapi");
  }
  for (const auto& tag : profile->tags) {
    if (!tag.empty()) {
      game.compatBadges.push_back(tag);
    }
  }
}

void ApplyConfigToGames(AppState* state) {
  if (!state) {
    return;
  }
  EnsureCompatibilityLoaded(state);
  for (auto& game : state->games) {
    const CompatibilityProfile* profile =
        MatchCompatibilityProfile(state->compat_cache, game.exe, game.name, game.steamAppId);
    PopulateCompatibilityForGame(state, game, profile);
    GameInjectionSettings settings = state->game_config.GetSettingsFor(game.exe);
    if (settings.files.empty()) {
      std::vector<std::wstring> compatPlan = BuildCompatibilityPlan(profile, state->settings, game.folder);
      if (!compatPlan.empty()) {
        settings.files = compatPlan;
      } else {
        settings.files = state->settings.defaultInjectionFiles;
      }
    }
    game.injectEnabled = settings.enabled;
    game.plannedFiles = settings.files;
    game.supportSummary = BuildSupportSummary(game);
    if (!game.coverBmp) {
      HBITMAP cached = CoverCache::LoadForExe(game.exe, kTileWidth, kTileHeight);
      if (cached) {
        game.coverBmp = cached;
      }
    }
    if (!game.coverBmp) {
      game.coverBmp = LocalMeta::IconAsCover(game.exe, kTileWidth, kTileHeight);
    }
    state->game_config.SetSettingsFor(game.exe, settings);
  }
}

void BeginCompatibilityRefresh(HWND hwnd, AppState* state, bool force_refresh) {
  if (!state || state->compat_refresh_in_progress) {
    return;
  }
  EnsureCompatibilityLoaded(state);
  state->compat_refresh_in_progress = true;
  state->compat_status = L"Refreshing compatibility list...";
  UpdateStatusBar(state, state->compat_status);
  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }

  CompatibilityCache cache_copy = state->compat_cache;
  std::thread([hwnd, state, force_refresh, cache_copy]() mutable {
    auto result = std::make_unique<CompatRefreshResult>();
    result->cache = std::move(cache_copy);
    std::vector<std::wstring> logLines;
    std::wstring status;
    bool fetched = RefreshCompatibilityFromNetwork(kCompatibilityIndexUrl, result->cache, logLines, status,
                                                   !force_refresh);
    bool hasNewData = fetched && status.find(L"unchanged") == std::wstring::npos;
    if (!fetched) {
      std::wstring localWiki = FindBundledCompatibilityWiki();
      if (!localWiki.empty()) {
        logLines.push_back(L"Falling back to bundled compatibility wiki at " + localWiki);
        std::wstring localStatus;
        if (RefreshCompatibilityFromLocal(localWiki, result->cache, logLines, localStatus)) {
          fetched = true;
          hasNewData = true;
          status = localStatus;
        } else if (!localStatus.empty()) {
          status = localStatus;
        }
      }
    }
    std::vector<CompatibilityProfile> overrides;
    if (LoadCompatibilityOverrides(overrides)) {
      MergeOverrides(result->cache.profiles, overrides, logLines);
    }
    result->cache.meta.count = result->cache.profiles.size();
    if (hasNewData) {
      SaveCompatibilityCache(result->cache);
      SaveCompatibilityMeta(result->cache.meta);
    }
    if (logLines.empty()) {
      logLines.push_back(status.empty() ? L"Compatibility sync completed." : status);
    }
    result->success = fetched;
    result->status = status;
    result->logLines = logLines;
    PostMessageW(hwnd, WM_APP_COMPAT_REFRESHED, 0, reinterpret_cast<LPARAM>(result.release()));
  }).detach();
}

void HandleCompatibilityRefreshed(HWND hwnd, AppState* state, LPARAM lparam) {
  std::unique_ptr<CompatRefreshResult> result(reinterpret_cast<CompatRefreshResult*>(lparam));
  if (!state) {
    return;
  }
  state->compat_refresh_in_progress = false;
  if (result) {
    if (!result->logLines.empty()) {
      LogCompatibilityEvent(result->logLines);
    }
    if (result->success) {
      state->compat_cache = std::move(result->cache);
      state->compat_cache.meta.count = state->compat_cache.profiles.size();
      state->compat_loaded = true;
      state->compat_status = result->status.empty() ? L"Compatibility list is up to date." : result->status;
      ApplyConfigToGames(state);
      state->game_config.Save();
      SettingsManager::Save(state->settings);
      if (hwnd) {
        InvalidateRect(hwnd, nullptr, TRUE);
      }
    } else {
      state->compat_status = result->status.empty() ? L"Compatibility sync failed." : result->status;
    }
  } else {
    state->compat_status = L"Compatibility sync failed.";
  }
  UpdateStatusBar(state, state->compat_status);
}

void BeginIgdbPrefetch(HWND hwnd, AppState* state) {
  if (!state || state->igdb_fetch_in_progress || state->games.empty()) {
    return;
  }
  if (state->settings.igdbClientId.empty() || state->settings.igdbClientSecret.empty()) {
    state->igdb_status = L"Configure IGDB Client ID and Secret in Settings to fetch covers.";
    UpdateStatusBar(state, state->igdb_status);
    return;
  }

  state->igdb_fetch_in_progress = true;
  state->igdb_status = L"Fetching IGDB metadata...";
  UpdateStatusBar(state, state->igdb_status);
  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }

  struct IgdbWorkItem {
    size_t index;
    std::wstring name;
    std::wstring exePath;
  };

  std::vector<IgdbWorkItem> items;
  items.reserve(state->games.size());
  for (size_t i = 0; i < state->games.size(); ++i) {
    IgdbWorkItem item;
    item.index = i;
    item.name = state->games[i].name;
    item.exePath = state->games[i].exe;
    items.push_back(std::move(item));
  }

  SettingsData settings_copy = state->settings;

  std::thread([hwnd, state, settings_copy, items]() mutable {
    std::wstring error;
    bool token_updated = false;
    if (settings_copy.igdbToken.empty() || IsIsoExpired(settings_copy.igdbTokenExpiresUtc)) {
      if (!IGDB::EnsureAccessToken(settings_copy.igdbClientId, settings_copy.igdbClientSecret, settings_copy.igdbToken,
                                   settings_copy.igdbTokenExpiresUtc, error)) {
        auto result = std::make_unique<IgdbFetchResult>();
        result->final = true;
        result->success = false;
        result->status = error.empty() ? L"Failed to refresh IGDB token." : error;
        PostMessageW(hwnd, WM_APP_IGDB_RESULT, 0, reinterpret_cast<LPARAM>(result.release()));
        return;
      }
      token_updated = true;
      auto token_result = std::make_unique<IgdbFetchResult>();
      token_result->tokenUpdated = true;
      token_result->token = settings_copy.igdbToken;
      token_result->tokenExpiry = settings_copy.igdbTokenExpiresUtc;
      token_result->status = L"IGDB access token refreshed.";
      PostMessageW(hwnd, WM_APP_IGDB_RESULT, 0, reinterpret_cast<LPARAM>(token_result.release()));
    }

    for (const auto& item : items) {
      auto result = std::make_unique<IgdbFetchResult>();
      result->gameIndex = item.index;
      result->success = false;
      std::wstring search_term = item.name;
      if (search_term.empty()) {
        search_term = std::filesystem::path(item.exePath).stem().wstring();
      }
      std::wstring display_name = search_term.empty() ? L"<Unnamed>" : search_term;
      std::optional<IgdbGame> game;
      if (!search_term.empty()) {
        game = IGDB::SearchOne(settings_copy.igdbClientId, settings_copy.igdbToken, search_term);
      }
      if (game) {
        result->success = true;
        result->gameData = *game;
        if (!game->coverImageId.empty()) {
          std::wstring url = IGDB::ImageUrlCoverBig(game->coverImageId);
          std::wstring cached = IGDB::CacheImage(url);
          if (!cached.empty() && CoverCache::SaveImageFileForExe(cached, item.exePath, kTileWidth, kTileHeight)) {
            result->coverUpdated = true;
            result->coverPath = CoverCache::PathForExe(item.exePath);
          }
        }
        if (result->gameData.summary.empty()) {
          result->status = L"Fetched IGDB details for " + display_name + L".";
        } else {
          result->status = L"Loaded IGDB profile for " + display_name + L".";
        }
      } else {
        result->status = L"No IGDB match for " + display_name + L".";
      }
      PostMessageW(hwnd, WM_APP_IGDB_RESULT, 0, reinterpret_cast<LPARAM>(result.release()));
    }

    auto final_result = std::make_unique<IgdbFetchResult>();
    final_result->final = true;
    final_result->success = true;
    final_result->status = L"IGDB metadata refresh complete.";
    if (token_updated) {
      final_result->tokenUpdated = true;
      final_result->token = settings_copy.igdbToken;
      final_result->tokenExpiry = settings_copy.igdbTokenExpiresUtc;
    }
    PostMessageW(hwnd, WM_APP_IGDB_RESULT, 0, reinterpret_cast<LPARAM>(final_result.release()));
  }).detach();
}

void HandleIgdbResult(HWND hwnd, AppState* state, LPARAM lparam) {
  std::unique_ptr<IgdbFetchResult> result(reinterpret_cast<IgdbFetchResult*>(lparam));
  if (!state || !result) {
    return;
  }

  if (result->tokenUpdated) {
    state->settings.igdbToken = result->token;
    state->settings.igdbTokenExpiresUtc = result->tokenExpiry;
    SettingsManager::Save(state->settings);
  }

  if (result->gameIndex != static_cast<size_t>(-1) && result->gameIndex < state->games.size()) {
    GameEntry& game = state->games[result->gameIndex];
    game.igdbRequested = true;
    if (result->success) {
      game.igdbHasData = true;
      game.igdbSummary = result->gameData.summary;
      game.igdbGenres = result->gameData.genres;
      game.igdbPlatforms = result->gameData.platforms;
      game.igdbReleaseDate = result->gameData.releaseDate;
      game.igdbRating = result->gameData.rating;
      game.igdbCoverPath = result->coverPath;
      if (result->coverUpdated) {
        if (game.coverBmp) {
          DeleteObject(game.coverBmp);
        }
        game.coverBmp = CoverCache::LoadForExe(game.exe, kTileWidth, kTileHeight);
      }
    } else {
      game.igdbHasData = false;
    }
  }

  if (!result->status.empty()) {
    state->igdb_status = result->status;
    UpdateStatusBar(state, state->igdb_status);
  }

  if (result->final) {
    state->igdb_fetch_in_progress = false;
  }

  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }
}

std::vector<std::wstring> ParseInjectionList(const std::wstring& text) {
  std::vector<std::wstring> items;
  for (const auto& line : SplitLines(text)) {
    std::wstring trimmed = Trim(line);
    if (!trimmed.empty()) {
      items.push_back(trimmed);
    }
  }
  return items;
}

void UpdateSelectionStatus(AppState* state) {
  if (!state) {
    return;
  }
  if (state->games.empty()) {
    UpdateStatusBar(state, L"No games detected. Configure scan folders in Settings.");
    return;
  }
  if (state->selected_index >= state->games.size()) {
    state->selected_index = 0;
  }
  const GameEntry& game = state->games[state->selected_index];
  std::wstring status = L"Selected ";
  status.append(game.name.empty() ? L"<Unnamed>" : game.name);
  status.append(L" (" + (game.source.empty() ? std::wstring(L"custom") : game.source) + L")");
  if (!game.plannedFiles.empty()) {
    status.append(L" • ");
    status.append(std::to_wstring(game.plannedFiles.size()));
    status.append(game.plannedFiles.size() == 1 ? L" file" : L" files");
  }
  UpdateStatusBar(state, status);
}

void SelectGame(AppState* state, size_t index) {
  if (!state || index >= state->games.size()) {
    return;
  }
  state->selected_index = index;
  UpdateSelectionStatus(state);
}

void ToggleGameInjection(AppState* state, size_t index) {
  if (!state || index >= state->games.size()) {
    return;
  }
  GameEntry& game = state->games[index];
  game.injectEnabled = !game.injectEnabled;
  GameInjectionSettings settings = state->game_config.GetSettingsFor(game.exe);
  if (settings.files.empty()) {
    settings.files = state->settings.defaultInjectionFiles.empty() ? game.plannedFiles : state->settings.defaultInjectionFiles;
  }
  settings.enabled = game.injectEnabled;
  settings.files = game.plannedFiles;
  state->game_config.SetSettingsFor(game.exe, settings);
  state->game_config.Save();
  std::wstring status = L"Injection for ";
  status.append(game.name.empty() ? L"<Unnamed>" : game.name);
  if (!state->settings.globalInjectionEnabled) {
    status.append(L": global override disabled (enable globally in Settings)");
  } else {
    status.append(game.injectEnabled ? L" enabled." : L" disabled.");
  }
  UpdateStatusBar(state, status);
}

void HandleKeyDown(HWND hwnd, AppState* state, WPARAM key) {
  if (!state || state->games.empty()) {
    return;
  }
  size_t index = state->selected_index;
  size_t new_index = index;
  const int columns = state->grid_columns <= 0 ? 1 : state->grid_columns;
  switch (key) {
    case VK_LEFT:
      if (index > 0 && (index % columns) != 0) {
        new_index = index - 1;
      }
      break;
    case VK_RIGHT:
      if (index + 1 < state->games.size() && ((index + 1) % columns) != 0) {
        new_index = index + 1;
      }
      break;
    case VK_UP:
      if (index >= static_cast<size_t>(columns)) {
        new_index = index - columns;
      }
      break;
    case VK_DOWN:
      if (index + columns < state->games.size()) {
        new_index = index + columns;
      }
      break;
    case VK_SPACE:
      ToggleGameInjection(state, index);
      InvalidateRect(hwnd, nullptr, TRUE);
      return;
    default:
      return;
  }

  if (new_index != index) {
    SelectGame(state, new_index);
    InvalidateRect(hwnd, nullptr, TRUE);
  }
}

void HandleMouseDown(HWND hwnd, AppState* state, int x, int y) {
  if (!state) {
    return;
  }
  POINT pt = {x, y};
  for (size_t i = 0; i < state->tile_bounds.size(); ++i) {
    if (PtInRect(&state->tile_bounds[i], pt)) {
      SelectGame(state, i);
      InvalidateRect(hwnd, nullptr, TRUE);
      SetFocus(hwnd);
      break;
    }
  }
}

struct SettingsDialogContext {
  AppState* app_state = nullptr;
  int current_game_index = -1;
  bool applied = false;
};

std::wstring GetDlgItemTextString(HWND dlg, int control_id) {
  HWND control = GetDlgItem(dlg, control_id);
  if (!control) {
    return {};
  }
  int length = GetWindowTextLengthW(control);
  std::wstring text(static_cast<size_t>(length), L'\0');
  if (length > 0) {
    GetWindowTextW(control, text.data(), length + 1);
  }
  text.resize(length);
  return text;
}

void SetDlgItemTextString(HWND dlg, int control_id, const std::wstring& text) {
  SetDlgItemTextW(dlg, control_id, text.c_str());
}

void UpdateIgdbStatusText(HWND dlg, const SettingsData& settings) {
  std::wstring status;
  if (settings.igdbToken.empty()) {
    status = L"Token not acquired.";
  } else {
    status = L"Token expires ";
    status.append(settings.igdbTokenExpiresUtc.empty() ? L"(unknown)" : settings.igdbTokenExpiresUtc);
  }
  SetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_STATUS, status);
}

void PopulateCustomFolders(HWND dlg, const std::vector<std::wstring>& folders) {
  SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_RESETCONTENT, 0, 0);
  for (const auto& folder : folders) {
    SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(folder.c_str()));
  }
}

std::vector<std::wstring> CollectCustomFolders(HWND dlg) {
  std::vector<std::wstring> folders;
  LRESULT count = SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_GETCOUNT, 0, 0);
  for (LRESULT i = 0; i < count; ++i) {
    LRESULT length = SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_GETTEXTLEN, i, 0);
    if (length <= 0) {
      continue;
    }
    std::wstring buffer(static_cast<size_t>(length), L'\0');
    SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_GETTEXT, i, reinterpret_cast<LPARAM>(buffer.data()));
    buffer.resize(static_cast<size_t>(length));
    std::wstring trimmed = Trim(buffer);
    if (!trimmed.empty()) {
      folders.push_back(trimmed);
    }
  }
  return folders;
}

void PersistGameFromDialog(HWND dlg, SettingsDialogContext* context) {
  if (!context || !context->app_state || context->current_game_index < 0) {
    return;
  }
  size_t index = static_cast<size_t>(context->current_game_index);
  if (index >= context->app_state->games.size()) {
    return;
  }
  GameEntry& game = context->app_state->games[index];
  const BOOL checked = IsDlgButtonChecked(dlg, IDC_SETTINGS_GAME_ENABLE);
  game.injectEnabled = (checked == BST_CHECKED);
  game.plannedFiles = ParseInjectionList(GetDlgItemTextString(dlg, IDC_SETTINGS_GAME_FILES));
  if (game.plannedFiles.empty()) {
    game.plannedFiles = context->app_state->settings.defaultInjectionFiles;
  }
  GameInjectionSettings settings;
  settings.enabled = game.injectEnabled;
  settings.files = game.plannedFiles;
  context->app_state->game_config.SetSettingsFor(game.exe, settings);
}

void LoadGameIntoDialog(HWND dlg, SettingsDialogContext* context, int list_index) {
  if (!context || !context->app_state) {
    return;
  }
  if (list_index < 0 || static_cast<size_t>(list_index) >= context->app_state->games.size()) {
    CheckDlgButton(dlg, IDC_SETTINGS_GAME_ENABLE, BST_UNCHECKED);
    SetDlgItemTextString(dlg, IDC_SETTINGS_GAME_FILES, L"");
    context->current_game_index = -1;
    return;
  }

  context->current_game_index = list_index;
  const GameEntry& game = context->app_state->games[static_cast<size_t>(list_index)];
  CheckDlgButton(dlg, IDC_SETTINGS_GAME_ENABLE, game.injectEnabled ? BST_CHECKED : BST_UNCHECKED);
  SetDlgItemTextString(dlg, IDC_SETTINGS_GAME_FILES, JoinLines(game.plannedFiles));
}

void PopulateGameList(HWND dlg, SettingsDialogContext* context) {
  if (!context || !context->app_state) {
    return;
  }
  SendDlgItemMessageW(dlg, IDC_SETTINGS_GAME_LIST, LB_RESETCONTENT, 0, 0);
  for (size_t i = 0; i < context->app_state->games.size(); ++i) {
    const auto& game = context->app_state->games[i];
    const std::wstring label = game.name.empty() ? L"<Unnamed>" : game.name;
    LRESULT idx = SendDlgItemMessageW(dlg, IDC_SETTINGS_GAME_LIST, LB_ADDSTRING, 0,
                                      reinterpret_cast<LPARAM>(label.c_str()));
    SendDlgItemMessageW(dlg, IDC_SETTINGS_GAME_LIST, LB_SETITEMDATA, idx, static_cast<LPARAM>(i));
  }
  if (!context->app_state->games.empty()) {
    size_t initial = std::min(context->app_state->selected_index, context->app_state->games.size() - 1);
    SendDlgItemMessageW(dlg, IDC_SETTINGS_GAME_LIST, LB_SETCURSEL, initial, 0);
    LoadGameIntoDialog(dlg, context, static_cast<int>(initial));
  }
}

std::wstring BrowseForFolder(HWND owner) {
  BROWSEINFOW bi = {};
  bi.hwndOwner = owner;
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
  bi.lpszTitle = L"Select a folder";
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
  if (!pidl) {
    return {};
  }
  wchar_t path[MAX_PATH] = {};
  if (!SHGetPathFromIDListW(pidl, path)) {
    CoTaskMemFree(pidl);
    return {};
  }
  std::wstring result(path);
  CoTaskMemFree(pidl);
  return result;
}

void ApplySettingsFromDialog(HWND dlg, SettingsDialogContext* context) {
  if (!context || !context->app_state) {
    return;
  }
  PersistGameFromDialog(dlg, context);

  AppState* state = context->app_state;
  SettingsData& settings = state->settings;

  std::wstring opti_dir = Trim(GetDlgItemTextString(dlg, IDC_SETTINGS_OPTISCALER_PATH));
  std::wstring fallback_dir = Trim(GetDlgItemTextString(dlg, IDC_SETTINGS_FALLBACK_PATH));
  settings.optiScalerInstallDir = opti_dir;
  settings.fallbackOptiScalerDir = fallback_dir;
  settings.autoUpdateEnabled = (IsDlgButtonChecked(dlg, IDC_SETTINGS_AUTO_UPDATE) == BST_CHECKED);
  settings.globalInjectionEnabled = (IsDlgButtonChecked(dlg, IDC_SETTINGS_GLOBAL_ENABLE) == BST_CHECKED);
  std::wstring igdb_id = Trim(GetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_CLIENT_ID));
  std::wstring igdb_secret = Trim(GetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_SECRET));
  bool creds_changed = (settings.igdbClientId != igdb_id) || (settings.igdbClientSecret != igdb_secret);
  settings.igdbClientId = igdb_id;
  settings.igdbClientSecret = igdb_secret;
  if (creds_changed) {
    settings.igdbToken.clear();
    settings.igdbTokenExpiresUtc.clear();
    UpdateIgdbStatusText(dlg, settings);
  }
  settings.defaultInjectionFiles = ParseInjectionList(GetDlgItemTextString(dlg, IDC_SETTINGS_DEFAULT_FILES));
  if (settings.defaultInjectionFiles.empty()) {
    settings.defaultInjectionFiles = {L"OptiScaler\\OptiScaler.dll", L"OptiScaler\\OptiScaler.ini"};
  }
  std::vector<std::wstring> folders = CollectCustomFolders(dlg);
  bool custom_changed = folders != settings.customScanFolders;
  settings.customScanFolders = std::move(folders);
  if (custom_changed) {
    state->require_rescan_after_settings = true;
  }

  for (auto& game : state->games) {
    if (game.plannedFiles.empty()) {
      game.plannedFiles = settings.defaultInjectionFiles;
    }
    GameInjectionSettings cfg;
    cfg.enabled = game.injectEnabled;
    cfg.files = game.plannedFiles;
    state->game_config.SetSettingsFor(game.exe, cfg);
  }

  state->opti_manager.SetInstallDirectory(settings.optiScalerInstallDir);
  state->opti_manager.SetAutoUpdateEnabled(settings.autoUpdateEnabled);
  state->opti_manager.SetFallbackPackageDirectory(settings.fallbackOptiScalerDir);

  state->game_config.Save();
  SettingsManager::Save(settings);
  ApplyConfigToGames(state);
  context->applied = true;
}

INT_PTR CALLBACK SettingsDialogProc(HWND dlg, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto* context = reinterpret_cast<SettingsDialogContext*>(GetWindowLongPtrW(dlg, GWLP_USERDATA));
  switch (msg) {
    case WM_INITDIALOG: {
      context = reinterpret_cast<SettingsDialogContext*>(lparam);
      SetWindowLongPtrW(dlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(context));
      if (!context || !context->app_state) {
        return TRUE;
      }
      const SettingsData& settings = context->app_state->settings;
      SetDlgItemTextString(dlg, IDC_SETTINGS_OPTISCALER_PATH, settings.optiScalerInstallDir);
      SetDlgItemTextString(dlg, IDC_SETTINGS_FALLBACK_PATH, settings.fallbackOptiScalerDir);
      CheckDlgButton(dlg, IDC_SETTINGS_AUTO_UPDATE, settings.autoUpdateEnabled ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_SETTINGS_GLOBAL_ENABLE, settings.globalInjectionEnabled ? BST_CHECKED : BST_UNCHECKED);
      SetDlgItemTextString(dlg, IDC_SETTINGS_DEFAULT_FILES, JoinLines(settings.defaultInjectionFiles));
      SetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_CLIENT_ID, settings.igdbClientId);
      SetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_SECRET, settings.igdbClientSecret);
      UpdateIgdbStatusText(dlg, settings);
      PopulateCustomFolders(dlg, settings.customScanFolders);
      PopulateGameList(dlg, context);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wparam)) {
        case IDC_SETTINGS_ADD_FOLDER: {
          std::wstring folder = BrowseForFolder(dlg);
          if (!folder.empty()) {
            SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_ADDSTRING, 0,
                                reinterpret_cast<LPARAM>(folder.c_str()));
          }
          return TRUE;
        }
        case IDC_SETTINGS_REMOVE_FOLDER: {
          LRESULT sel = SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_GETCURSEL, 0, 0);
          if (sel != LB_ERR) {
            SendDlgItemMessageW(dlg, IDC_SETTINGS_CUSTOM_LIST, LB_DELETESTRING, sel, 0);
          }
          return TRUE;
        }
        case IDC_SETTINGS_FETCH_TOKEN: {
          if (!context || !context->app_state) {
            return TRUE;
          }
          std::wstring client_id = Trim(GetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_CLIENT_ID));
          std::wstring secret = Trim(GetDlgItemTextString(dlg, IDC_SETTINGS_IGDB_SECRET));
          std::wstring token;
          std::wstring expires;
          std::wstring error;
          if (IGDB::EnsureAccessToken(client_id, secret, token, expires, error)) {
            SettingsData& settings = context->app_state->settings;
            settings.igdbClientId = client_id;
            settings.igdbClientSecret = secret;
            settings.igdbToken = token;
            settings.igdbTokenExpiresUtc = expires;
            SettingsManager::Save(settings);
            UpdateIgdbStatusText(dlg, settings);
            context->applied = true;
            MessageBoxW(dlg, L"IGDB access token fetched successfully.", L"OptiScaler Manager Lite", MB_ICONINFORMATION);
          } else {
            std::wstring message = L"Failed to fetch IGDB token: " + (error.empty() ? L"Unknown error." : error);
            MessageBoxW(dlg, message.c_str(), L"OptiScaler Manager Lite", MB_ICONERROR);
          }
          return TRUE;
        }
        case IDC_SETTINGS_BROWSE_OPTISCALER: {
          std::wstring folder = BrowseForFolder(dlg);
          if (!folder.empty()) {
            SetDlgItemTextString(dlg, IDC_SETTINGS_OPTISCALER_PATH, folder);
          }
          return TRUE;
        }
        case IDC_SETTINGS_BROWSE_FALLBACK: {
          std::wstring folder = BrowseForFolder(dlg);
          if (!folder.empty()) {
            SetDlgItemTextString(dlg, IDC_SETTINGS_FALLBACK_PATH, folder);
          }
          return TRUE;
        }
        case IDC_SETTINGS_GAME_LIST: {
          if (HIWORD(wparam) == LBN_SELCHANGE) {
            LRESULT sel = SendDlgItemMessageW(dlg, IDC_SETTINGS_GAME_LIST, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
              PersistGameFromDialog(dlg, context);
              LoadGameIntoDialog(dlg, context, static_cast<int>(sel));
            }
          }
          return TRUE;
        }
        case IDC_SETTINGS_APPLY: {
          ApplySettingsFromDialog(dlg, context);
          return TRUE;
        }
        case IDOK: {
          ApplySettingsFromDialog(dlg, context);
          EndDialog(dlg, IDOK);
          return TRUE;
        }
        case IDCANCEL: {
          EndDialog(dlg, IDCANCEL);
          return TRUE;
        }
        default:
          break;
      }
      break;
    }
  }
  return FALSE;
}

void ShowSettingsDialog(HWND owner, AppState* state) {
  if (!state) {
    return;
  }
  SettingsDialogContext context;
  context.app_state = state;
  DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_SETTINGS), owner, SettingsDialogProc,
                  reinterpret_cast<LPARAM>(&context));
  if (context.applied) {
    SettingsManager::Save(state->settings);
    state->game_config.Save();
    ApplyConfigToGames(state);
    if (state->require_rescan_after_settings) {
      PerformScan(owner, state);
      state->require_rescan_after_settings = false;
    } else {
      InvalidateRect(owner, nullptr, TRUE);
    }
    UpdateSelectionStatus(state);
    BeginIgdbPrefetch(owner, state);
  }
}

RendererPreference ParseRendererPreference() {
  // TODO: Load renderer preference from persisted settings.
  return RendererPreference::kGDI;
}

void UpdateStatusBar(AppState* state, const std::wstring& text) {
  if (state && state->status_bar) {
    SendMessageW(state->status_bar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
  }
}

void PerformScan(HWND hwnd, AppState* state) {
  if (!state) {
    return;
  }

  UpdateStatusBar(state, L"Scanning for games...");
  std::vector<std::wstring> roots = Scanner::DefaultFolders();
  for (const auto& custom : state->settings.customScanFolders) {
    if (!custom.empty()) {
      roots.push_back(custom);
    }
  }
  std::sort(roots.begin(), roots.end());
  roots.erase(std::unique(roots.begin(), roots.end()), roots.end());

  FreeGameAssets(&state->games);
  state->games = Scanner::ScanAll(roots);
  state->selected_index = 0;
  state->tile_bounds.assign(state->games.size(), RECT{});
  state->igdb_status.clear();
  state->igdb_fetch_in_progress = false;
  for (auto& game : state->games) {
    game.igdbRequested = false;
    game.igdbHasData = false;
    game.igdbSummary.clear();
    game.igdbGenres.clear();
    game.igdbPlatforms.clear();
    game.igdbReleaseDate.clear();
    game.igdbRating = 0.0;
    game.igdbCoverPath.clear();
  }

  std::unordered_set<std::wstring> present_paths;
  for (const auto& game : state->games) {
    present_paths.insert(game.exe);
  }
  state->game_config.RemoveMissing(present_paths);
  ApplyConfigToGames(state);
  state->game_config.Save();
  SettingsManager::Save(state->settings);

  if (state->games.empty()) {
    if (roots.empty()) {
      UpdateStatusBar(state,
                      L"No default or custom scan folders detected. Use Settings to add your game libraries.");
    } else {
      UpdateStatusBar(state, L"Scan completed but no games were detected in the configured folders.");
    }
  } else {
    std::wstringstream status;
    status << L"Found " << state->games.size() << (state->games.size() == 1 ? L" game" : L" games");
    if (!roots.empty()) {
      status << L" across " << roots.size() << (roots.size() == 1 ? L" folder." : L" folders.");
    } else {
      status << L'.';
    }
    UpdateStatusBar(state, status.str());
  }

  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }
}

void OnPaint(HWND hwnd, AppState* state) {
  PAINTSTRUCT ps;
  BeginPaint(hwnd, &ps);
  RECT rc;
  GetClientRect(hwnd, &rc);
  if (!state || !state->renderer) {
    EndPaint(hwnd, &ps);
    return;
  }

  const int client_width = rc.right - rc.left;
  const int client_height = rc.bottom - rc.top;
  state->renderer->Resize(client_width, client_height);
  state->renderer->Begin();

  const COLORREF text_color = RGB(245, 245, 245);
  const COLORREF highlight_color = RGB(255, 223, 0);
  const COLORREF meta_color = RGB(200, 210, 255);
  const COLORREF subtle_color = RGB(190, 190, 190);
  const int title_y = 12;
  state->renderer->DrawText(L"OptiScaler Manager Lite (prototype)", kGridMargin, title_y, text_color);

  int content_top = title_y + 28;
  if (!state->compat_status.empty()) {
    std::vector<std::wstring> status_lines = SplitLines(state->compat_status);
    DrawMultiline(state->renderer.get(), state->compat_status, kGridMargin, content_top, meta_color);
    content_top += static_cast<int>(status_lines.size() * 18) + 8;
  }
  if (!state->igdb_status.empty()) {
    std::vector<std::wstring> igdb_lines = SplitLines(state->igdb_status);
    DrawMultiline(state->renderer.get(), state->igdb_status, kGridMargin, content_top, meta_color);
    content_top += static_cast<int>(igdb_lines.size() * 18) + 8;
  }
  int details_x = client_width - kDetailsWidth - kGridMargin;
  details_x = std::max(details_x, kGridMargin + kTileWidth + kTileSpacing);
  int grid_width = std::max(0, details_x - kGridMargin);
  int columns = std::max(1, grid_width / (kTileWidth + kTileSpacing));
  state->grid_columns = columns;
  state->tile_bounds.assign(state->games.size(), RECT{});

  if (!state->games.empty()) {
    const int tile_pitch_y = kTileHeight + 96;
    for (size_t i = 0; i < state->games.size(); ++i) {
      int row = static_cast<int>(i / columns);
      int col = static_cast<int>(i % columns);
      int tile_x = kGridMargin + col * (kTileWidth + kTileSpacing);
      int tile_y = content_top + row * tile_pitch_y;
      if (tile_y + kTileHeight > client_height - 32) {
        break;
      }
      state->tile_bounds[i] = {tile_x, tile_y, tile_x + kTileWidth, tile_y + kTileHeight};
      const GameEntry& game = state->games[i];
      if (game.coverBmp) {
        state->renderer->DrawBitmap(game.coverBmp, tile_x, tile_y, kTileWidth, kTileHeight);
      }
      const std::wstring& display_name = game.name.empty() ? L"<Unnamed>" : game.name;
      COLORREF name_color = (i == state->selected_index) ? highlight_color : text_color;
      state->renderer->DrawText(display_name, tile_x, tile_y + kTileHeight + 4, name_color);

      std::wstring meta_line = game.source.empty() ? L"custom" : game.source;
      if (state->settings.globalInjectionEnabled) {
        meta_line.append(game.injectEnabled ? L" • injection on" : L" • per-game off");
      } else {
        meta_line.append(L" • global injection off");
      }
      int meta_y = tile_y + kTileHeight + 24;
      state->renderer->DrawText(meta_line, tile_x, meta_y, subtle_color);

      int compat_y = meta_y + 16;
      if (game.compatProfileIndex >= 0) {
        std::wstring compat_line = L"Compat: ";
        if (!game.compatBadges.empty()) {
          compat_line.append(game.compatBadges.front());
        } else if (!game.compatInputs.empty()) {
          compat_line.append(L"Profile loaded");
        } else {
          compat_line.append(L"Profile available");
        }
        state->renderer->DrawText(compat_line, tile_x, compat_y, meta_color);
        if (game.compatBadges.size() > 1) {
          std::wstring badge_line;
          for (size_t b = 1; b < std::min<size_t>(game.compatBadges.size(), 3); ++b) {
            if (!badge_line.empty()) {
              badge_line.append(L" • ");
            }
            badge_line.append(game.compatBadges[b]);
          }
          if (!badge_line.empty()) {
            state->renderer->DrawText(badge_line, tile_x, compat_y + 16, meta_color);
          }
        }
      } else if (state->compat_refresh_in_progress) {
        state->renderer->DrawText(L"Compatibility: refreshing...", tile_x, compat_y, subtle_color);
      } else if (state->compat_cache.profiles.empty()) {
        state->renderer->DrawText(L"Compatibility: unavailable", tile_x, compat_y, subtle_color);
      }
    }
  } else {
    state->renderer->DrawText(L"No games found. Use File → Settings to add custom libraries and rescan.", kGridMargin,
                              content_top, subtle_color);
  }

  int details_y = content_top;
  state->renderer->DrawText(L"System overview", details_x, details_y, highlight_color);
  details_y += 18;
  std::wstring gpu_line = L"GPU: " + (state->system_info.gpuName.empty() ? L"Unknown" : state->system_info.gpuName);
  if (state->system_info.gpuMemoryMB > 0) {
    gpu_line.append(L" (" + std::to_wstring(state->system_info.gpuMemoryMB) + L" MB VRAM)");
  }
  state->renderer->DrawText(gpu_line, details_x, details_y, text_color);
  details_y += 18;
  std::wstring cpu_line = L"CPU: " + (state->system_info.cpuName.empty() ? L"Unknown" : state->system_info.cpuName);
  if (state->system_info.cpuCores > 0 || state->system_info.cpuThreads > 0) {
    cpu_line.append(L" (" + std::to_wstring(state->system_info.cpuCores) + L" cores / " +
                    std::to_wstring(state->system_info.cpuThreads) + L" threads)");
  }
  state->renderer->DrawText(cpu_line, details_x, details_y, text_color);
  details_y += 18;
  if (state->system_info.ramMB > 0) {
    std::wstring ram_line = L"RAM: " + std::to_wstring(state->system_info.ramMB) + L" MB";
    state->renderer->DrawText(ram_line, details_x, details_y, text_color);
    details_y += 18;
  }

  std::wstring global_line = L"Global injection: ";
  global_line.append(state->settings.globalInjectionEnabled ? L"enabled" : L"disabled");
  state->renderer->DrawText(global_line, details_x, details_y, text_color);
  details_y += 18;

  std::wstring install_line = L"OptiScaler install: ";
  install_line.append(state->settings.optiScalerInstallDir.empty() ? L"not set" : state->settings.optiScalerInstallDir);
  DrawMultiline(state->renderer.get(), install_line, details_x, details_y, meta_color);
  details_y += 36;

  std::wstring fallback_line = L"Offline package: ";
  fallback_line.append(state->settings.fallbackOptiScalerDir.empty() ? L"not configured"
                                                                : state->settings.fallbackOptiScalerDir);
  DrawMultiline(state->renderer.get(), fallback_line, details_x, details_y, meta_color);
  details_y += 36;

  if (!state->games.empty() && state->selected_index < state->games.size()) {
    const GameEntry& game = state->games[state->selected_index];
    state->renderer->DrawText(L"Selected game", details_x, details_y, highlight_color);
    details_y += 18;
    std::wstring info = L"Name: " + (game.name.empty() ? L"<Unnamed>" : game.name);
    info.append(L"\nSource: ");
    info.append(game.source.empty() ? L"custom" : game.source);
    info.append(L"\nFolder: ");
    info.append(game.folder);
    info.append(L"\nOfficial support: ");
    info.append(game.supportSummary.empty() ? L"Not available" : game.supportSummary);
    const bool effective_injection = state->settings.globalInjectionEnabled && game.injectEnabled;
    info.append(L"\nInjection status: ");
    if (!state->settings.globalInjectionEnabled) {
      info.append(L"Disabled globally");
    } else {
      info.append(effective_injection ? L"Enabled" : L"Disabled for this game");
    }
    std::vector<std::wstring> info_lines = SplitLines(info);
    DrawMultiline(state->renderer.get(), info, details_x, details_y, text_color);
    details_y += static_cast<int>(info_lines.size() * 18);

    std::wstring files_text = L"Planned files:";
    if (game.plannedFiles.empty()) {
      files_text.append(L"\n  • (none configured)");
    } else {
      for (const auto& file : game.plannedFiles) {
        files_text.append(L"\n  • ");
        files_text.append(file);
      }
    }
    std::vector<std::wstring> file_lines = SplitLines(files_text);
    DrawMultiline(state->renderer.get(), files_text, details_x, details_y, meta_color);
    details_y += static_cast<int>(file_lines.size() * 16);

    if (game.igdbHasData) {
      details_y += 12;
      state->renderer->DrawText(L"IGDB Metadata", details_x, details_y, highlight_color);
      details_y += 18;
      if (!game.igdbSummary.empty()) {
        std::vector<std::wstring> summary_lines = SplitLines(game.igdbSummary);
        DrawMultiline(state->renderer.get(), game.igdbSummary, details_x, details_y, text_color);
        details_y += static_cast<int>(summary_lines.size() * 16);
      }
      std::wstring genre_line = JoinWithComma(game.igdbGenres);
      if (!genre_line.empty()) {
        DrawMultiline(state->renderer.get(), L"Genres: " + genre_line, details_x, details_y, meta_color);
        details_y += 18;
      }
      std::wstring platform_line = JoinWithComma(game.igdbPlatforms);
      if (!platform_line.empty()) {
        DrawMultiline(state->renderer.get(), L"Platforms: " + platform_line, details_x, details_y, meta_color);
        details_y += 18;
      }
      if (!game.igdbReleaseDate.empty()) {
        DrawMultiline(state->renderer.get(), L"Release: " + game.igdbReleaseDate, details_x, details_y, meta_color);
        details_y += 18;
      }
      if (game.igdbRating > 0.0) {
        std::wstringstream rating;
        rating << std::fixed << std::setprecision(1) << game.igdbRating;
        DrawMultiline(state->renderer.get(), L"Rating: " + rating.str(), details_x, details_y, meta_color);
        details_y += 18;
      }
    } else if (!state->igdb_fetch_in_progress && state->settings.igdbClientId.empty()) {
      DrawMultiline(state->renderer.get(),
                    L"IGDB credentials not set. Add a Client ID and Secret in Settings to download covers and details.",
                    details_x, details_y, subtle_color);
      details_y += 32;
    }

    if (game.compatProfileIndex >= 0) {
      details_y += 12;
      state->renderer->DrawText(L"Compatibility", details_x, details_y, highlight_color);
      details_y += 18;

      std::wstring dll_line = L"Install DLL: ";
      dll_line.append(game.compatDll.empty() ? L"dxgi.dll" : game.compatDll);
      if (!game.compatTestedVersion.empty()) {
        dll_line.append(L" • Profile version ");
        dll_line.append(game.compatTestedVersion);
      }
      DrawMultiline(state->renderer.get(), dll_line, details_x, details_y, meta_color);
      details_y += 18;

      if (!game.compatInputs.empty()) {
        std::wstring input_line = L"Inputs: ";
        for (size_t i = 0; i < game.compatInputs.size(); ++i) {
          if (i > 0) {
            input_line.append(L", ");
          }
          input_line.append(game.compatInputs[i]);
        }
        DrawMultiline(state->renderer.get(), input_line, details_x, details_y, meta_color);
        details_y += 18;
      }

      std::wstring fg_line = L"Frame Generation: ";
      fg_line.append(game.compatFgSupported ? L"Supported" : L"Unsupported");
      COLORREF fg_color = game.compatFgSupported ? meta_color : highlight_color;
      DrawMultiline(state->renderer.get(), fg_line, details_x, details_y, fg_color);
      details_y += 18;

      if (!game.compatBadges.empty()) {
        std::wstring badge_text = L"Badges:";
        for (const auto& badge : game.compatBadges) {
          badge_text.append(L"\n  • ");
          badge_text.append(badge);
        }
        std::vector<std::wstring> badge_lines = SplitLines(badge_text);
        DrawMultiline(state->renderer.get(), badge_text, details_x, details_y, meta_color);
        details_y += static_cast<int>(badge_lines.size() * 16);
      }

      if (!game.compatKnownIssues.empty()) {
        std::wstring issues_text = L"Known issues:";
        for (const auto& issue : game.compatKnownIssues) {
          issues_text.append(L"\n  • ");
          issues_text.append(issue);
        }
        std::vector<std::wstring> issue_lines = SplitLines(issues_text);
        DrawMultiline(state->renderer.get(), issues_text, details_x, details_y, highlight_color);
        details_y += static_cast<int>(issue_lines.size() * 16);
      }

      if (!game.compatNotes.empty()) {
        std::wstring notes_text = L"Notes:";
        for (const auto& note : game.compatNotes) {
          notes_text.append(L"\n  • ");
          notes_text.append(note);
        }
        std::vector<std::wstring> note_lines = SplitLines(notes_text);
        DrawMultiline(state->renderer.get(), notes_text, details_x, details_y, meta_color);
        details_y += static_cast<int>(note_lines.size() * 16);
      }

      if (game.compatRequiresFakenvapi) {
        DrawMultiline(state->renderer.get(),
                      L"Hint: Install Fakenvapi if DLSS/FSR/XeSS inputs are hidden after copying OptiScaler.",
                      details_x, details_y, highlight_color);
        details_y += 32;
      }
    } else if (state->compat_cache.profiles.empty()) {
      DrawMultiline(state->renderer.get(),
                    L"Compatibility list not downloaded yet. Use Tools → Refresh Compatibility List to sync wiki guidance.",
                    details_x, details_y, subtle_color);
      details_y += 32;
    } else {
      DrawMultiline(state->renderer.get(),
                    L"No compatibility profile matched this game. Refresh or file an override if the wiki adds one.",
                    details_x, details_y, subtle_color);
      details_y += 32;
    }

    DrawMultiline(state->renderer.get(),
                  L"Tips: Press Space to toggle injection for the selected game. Use Settings to edit file lists."
                  L"\nTools → Refresh Compatibility List pulls the latest OptiScaler wiki guidance.",
                  details_x, details_y, subtle_color);
  }

  state->renderer->End();
  EndPaint(hwnd, &ps);
}

void OnSize(HWND hwnd, AppState* state, UINT width, UINT height) {
  if (state && state->status_bar) {
    SendMessageW(state->status_bar, WM_SIZE, 0, 0);
    RECT rc = {};
    GetWindowRect(state->status_bar, &rc);
    height -= static_cast<UINT>(rc.bottom - rc.top);
  }
  if (state && state->renderer) {
    state->renderer->Resize(width, height);
  }
}

void OnCommand(HWND hwnd, AppState* state, WPARAM wparam) {
  const int command = LOWORD(wparam);
  switch (command) {
    case IDM_FILE_EXIT:
      PostMessageW(hwnd, WM_CLOSE, 0, 0);
      break;
    case IDM_FILE_RESCAN: {
      PerformScan(hwnd, state);
      if (state && !state->games.empty()) {
        UpdateSelectionStatus(state);
      }
      break;
    }
    case IDM_TOOLS_SETTINGS:
      ShowSettingsDialog(hwnd, state);
      break;
    case IDM_TOOLS_REFRESH_COMPAT:
      BeginCompatibilityRefresh(hwnd, state, true);
      break;
    case IDM_HELP_LOGS:
      MessageBoxW(hwnd, L"Log folder opening not yet implemented.", L"OptiScaler Manager Lite", MB_ICONINFORMATION);
      break;
    default:
      break;
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  AppState* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  switch (msg) {
    case WM_CREATE: {
      auto* create = reinterpret_cast<LPCREATESTRUCTW>(lparam);
      auto* init_state = static_cast<AppState*>(create->lpCreateParams);
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(init_state));
      state = init_state;
      if (!state) {
        return -1;
      }
      INITCOMMONCONTROLSEX icc = {};
      icc.dwSize = sizeof(icc);
      icc.dwICC = ICC_BAR_CLASSES;
      InitCommonControlsEx(&icc);

      state->status_bar = CreateStatusWindowW(WS_CHILD | WS_VISIBLE, L"Ready", hwnd, IDC_STATUS_BAR);
      state->renderer = CreateRenderer(ParseRendererPreference(), hwnd);
      if (!state->renderer) {
        MessageBoxW(hwnd, L"Failed to initialize renderer.", L"OptiScaler Manager Lite", MB_ICONERROR);
        return -1;
      }
      state->system_info = GetSystemInfo();
      if (!SettingsManager::Load(state->settings)) {
        SettingsManager::Save(state->settings);
      }
      state->game_config.Load();
      state->opti_manager.SetInstallDirectory(state->settings.optiScalerInstallDir);
      state->opti_manager.SetAutoUpdateEnabled(state->settings.autoUpdateEnabled);
      state->opti_manager.SetFallbackPackageDirectory(state->settings.fallbackOptiScalerDir);
      EnsureCompatibilityLoaded(state);
      UpdateStatusBar(state, L"Ready.");
      PerformScan(hwnd, state);
      if (state->compat_cache.profiles.empty()) {
        BeginCompatibilityRefresh(hwnd, state, false);
      }
      break;
    }
    case WM_SIZE:
      OnSize(hwnd, state, LOWORD(lparam), HIWORD(lparam));
      break;
    case WM_COMMAND:
      OnCommand(hwnd, state, wparam);
      break;
    case WM_APP_COMPAT_REFRESHED:
      HandleCompatibilityRefreshed(hwnd, state, lparam);
      break;
    case WM_APP_IGDB_RESULT:
      HandleIgdbResult(hwnd, state, lparam);
      break;
    case WM_KEYDOWN:
      HandleKeyDown(hwnd, state, wparam);
      break;
    case WM_LBUTTONDOWN:
      HandleMouseDown(hwnd, state, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
      break;
    case WM_PAINT:
      OnPaint(hwnd, state);
      break;
    case WM_DESTROY:
      if (state) {
        state->game_config.Save();
        SettingsManager::Save(state->settings);
        FreeGameAssets(&state->games);
      }
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProcW(hwnd, msg, wparam, lparam);
  }
  return 0;
}

}  // namespace optiscaler

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int cmd_show) {
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = optiscaler::WndProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = optiscaler::kWindowClass;

  if (!RegisterClassExW(&wc)) {
    return 0;
  }

  HMENU menu = LoadMenuW(instance, MAKEINTRESOURCEW(IDC_MAINMENU));
  if (!menu) {
    menu = CreateMenu();
  }

  optiscaler::AppState state;
  HWND hwnd = CreateWindowExW(0, optiscaler::kWindowClass, L"OptiScaler Manager Lite", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, menu, instance, &state);
  if (!hwnd) {
    return 0;
  }
  ShowWindow(hwnd, cmd_show);
  UpdateWindow(hwnd);

  MSG msg = {};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}
