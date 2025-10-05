#include "settings.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <sstream>

#include "cache.h"

namespace optiscaler {

namespace {

std::wstring Trim(const std::wstring& value) {
  size_t start = 0;
  while (start < value.size() && iswspace(value[start])) {
    ++start;
  }
  size_t end = value.size();
  while (end > start && iswspace(value[end - 1])) {
    --end;
  }
  return value.substr(start, end - start);
}

std::wstring ToLower(const std::wstring& value) {
  std::wstring lower(value.size(), L'\0');
  std::transform(value.begin(), value.end(), lower.begin(), [](wchar_t ch) {
    return static_cast<wchar_t>(towlower(ch));
  });
  return lower;
}

RendererPreference ParseRendererPreference(const std::wstring& value) {
  const std::wstring lower = ToLower(value);
  if (lower == L"d3d12") {
    return RendererPreference::kD3D12;
  }
  if (lower == L"d3d11") {
    return RendererPreference::kD3D11;
  }
  if (lower == L"gdi") {
    return RendererPreference::kGDI;
  }
  return RendererPreference::kAuto;
}

std::wstring RendererPreferenceToString(RendererPreference pref) {
  switch (pref) {
    case RendererPreference::kD3D12:
      return L"d3d12";
    case RendererPreference::kD3D11:
      return L"d3d11";
    case RendererPreference::kGDI:
      return L"gdi";
    case RendererPreference::kAuto:
    default:
      return L"auto";
  }
}

bool ParseBool(const std::wstring& value) {
  const std::wstring lower = ToLower(value);
  return lower == L"1" || lower == L"true" || lower == L"yes" || lower == L"on";
}

std::wstring DirectoryFromPath(const std::wstring& file_path) {
  if (file_path.empty()) {
    return {};
  }
  std::filesystem::path path(file_path);
  path.remove_filename();
  return path.wstring();
}

}  // namespace

std::wstring Settings::FilePath() {
  const std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return {};
  }
  Cache::EnsureDirectory(root);
  std::filesystem::path path(root);
  path /= L"settings.ini";
  return path.wstring();
}

bool Settings::Load(SettingsData& data) {
  data = SettingsData{};
  const std::wstring path = FilePath();
  if (path.empty()) {
    return false;
  }
  Cache::EnsureDirectory(DirectoryFromPath(path));

  std::wstring content;
  if (!Cache::ReadText(path, content)) {
    return Save(data);
  }

  std::wistringstream stream(content);
  std::wstring line;
  while (std::getline(stream, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == L'#') {
      continue;
    }
    const size_t eq = line.find(L'=');
    if (eq == std::wstring::npos) {
      continue;
    }
    const std::wstring key = Trim(line.substr(0, eq));
    const std::wstring value = Trim(line.substr(eq + 1));
    if (key == L"optiDir") {
      data.optiScalerDirectory = value;
    } else if (key == L"autoUpdate") {
      data.autoUpdate = ParseBool(value);
    } else if (key == L"renderer") {
      data.rendererPreference = ParseRendererPreference(value);
    } else if (key == L"igdbClientId") {
      data.igdbClientId = value;
    } else if (key == L"igdbSecret") {
      data.igdbClientSecret = value;
    } else if (key == L"igdbToken") {
      data.igdbToken = value;
    } else if (key == L"igdbTokenExpiresUtc") {
      data.igdbTokenExpiresUtc = value;
    } else if (key.rfind(L"folder+", 0) == 0) {
      data.customScanFolders.push_back(value);
    }
  }

  auto& folders = data.customScanFolders;
  folders.erase(std::remove_if(folders.begin(), folders.end(), [](const std::wstring& entry) {
                    return Trim(entry).empty();
                  }),
                folders.end());
  std::sort(folders.begin(), folders.end());
  folders.erase(std::unique(folders.begin(), folders.end()), folders.end());

  return true;
}

bool Settings::Save(const SettingsData& data) {
  const std::wstring path = FilePath();
  if (path.empty()) {
    return false;
  }
  Cache::EnsureDirectory(DirectoryFromPath(path));

  std::wostringstream stream;
  stream << L"optiDir=" << data.optiScalerDirectory << L"\n";
  stream << L"autoUpdate=" << (data.autoUpdate ? L"1" : L"0") << L"\n";
  stream << L"renderer=" << RendererPreferenceToString(data.rendererPreference) << L"\n";
  stream << L"igdbClientId=" << data.igdbClientId << L"\n";
  stream << L"igdbSecret=" << data.igdbClientSecret << L"\n";
  stream << L"igdbToken=" << data.igdbToken << L"\n";
  stream << L"igdbTokenExpiresUtc=" << data.igdbTokenExpiresUtc << L"\n";
  for (const auto& folder : data.customScanFolders) {
    stream << L"folder+" << folder << L"\n";
  }

  return Cache::WriteText(path, stream.str());
}

}  // namespace optiscaler

