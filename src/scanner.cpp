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

std::wstring ReadFileUtf8(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return {};
  }
  std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  if (buffer.size() >= 3 && static_cast<unsigned char>(buffer[0]) == 0xEF &&
      static_cast<unsigned char>(buffer[1]) == 0xBB &&
      static_cast<unsigned char>(buffer[2]) == 0xBF) {
    buffer.erase(buffer.begin(), buffer.begin() + 3);
  }
  if (buffer.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, buffer.data(), static_cast<int>(buffer.size()), nullptr, 0);
  if (count <= 0) {
    return {};
  }
  std::wstring wide(static_cast<size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, buffer.data(), static_cast<int>(buffer.size()), wide.data(), count);
  return wide;
}

std::vector<std::wstring> ExtractQuotedTokens(const std::wstring& text) {
  std::vector<std::wstring> tokens;
  std::wstring current;
  bool in_quote = false;
  bool escape = false;
  for (wchar_t ch : text) {
    if (!in_quote) {
      if (ch == L'"') {
        in_quote = true;
        current.clear();
        escape = false;
      }
      continue;
    }

    if (escape) {
      current.push_back(ch);
      escape = false;
      continue;
    }

    if (ch == L'\\') {
      escape = true;
      continue;
    }

    if (ch == L'"') {
      tokens.push_back(current);
      in_quote = false;
      continue;
    }

    current.push_back(ch);
  }
  return tokens;
}

std::wstring UnescapeBackslashes(std::wstring value) {
  std::wstring result;
  result.reserve(value.size());
  bool escape = false;
  for (wchar_t ch : value) {
    if (escape) {
      result.push_back(ch);
      escape = false;
      continue;
    }
    if (ch == L'\\') {
      escape = true;
      continue;
    }
    result.push_back(ch);
  }
  if (escape) {
    result.push_back(L'\\');
  }
  return result;
}

int ScoreExecutableForInstall(const std::filesystem::path& exe_path,
                              const SteamInstallInfo& install) {
  int score = 0;
  std::wstring filename_lower = ToLower(exe_path.filename().wstring());
  if (filename_lower.find(L"win64") != std::wstring::npos ||
      filename_lower.find(L"x64") != std::wstring::npos) {
    score += 40;
  }
  if (filename_lower.find(L"shipping") != std::wstring::npos ||
      filename_lower.find(L"game") != std::wstring::npos) {
    score += 15;
  }
  if (filename_lower.find(L"win32") != std::wstring::npos ||
      filename_lower.find(L"x86") != std::wstring::npos) {
    score -= 10;
  }
  if (filename_lower.find(L"launcher") != std::wstring::npos) {
    score -= 40;
  }
  if (filename_lower.find(L"steamservice") != std::wstring::npos) {
    score -= 60;
  }
  std::wstring install_dir_name =
      ToLower(std::filesystem::path(install.folder).filename().wstring());
  if (!install_dir_name.empty() && filename_lower.find(install_dir_name) != std::wstring::npos) {
    score += 25;
  }

  std::filesystem::path install_path(install.folder);
  std::error_code ec;
  std::filesystem::path relative = std::filesystem::relative(exe_path, install_path, ec);
  if (!ec) {
    int depth = 0;
    for (const auto& part : relative) {
      (void)part;
      ++depth;
    }
    if (depth > 0) {
      --depth;  // exclude the executable itself
    }
    score -= depth * 5;
  }
  return score;
}

std::vector<std::filesystem::path> ParseSteamLibraryFolders(const std::filesystem::path& library_file) {
  std::vector<std::filesystem::path> results;
  std::error_code ec;
  if (!std::filesystem::exists(library_file, ec)) {
    return results;
  }
  std::wstring content = ReadFileUtf8(library_file);
  if (content.empty()) {
    return results;
  }
  std::vector<std::wstring> tokens = ExtractQuotedTokens(content);
  std::vector<std::wstring> raw_paths;
  for (size_t i = 0; i < tokens.size(); ++i) {
    const std::wstring& token = tokens[i];
    if (token == L"path") {
      if (i + 1 < tokens.size()) {
        raw_paths.push_back(UnescapeBackslashes(tokens[++i]));
      }
      continue;
    }
    bool numeric = !token.empty();
    for (wchar_t ch : token) {
      if (!std::iswdigit(ch)) {
        numeric = false;
        break;
      }
    }
    if (numeric && i + 1 < tokens.size()) {
      raw_paths.push_back(UnescapeBackslashes(tokens[++i]));
    }
  }

  for (const auto& raw : raw_paths) {
    if (raw.empty()) {
      continue;
    }
    std::filesystem::path base(raw);
    std::filesystem::path common = base / "steamapps" / "common";
    results.push_back(common);
  }
  return results;
}

std::vector<SteamInstallInfo> LoadSteamManifests(const std::filesystem::path& steamapps_dir) {
  std::vector<SteamInstallInfo> installs;
  std::error_code ec;
  if (!std::filesystem::exists(steamapps_dir, ec) ||
      !std::filesystem::is_directory(steamapps_dir, ec)) {
    return installs;
  }
  std::filesystem::path common_dir = steamapps_dir / "common";
  std::filesystem::directory_iterator dir(steamapps_dir, ec);
  if (ec) {
    return installs;
  }

  for (const auto& entry : dir) {
    if (!entry.is_regular_file(ec)) {
      continue;
    }
    const std::filesystem::path& manifest_path = entry.path();
    const std::wstring filename_lower = ToLower(manifest_path.filename().wstring());
    if (filename_lower.rfind(L"appmanifest_", 0) != 0 || manifest_path.extension() != L".acf") {
      continue;
    }

    std::wstring content = ReadFileUtf8(manifest_path);
    if (content.empty()) {
      continue;
    }
    std::vector<std::wstring> tokens = ExtractQuotedTokens(content);
    SteamInstallInfo info;
    bool have_app_id = false;
    bool have_install_dir = false;
    for (size_t i = 0; i < tokens.size(); ++i) {
      const std::wstring& key = tokens[i];
      if (key == L"appid" && i + 1 < tokens.size()) {
        info.app_id = static_cast<uint32_t>(std::wcstoul(tokens[i + 1].c_str(), nullptr, 10));
        have_app_id = true;
        ++i;
        continue;
      }
      if (key == L"name" && i + 1 < tokens.size()) {
        info.name = tokens[i + 1];
        ++i;
        continue;
      }
      if (key == L"installdir" && i + 1 < tokens.size()) {
        std::filesystem::path install_path = common_dir / tokens[i + 1];
        std::error_code install_ec;
        if (std::filesystem::exists(install_path, install_ec)) {
          info.folder = install_path.lexically_normal().wstring();
          info.normalized_prefix = NormalizeForComparison(install_path, true);
          have_install_dir = true;
        }
        ++i;
        continue;
      }
    }
    if (have_app_id && have_install_dir) {
      installs.push_back(std::move(info));
    }
  }
  return installs;
}

const SteamInstallInfo* MatchSteamInstall(const std::vector<SteamInstallInfo>& installs,
                                         const std::filesystem::path& exe_path) {
  if (installs.empty()) {
    return nullptr;
  }
  std::wstring exe_normalized = NormalizeForComparison(exe_path, true);
  for (const auto& install : installs) {
    if (!install.normalized_prefix.empty() &&
        exe_normalized.rfind(install.normalized_prefix, 0) == 0) {
      return &install;
    }
  }
  return nullptr;
}

std::filesystem::path ResolveSteamAppsDir(const std::filesystem::path& root_path) {
  std::error_code ec;
  std::filesystem::path candidate;
  const std::wstring filename_lower = ToLower(root_path.filename().wstring());
  if (filename_lower == L"common") {
    std::filesystem::path parent = root_path.parent_path();
    if (ToLower(parent.filename().wstring()) == L"steamapps") {
      candidate = parent;
    }
  }
  if (candidate.empty()) {
    std::filesystem::path direct = root_path / "steamapps";
    if (std::filesystem::exists(direct, ec) && std::filesystem::is_directory(direct, ec)) {
      candidate = direct;
    }
  }
  if (candidate.empty()) {
    std::filesystem::path parent = root_path.parent_path() / "steamapps";
    if (std::filesystem::exists(parent, ec) && std::filesystem::is_directory(parent, ec)) {
      candidate = parent;
    }
  }
  return candidate;
}

void AddSteamLibrariesFrom(std::vector<std::wstring>& roots, const std::filesystem::path& steam_root) {
  if (steam_root.empty()) {
    return;
  }
  std::filesystem::path steamapps = steam_root / "steamapps";
  AppendIfExists(roots, (steamapps / "common").wstring());
  std::vector<std::filesystem::path> libraries =
      ParseSteamLibraryFolders(steamapps / "libraryfolders.vdf");
  for (const auto& library : libraries) {
    AppendIfExists(roots, library.lexically_normal().wstring());
  }
}

void AppendIfExists(std::vector<std::wstring>& roots, std::wstring path) {
  if (path.empty()) {
    return;
  }
  std::filesystem::path fs_path(std::move(path));
  std::error_code ec;
  if (std::filesystem::exists(fs_path, ec)) {
    roots.emplace_back(fs_path.lexically_normal().wstring());
  }
}

std::wstring GetEnvVar(const wchar_t* name) {
  const DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
  if (needed <= 1) {
    return {};
  }
  std::wstring value;
  value.resize(needed - 1);
  DWORD written = GetEnvironmentVariableW(name, value.data(), needed);
  if (written == 0) {
    return {};
  }
  value.resize(written);
  return value;
}

}  // namespace

std::vector<GameEntry> Scanner::ScanAll(const std::vector<std::wstring>& roots) {
  std::vector<GameEntry> games;
  std::unordered_set<std::wstring> seen_paths;
  std::unordered_map<std::wstring, size_t> steam_install_index;
  std::unordered_map<std::wstring, int> steam_install_scores;
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
