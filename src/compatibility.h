#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace optiscaler {

enum class InstallDll { kDxgi, kWinmm, kDinput8, kOther };

struct CompatSettings {
  std::map<std::wstring, std::wstring> requiredIni;
  std::map<std::wstring, std::wstring> fgIni;
};

struct CompatNotes {
  bool requiresFakenvapi = false;
  bool optiFgSupported = true;
  std::vector<std::wstring> overlaysToDisable;
  std::vector<std::wstring> knownIssues;
  std::vector<std::wstring> extraSteps;
};

struct CompatibilityProfile {
  std::wstring pageName;
  std::wstring gameName;
  std::optional<uint32_t> steamAppId;
  InstallDll dll = InstallDll::kDxgi;
  std::vector<std::wstring> inputs;
  std::vector<std::wstring> exeHints;
  std::vector<std::wstring> folderHints;
  std::wstring testedVersion;
  std::wstring os;
  std::wstring gpu;
  CompatSettings settings;
  CompatNotes notes;
  std::vector<std::wstring> tags;
  std::wstring lastFetchedUtc;
  std::vector<std::wstring> parseWarnings;
};

struct CompatibilityCacheMeta {
  std::wstring fetchedUtc;
  std::wstring sourceUrl;
  std::wstring etag;
  std::wstring lastModified;
  size_t count = 0;
};

struct CompatibilityCache {
  CompatibilityCacheMeta meta;
  std::vector<CompatibilityProfile> profiles;
};

std::wstring CompatibilityCacheDirectory();
std::wstring CompatibilityCachePath();
std::wstring CompatibilityMetaPath();
std::wstring CompatibilityOverridesPath();
std::wstring CompatibilityLogPath();

bool LoadCompatibilityCache(CompatibilityCache& cache);
bool SaveCompatibilityCache(const CompatibilityCache& cache);
bool LoadCompatibilityOverrides(std::vector<CompatibilityProfile>& overrides);
bool LoadCompatibilityMeta(CompatibilityCacheMeta& meta);
bool SaveCompatibilityMeta(const CompatibilityCacheMeta& meta);
bool ShouldPromptCompatibilityRefresh(const CompatibilityCacheMeta& meta, int maxAgeDays);

bool RefreshCompatibilityFromNetwork(const std::wstring& indexUrl, CompatibilityCache& cache,
                                     std::vector<std::wstring>& logLines, std::wstring& statusMessage,
                                     bool allowCachedHeaders);

std::wstring InstallDllToFileName(InstallDll dll);
InstallDll InstallDllFromFileName(const std::wstring& name);
std::wstring NormalizeToken(const std::wstring& value);
std::wstring NormalizeExeName(const std::wstring& path);

const CompatibilityProfile* MatchCompatibilityProfile(const CompatibilityCache& cache,
                                                      const std::wstring& exePath,
                                                      const std::wstring& gameName,
                                                      std::optional<uint32_t> steamAppId);

void MergeOverrides(std::vector<CompatibilityProfile>& baseProfiles,
                    const std::vector<CompatibilityProfile>& overrides,
                    std::vector<std::wstring>& logLines);

void LogCompatibilityEvent(const std::vector<std::wstring>& lines);

}  // namespace optiscaler

