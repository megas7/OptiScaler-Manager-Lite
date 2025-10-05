#include "scanner.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <unordered_set>

#include <windows.h>

namespace optiscaler {

namespace {

constexpr size_t kMaxScanDepth = 4;

std::wstring ToLower(const std::wstring& value) {
  std::wstring lower(value);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t ch) {
    return static_cast<wchar_t>(std::towlower(ch));
  });
  return lower;
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
      games.emplace_back(std::move(game));
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
    AppendIfExists(roots, program_files_x86 + L"\\Steam\\steamapps\\common");
  }
  if (!program_files.empty()) {
    AppendIfExists(roots, program_files + L"\\Steam\\steamapps\\common");
    AppendIfExists(roots, program_files + L"\\Epic Games");
    AppendIfExists(roots, program_files + L"\\ModifiableWindowsApps");
  }
  if (!program_data.empty()) {
    AppendIfExists(roots, program_data + L"\\Microsoft\\Windows\\Start Menu\\Programs");
  }

  std::sort(roots.begin(), roots.end());
  roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
  return roots;
}

}  // namespace optiscaler
