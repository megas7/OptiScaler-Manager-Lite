#include "scanner.h"

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

#include <windows.h>

namespace optiscaler {

namespace {

constexpr size_t kMaxScanDepth = 4;

struct SteamInstallInfo {
  uint32_t app_id = 0;
  std::wstring name;
  std::wstring folder;
  std::wstring normalized_prefix;
};

std::wstring ToLower(const std::wstring& value) {
  std::wstring lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t ch) {
    return static_cast<wchar_t>(std::towlower(ch));
  });
  return lower;
}

std::wstring NormalizeForComparison(const std::filesystem::path& path,
                                    bool append_separator) {
  std::wstring normalized = path.lexically_normal().native();
  std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
  normalized = ToLower(normalized);
  if (append_separator && !normalized.empty() && normalized.back() != L'\\') {
    normalized.push_back(L'\\');
  }
  return normalized;
}

bool ShouldSkipExecutable(const std::wstring& filename_lower) {
  constexpr const wchar_t* kPrefixes[] = {
      L"unins",      L"uninstall",  L"setup",        L"dxsetup",   L"vc_redist",
      L"vcredist",   L"helper",     L"crashreport",  L"readme",    L"support",
      L"patch",      L"update",     L"redistributable"};
  for (const auto* prefix : kPrefixes) {
    if (filename_lower.rfind(prefix, 0) == 0) {
      return true;
    }
  }

  constexpr const wchar_t* kSubstrings[] = {
      L"crashhandler", L"crashreporter", L"unitycrash", L"eac_launcher", L"unrealcefsubprocess"};
  for (const auto* substring : kSubstrings) {
    if (filename_lower.find(substring) != std::wstring::npos) {
      return true;
    }
  }
  return false;
}

bool ShouldSkipPath(const std::filesystem::path& path) {
  std::wstring path_lower = ToLower(path.lexically_normal().native());
  constexpr const wchar_t* kIgnored[] = {
      L"\\_commonredist\\", L"\\commonredist\\", L"\\redist\\", L"\\redistributable\\",
      L"\\support\\",       L"\\vc_redist\\",   L"\\tools\\",  L"\\extras\\"};
  for (const auto* needle : kIgnored) {
    if (path_lower.find(needle) != std::wstring::npos) {
      return true;
    }
  }
  return false;
}

std::wstring GuessSource(const std::wstring& root) {
  const std::wstring lower = ToLower(root);
  if (lower.find(L"steam") != std::wstring::npos) {
    return L"steam";
  }
  if (lower.find(L"epic") != std::wstring::npos) {
    return L"epic";
  }
  if (lower.find(L"xbox") != std::wstring::npos || lower.find(L"microsoft") != std::wstring::npos) {
    return L"xbox";
  }
  return L"custom";
}

std::wstring DisplayNameFromStem(std::wstring stem) {
  for (auto& ch : stem) {
    if (ch == L'_' || ch == L'-') {
      ch = L' ';
    }
  }
  return stem;
}

  std::error_code ec;
  for (const auto& root : roots) {
    if (root.empty()) {
      continue;
    }
    std::filesystem::path root_path(root);
    if (!std::filesystem::exists(root_path, ec)) {
      ec.clear();
      continue;
    }
    ec.clear();
    const std::wstring source = GuessSource(root);
    std::vector<SteamInstallInfo> steam_installs;
    if (source == L"steam") {
      std::filesystem::path steamapps_dir = ResolveSteamAppsDir(root_path);
      if (!steamapps_dir.empty()) {
        steam_installs = LoadSteamManifests(steamapps_dir);
      }
    }
    std::filesystem::recursive_directory_iterator it(
        root_path, std::filesystem::directory_options::skip_permission_denied, ec);
    if (ec) {
      ec.clear();
      continue;
    }
    std::filesystem::recursive_directory_iterator end;
    while (it != end) {
      if (ec) {
        ec.clear();
        it.increment(ec);
        continue;
      }
      if (static_cast<size_t>(it.depth()) > kMaxScanDepth) {
        it.disable_recursion_pending();
        it.increment(ec);
        continue;
      }
      const auto& entry = *it;
      if (!entry.is_regular_file(ec)) {
        if (ec) {
          ec.clear();
        }
        it.increment(ec);
        continue;
      }

      const std::filesystem::path& file_path = entry.path();
      if (ShouldSkipPath(file_path.parent_path())) {
        it.increment(ec);
        continue;
      }
      std::wstring extension = ToLower(file_path.extension().wstring());
      if (extension != L".exe") {
        it.increment(ec);
        continue;
      }

      std::wstring filename_lower = ToLower(file_path.filename().wstring());
      if (ShouldSkipExecutable(filename_lower)) {
        it.increment(ec);
        continue;
      }

      std::filesystem::path absolute = std::filesystem::absolute(file_path, ec);
      if (ec) {
        ec.clear();
        it.increment(ec);
        continue;
      }
      absolute = absolute.lexically_normal();
      std::wstring normalized_lower = ToLower(absolute.native());
      if (!seen_paths.insert(normalized_lower).second) {
        it.increment(ec);
        continue;
      }

      GameEntry game;
      game.exe = absolute.wstring();
      game.folder = absolute.parent_path().wstring();
      game.name = DisplayNameFromStem(file_path.stem().wstring());
      game.source = source;

      std::wstring install_prefix_key;
      int install_score = 0;
      const SteamInstallInfo* matched_install = nullptr;
      if (!steam_installs.empty()) {
        matched_install = MatchSteamInstall(steam_installs, absolute);
        if (matched_install && !matched_install->normalized_prefix.empty()) {
          install_prefix_key = matched_install->normalized_prefix;
          install_score = ScoreExecutableForInstall(absolute, *matched_install);
          auto existing = steam_install_index.find(install_prefix_key);
          if (existing != steam_install_index.end()) {
            int previous_score = steam_install_scores[install_prefix_key];
            if (install_score > previous_score) {
              GameEntry& existing_game = games[existing->second];
              existing_game.exe = absolute.wstring();
              existing_game.folder = matched_install->folder;
              if (!matched_install->name.empty()) {
                existing_game.name = matched_install->name;
              }
              existing_game.steamAppId = matched_install->app_id;
              steam_install_scores[install_prefix_key] = install_score;
            }
            it.increment(ec);
            continue;
          }
        }
      }

      if (matched_install) {
        game.folder = matched_install->folder;
        if (!matched_install->name.empty()) {
          game.name = matched_install->name;
        }
        game.steamAppId = matched_install->app_id;
      }

      games.emplace_back(std::move(game));
      if (!install_prefix_key.empty()) {
        steam_install_index[install_prefix_key] = games.size() - 1;
        steam_install_scores[install_prefix_key] = install_score;
      }
      it.increment(ec);
    }
  }

  std::sort(games.begin(), games.end(), [](const GameEntry& a, const GameEntry& b) {
    const std::wstring lower_a = ToLower(a.name);
    const std::wstring lower_b = ToLower(b.name);
    if (lower_a == lower_b) {
      return a.exe < b.exe;
    }
    return lower_a < lower_b;
  });

  return games;
}

std::vector<std::wstring> Scanner::DefaultFolders() {
  std::vector<std::wstring> roots;
  const std::wstring program_files_x86 = GetEnvVar(L"ProgramFiles(x86)");
  std::wstring program_files = GetEnvVar(L"ProgramW6432");
  if (program_files.empty()) {
    program_files = GetEnvVar(L"ProgramFiles");
  }
  const std::wstring program_data = GetEnvVar(L"ProgramData");

  if (!program_files_x86.empty()) {
    AddSteamLibrariesFrom(roots, std::filesystem::path(program_files_x86) / "Steam");
  }
  if (!program_files.empty()) {
    AddSteamLibrariesFrom(roots, std::filesystem::path(program_files) / "Steam");
    AppendIfExists(roots, (std::filesystem::path(program_files) / "Epic Games").wstring());
    AppendIfExists(roots, (std::filesystem::path(program_files) / "ModifiableWindowsApps").wstring());
  }
  if (!program_data.empty()) {
    AppendIfExists(roots, program_data + L"\\Microsoft\\Windows\\Start Menu\\Programs");
  }

  std::sort(roots.begin(), roots.end());
  roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
  return roots;
}

}  // namespace optiscaler
