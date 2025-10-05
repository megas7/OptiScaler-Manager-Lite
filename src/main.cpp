#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <objbase.h>
#include <shlobj.h>
#include <winver.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cwctype>
#include <cwchar>

#include "cover_cache.h"
#include "game_types.h"
#include "igdb.h"
#include "launcher.h"
#include "renderer_factory.h"
#include "resource.h"
#include "scanner.h"
#include "systeminfo.h"

#pragma comment(lib, "comctl32.lib")

namespace optiscaler {

namespace {

constexpr int kCoverWidth = 200;
constexpr int kCoverHeight = 300;
constexpr int kTileSpacing = 24;
constexpr int kTileTextHeight = 64;
constexpr int kGridMargin = 24;
constexpr int kGridTopOffset = 120;

struct SupportInfo {
  std::wstring official;
  std::wstring notes;
};

std::wstring Trim(const std::wstring& value) {
  size_t begin = 0;
  while (begin < value.size() && std::iswspace(value[begin])) {
    ++begin;
  }
  size_t end = value.size();
  while (end > begin && std::iswspace(value[end - 1])) {
    --end;
  }
  return value.substr(begin, end - begin);
}

std::wstring NormalizePathLower(std::wstring path) {
  std::replace(path.begin(), path.end(), L'/', L'\\');
  std::transform(path.begin(), path.end(), path.begin(), [](wchar_t ch) {
    return static_cast<wchar_t>(std::towlower(ch));
  });
  return path;
}

COLORREF AccentColorForKey(const std::wstring& key) {
  std::hash<std::wstring> hasher;
  size_t h = hasher(key);
  BYTE r = static_cast<BYTE>(80 + (h & 0x7F));
  BYTE g = static_cast<BYTE>(80 + ((h >> 8) & 0x7F));
  BYTE b = static_cast<BYTE>(80 + ((h >> 16) & 0x7F));
  return RGB(r, g, b);
}

HBITMAP CreatePlaceholderCover(const std::wstring& title, COLORREF accent) {
  HDC screen_dc = GetDC(nullptr);
  if (!screen_dc) {
    return nullptr;
  }
  HDC memory_dc = CreateCompatibleDC(screen_dc);
  if (!memory_dc) {
    ReleaseDC(nullptr, screen_dc);
    return nullptr;
  }
  HBITMAP bitmap = CreateCompatibleBitmap(screen_dc, kCoverWidth, kCoverHeight);
  if (!bitmap) {
    DeleteDC(memory_dc);
    ReleaseDC(nullptr, screen_dc);
    return nullptr;
  }
  HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
  RECT rc = {0, 0, kCoverWidth, kCoverHeight};
  HBRUSH brush = CreateSolidBrush(accent);
  FillRect(memory_dc, &rc, brush);
  DeleteObject(brush);

  SetBkMode(memory_dc, TRANSPARENT);
  SetTextColor(memory_dc, RGB(255, 255, 255));
  HFONT font = CreateFontW(42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
  HGDIOBJ old_font = nullptr;
  if (font) {
    old_font = SelectObject(memory_dc, font);
  }
  RECT text_rc = {16, 16, kCoverWidth - 16, kCoverHeight - 16};
  std::wstring text = title;
  if (text.length() > 48) {
    text = text.substr(0, 47) + L"…";
  }
  DrawTextW(memory_dc, text.c_str(), static_cast<int>(text.length()), &text_rc,
            DT_CENTER | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX);
  if (old_font) {
    SelectObject(memory_dc, old_font);
  }
  if (font) {
    DeleteObject(font);
  }
  SelectObject(memory_dc, old_bitmap);
  DeleteDC(memory_dc);
  ReleaseDC(nullptr, screen_dc);
  return bitmap;
}

void EnsureGameCover(GameEntry& game) {
  if (game.coverBmp) {
    return;
  }
  game.coverBmp = CreatePlaceholderCover(game.name.empty() ? game.exe : game.name, AccentColorForKey(game.exe));
}

std::wstring SanitizeFileComponent(std::wstring name) {
  constexpr const wchar_t* invalid = L"<>:\"/\\|?*";
  for (auto& ch : name) {
    if (wcschr(invalid, ch)) {
      ch = L'_';
    }
  }
  return name;
}

std::vector<std::wstring> BuildDefaultInjectionPlan(const GameEntry& game, const std::wstring& opti_dir) {
  std::vector<std::wstring> plan;
  if (!opti_dir.empty()) {
    plan.push_back(L"Using override content from " + opti_dir);
  }
  plan.push_back(L"OptiScaler64.dll → " + game.folder);
  plan.push_back(L"OptiScalerSettings.json → " + game.folder + L"\\OptiScalerSettings.json");
  std::wstring profile_name = SanitizeFileComponent(game.name.empty() ? L"profile" : game.name);
  plan.push_back(L"profiles\\" + profile_name + L".json → " + game.folder + L"\\OptiScalerProfile.json");
  if (game.source == L"steam" && game.steamAppId) {
    plan.push_back(L"steam_appid.txt → " + std::to_wstring(*game.steamAppId));
  }
  if (game.source == L"epic") {
    plan.push_back(L"Epic manifest hook → OptiScalerLauncher.exe");
  }
  return plan;
}

SupportInfo DetermineSupportInfo(const GameEntry& game) {
  SupportInfo info;
  if (game.source == L"steam") {
    info.official = L"Steam build with OptiScaler overlay";
    info.notes = L"DX11/DX12 hooks, Steam Cloud, FSR2/FSR3 toggles supported.";
  } else if (game.source == L"epic") {
    info.official = L"Epic Games Store build";
    info.notes = L"OptiScaler injects via Epic launcher manifest; supports DLSS/FSR and HDR.";
  } else if (game.source == L"xbox") {
    info.official = L"Microsoft Store / Xbox edition";
    info.notes = L"File-based injection only; launching remains read-only.";
  } else {
    info.official = L"Custom folder";
    info.notes = L"Manual injection available with OptiScaler runtime and config mapping.";
  }
  return info;
}

std::wstring QueryFileVersionString(const std::wstring& path) {
  DWORD handle = 0;
  DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
  if (size == 0) {
    return {};
  }
  std::vector<uint8_t> data(size);
  if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) {
    return {};
  }
  VS_FIXEDFILEINFO* info = nullptr;
  UINT length = 0;
  if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &length) || !info) {
    return {};
  }
  std::wstringstream stream;
  stream << HIWORD(info->dwFileVersionMS) << L'.' << LOWORD(info->dwFileVersionMS) << L'.'
         << HIWORD(info->dwFileVersionLS) << L'.' << LOWORD(info->dwFileVersionLS);
  return stream.str();
}

std::wstring JoinLines(const std::vector<std::wstring>& lines) {
  std::wstring joined;
  for (size_t i = 0; i < lines.size(); ++i) {
    joined.append(lines[i]);
    if (i + 1 < lines.size()) {
      joined.append(L"\r\n");
    }
  }
  return joined;
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
  std::vector<std::wstring> lines;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find_first_of(L"\r\n", pos);
    std::wstring line = text.substr(pos, end == std::wstring::npos ? std::wstring::npos : end - pos);
    line = Trim(line);
    if (!line.empty()) {
      lines.push_back(line);
    }
    if (end == std::wstring::npos) {
      break;
    }
    pos = end + 1;
    if (pos < text.size() && text[end] == L'\r' && text[pos] == L'\n') {
      ++pos;
    }
  }
  return lines;
}

void ReleaseGameResources(std::vector<GameEntry>& games) {
  for (auto& game : games) {
    if (game.coverBmp) {
      DeleteObject(game.coverBmp);
      game.coverBmp = nullptr;
    }
  }
}

int ComputeColumnsForClientWidth(int width) {
  int available = width - 2 * kGridMargin;
  int stride = kCoverWidth + kTileSpacing;
  if (available < stride) {
    return 1;
  }
  int columns = available / stride;
  return std::max(1, columns);
}

int ComputeGridLeft(int client_width, int columns) {
  int grid_width = columns * (kCoverWidth + kTileSpacing) - kTileSpacing;
  if (grid_width < 0) {
    grid_width = kCoverWidth;
  }
  int centered = (client_width - grid_width) / 2;
  return std::max(kGridMargin, centered);
}

size_t ComputeRowsForGames(size_t game_count, int columns) {
  if (columns <= 0) {
    return game_count;
  }
  return (game_count + static_cast<size_t>(columns) - 1) / static_cast<size_t>(columns);
}

}  // namespace

constexpr wchar_t kWindowClass[] = L"OptiScalerMgrLiteWindow";

struct AppState {
  std::vector<GameEntry> games;
  size_t selected_index = 0;
  std::unique_ptr<IRenderer> renderer;
  HWND status_bar = nullptr;
  bool global_injection_enabled = true;
  std::wstring optiscaler_override_dir;
  SystemInfoData system_info;
};

RendererPreference ParseRendererPreference() {
  // TODO: Load renderer preference from persisted settings.
  return RendererPreference::kGDI;
}

void UpdateStatusBar(AppState* state, const std::wstring& text) {
  if (state && state->status_bar) {
    SendMessageW(state->status_bar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
  }
}

void UpdateSelectionStatus(AppState* state) {
  if (!state || state->games.empty() || state->selected_index >= state->games.size()) {
    return;
  }
  const GameEntry& game = state->games[state->selected_index];
  std::wstring status = game.name.empty() ? game.exe : game.name;
  if (!game.officialSupport.empty()) {
    status.append(L" — ");
    status.append(game.officialSupport);
  }
  status.append(L" | Injection ");
  status.append(game.injectEnabled ? L"on" : L"off");
  UpdateStatusBar(state, status);
}

void PerformScan(HWND hwnd, AppState* state) {
  if (!state) {
    return;
  }

  UpdateStatusBar(state, L"Scanning for games...");
  auto roots = Scanner::DefaultFolders();
  std::vector<GameEntry> scanned = Scanner::ScanAll(roots);
  std::wstring previous_selected_path;
  if (state->selected_index < state->games.size()) {
    previous_selected_path = NormalizePathLower(state->games[state->selected_index].exe);
  }

  std::unordered_map<std::wstring, std::pair<bool, std::vector<std::wstring>>> previous;
  previous.reserve(state->games.size());
  for (auto& game : state->games) {
    previous.emplace(NormalizePathLower(game.exe), std::make_pair(game.injectEnabled, game.plannedFiles));
  }
  ReleaseGameResources(state->games);
  state->games.clear();
  state->games.reserve(scanned.size());

  size_t matched_index = static_cast<size_t>(-1);
  for (auto& game : scanned) {
    const std::wstring key = NormalizePathLower(game.exe);
    auto found = previous.find(key);
    if (found != previous.end()) {
      game.injectEnabled = found->second.first;
      if (!found->second.second.empty()) {
        game.plannedFiles = found->second.second;
      }
    } else {
      game.injectEnabled = state->global_injection_enabled;
    }
    if (game.plannedFiles.empty()) {
      game.plannedFiles = BuildDefaultInjectionPlan(game, state->optiscaler_override_dir);
    }
    SupportInfo support = DetermineSupportInfo(game);
    game.officialSupport = support.official;
    game.supportNotes = support.notes;
    if (game.detectedVersion.empty()) {
      std::wstring version = QueryFileVersionString(game.exe);
      if (!version.empty()) {
        game.detectedVersion = L"v" + version;
      } else {
        game.detectedVersion = L"Unknown version";
      }
    }
    EnsureGameCover(game);
    state->games.emplace_back(std::move(game));
    if (!previous_selected_path.empty() && previous_selected_path == key) {
      matched_index = state->games.size() - 1;
    }
  }

  if (state->games.empty()) {
    state->selected_index = 0;
  } else if (matched_index != static_cast<size_t>(-1)) {
    state->selected_index = matched_index;
  } else if (state->selected_index >= state->games.size()) {
    state->selected_index = 0;
  }

  if (state->games.empty()) {
    if (roots.empty()) {
      UpdateStatusBar(state, L"No default scan folders detected. Add custom folders from Settings when available.");
    } else {
      UpdateStatusBar(state, L"No games detected in default folders.");
    }
  } else {
    std::wstring status = L"Found ";
    status.append(std::to_wstring(state->games.size()));
    status.append(state->games.size() == 1 ? L" game" : L" games");
    if (!roots.empty()) {
      status.append(L" across ");
      status.append(std::to_wstring(roots.size()));
      status.append(roots.size() == 1 ? L" folder." : L" folders.");
    } else {
      status.append(L".");
    }
    if (!state->games.empty()) {
      const GameEntry& selected = state->games[state->selected_index];
      status.append(L" Selected: ");
      status.append(selected.name.empty() ? selected.exe : selected.name);
      status.append(L" (Injection ");
      status.append(selected.injectEnabled ? L"on" : L"off");
      status.append(L").");
    }
    UpdateStatusBar(state, status);
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
  state->renderer->Resize(rc.right - rc.left, rc.bottom - rc.top);
  state->renderer->Begin();
  const COLORREF text_color = RGB(240, 240, 240);
  const COLORREF highlight_color = RGB(255, 223, 0);
  const COLORREF secondary_color = RGB(210, 210, 210);
  const COLORREF frame_color = RGB(255, 255, 255);

  state->renderer->DrawText(L"OptiScaler Manager Lite (prototype)", 16, 16, text_color);

  int info_y = 48;
  const SystemInfoData& sys = state->system_info;
  std::wstring gpu_line = L"GPU: ";
  gpu_line.append(sys.gpuName.empty() ? L"Unknown" : sys.gpuName);
  if (sys.gpuMemoryMB != 0) {
    gpu_line.append(L" (" + std::to_wstring(sys.gpuMemoryMB) + L" MB VRAM)");
  }
  state->renderer->DrawText(gpu_line, 16, info_y, text_color);
  info_y += 18;

  std::wstring cpu_line = L"CPU: ";
  cpu_line.append(sys.cpuName.empty() ? L"Unknown" : sys.cpuName);
  uint32_t cores = sys.cpuCores != 0 ? sys.cpuCores : sys.cpuThreads;
  uint32_t threads = sys.cpuThreads != 0 ? sys.cpuThreads : sys.cpuCores;
  if (cores != 0 || threads != 0) {
    cpu_line.append(L" (" + std::to_wstring(cores) + L"C/" + std::to_wstring(threads) + L"T)");
  }
  state->renderer->DrawText(cpu_line, 16, info_y, text_color);
  info_y += 18;

  std::wstring ram_line = L"RAM: ";
  ram_line.append(std::to_wstring(sys.ramMB));
  ram_line.append(L" MB | OS: ");
  ram_line.append(sys.osVersion.empty() ? L"Windows" : sys.osVersion);
  state->renderer->DrawText(ram_line, 16, info_y, text_color);
  info_y += 20;

  std::wstring injection_line = L"Global injection: ";
  injection_line.append(state->global_injection_enabled ? L"enabled" : L"disabled");
  injection_line.append(L" | OptiScaler folder: ");
  if (state->optiscaler_override_dir.empty()) {
    injection_line.append(L"GitHub releases (override in Settings for offline)");
  } else {
    injection_line.append(state->optiscaler_override_dir);
  }
  state->renderer->DrawText(injection_line, 16, info_y, text_color);
  info_y += 28;

  const int client_width = rc.right - rc.left;
  int columns = ComputeColumnsForClientWidth(client_width);
  int grid_left = ComputeGridLeft(client_width, columns);
  int grid_top = std::max(kGridTopOffset, info_y);
  const int tile_stride_y = kCoverHeight + kTileTextHeight + kTileSpacing;
  const int tile_stride_x = kCoverWidth + kTileSpacing;

  if (state->games.empty()) {
    state->renderer->DrawText(L"No games found. Use File → Rescan or add folders from Settings to populate the library.",
                              16, grid_top, text_color);
  } else {
    for (size_t index = 0; index < state->games.size(); ++index) {
      const GameEntry& game = state->games[index];
      size_t row = index / static_cast<size_t>(columns);
      size_t column = index % static_cast<size_t>(columns);
      int x = grid_left + static_cast<int>(column) * tile_stride_x;
      int y = grid_top + static_cast<int>(row) * tile_stride_y;
      if (game.coverBmp) {
        state->renderer->DrawBitmap(game.coverBmp, x, y, kCoverWidth, kCoverHeight);
      }
      if (index == state->selected_index) {
        state->renderer->DrawFrame(x - 2, y - 2, kCoverWidth + 4, kCoverHeight + 4, frame_color, 3);
      }
      int text_y = y + kCoverHeight + 6;
      std::wstring title = game.name.empty() ? game.exe : game.name;
      if (title.length() > 26) {
        title = title.substr(0, 25) + L"…";
      }
      state->renderer->DrawText(title, x, text_y, index == state->selected_index ? highlight_color : text_color);
      text_y += 18;
      std::wstring source_line = L"[";
      source_line.append(game.source.empty() ? L"custom" : game.source);
      source_line.append(L"] ");
      source_line.append(game.detectedVersion.empty() ? L"Unknown build" : game.detectedVersion);
      if (source_line.length() > 32) {
        source_line = source_line.substr(0, 31) + L"…";
      }
      state->renderer->DrawText(source_line, x, text_y, secondary_color);
      text_y += 18;
      std::wstring inject_line = L"Inject: ";
      inject_line.append(game.injectEnabled ? L"on" : L"off");
      state->renderer->DrawText(inject_line, x, text_y, secondary_color);
    }
  }

  if (!state->games.empty() && state->selected_index < state->games.size()) {
    const GameEntry& selected = state->games[state->selected_index];
    size_t rows = ComputeRowsForGames(state->games.size(), columns);
    int details_y = grid_top + static_cast<int>(rows) * tile_stride_y + 12;
    int details_x = grid_left;
    state->renderer->DrawText(L"Selected game details", details_x, details_y, highlight_color);
    details_y += 20;

    std::wstring name_line = L"Name: ";
    name_line.append(selected.name.empty() ? selected.exe : selected.name);
    state->renderer->DrawText(name_line, details_x, details_y, text_color);
    details_y += 18;

    std::wstring folder_line = L"Folder: ";
    folder_line.append(selected.folder);
    state->renderer->DrawText(folder_line, details_x, details_y, secondary_color);
    details_y += 18;

    std::wstring support_line = L"Official support: ";
    support_line.append(selected.officialSupport);
    state->renderer->DrawText(support_line, details_x, details_y, text_color);
    details_y += 18;

    std::wstring notes_line = L"Highlights: ";
    notes_line.append(selected.supportNotes);
    state->renderer->DrawText(notes_line, details_x, details_y, secondary_color);
    details_y += 20;

    state->renderer->DrawText(L"Planned injection preview:", details_x, details_y, text_color);
    details_y += 18;
    size_t to_show = std::min<size_t>(selected.plannedFiles.size(), 6);
    for (size_t i = 0; i < to_show; ++i) {
      std::wstring entry = selected.plannedFiles[i];
      if (entry.length() > 64) {
        entry = entry.substr(0, 63) + L"…";
      }
      state->renderer->DrawText(L"• " + entry, details_x + 16, details_y, secondary_color);
      details_y += 18;
    }
    if (selected.plannedFiles.size() > to_show) {
      std::wstring more = L"+";
      more.append(std::to_wstring(selected.plannedFiles.size() - to_show));
      more.append(L" more items. Open Settings to edit.");
      state->renderer->DrawText(more, details_x + 16, details_y, secondary_color);
    }
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

void SetSelection(HWND hwnd, AppState* state, size_t index) {
  if (!state || index >= state->games.size()) {
    return;
  }
  if (state->selected_index == index) {
    return;
  }
  state->selected_index = index;
  UpdateSelectionStatus(state);
  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }
}

void OnKeyDown(HWND hwnd, AppState* state, WPARAM key) {
  if (!state || state->games.empty()) {
    return;
  }
  RECT rc;
  GetClientRect(hwnd, &rc);
  int columns = ComputeColumnsForClientWidth(rc.right - rc.left);
  if (columns <= 0) {
    columns = 1;
  }
  size_t index = state->selected_index;
  switch (key) {
    case VK_LEFT:
      if (index > 0) {
        --index;
      }
      break;
    case VK_RIGHT:
      if (index + 1 < state->games.size()) {
        ++index;
      }
      break;
    case VK_UP:
      if (index >= static_cast<size_t>(columns)) {
        index -= static_cast<size_t>(columns);
      } else {
        index = 0;
      }
      break;
    case VK_DOWN: {
      size_t candidate = index + static_cast<size_t>(columns);
      if (candidate < state->games.size()) {
        index = candidate;
      } else {
        index = state->games.size() - 1;
      }
      break;
    }
    case VK_HOME:
      index = 0;
      break;
    case VK_END:
      index = state->games.size() - 1;
      break;
    case VK_PRIOR: {
      size_t stride = static_cast<size_t>(columns) * 2;
      if (index >= stride) {
        index -= stride;
      } else {
        index = 0;
      }
      break;
    }
    case VK_NEXT: {
      size_t stride = static_cast<size_t>(columns) * 2;
      size_t candidate = index + stride;
      if (candidate < state->games.size()) {
        index = candidate;
      } else {
        index = state->games.size() - 1;
      }
      break;
    }
    default:
      return;
  }
  SetSelection(hwnd, state, index);
}

void OnLButtonDown(HWND hwnd, AppState* state, int x, int y) {
  if (!state || state->games.empty()) {
    return;
  }
  RECT rc;
  GetClientRect(hwnd, &rc);
  int client_width = rc.right - rc.left;
  int columns = ComputeColumnsForClientWidth(client_width);
  if (columns <= 0) {
    columns = 1;
  }
  int grid_left = ComputeGridLeft(client_width, columns);
  int grid_top = std::max(kGridTopOffset, 48 + 18 + 18 + 20 + 28);
  int rel_x = x - grid_left;
  int rel_y = y - grid_top;
  if (rel_x < 0 || rel_y < 0) {
    return;
  }
  int cell_width = kCoverWidth + kTileSpacing;
  int cell_height = kCoverHeight + kTileTextHeight + kTileSpacing;
  int column = rel_x / cell_width;
  int row = rel_y / cell_height;
  if (column < 0 || column >= columns || row < 0) {
    return;
  }
  int within_x = rel_x % cell_width;
  if (within_x > kCoverWidth) {
    return;
  }
  int within_y = rel_y % cell_height;
  if (within_y > kCoverHeight + kTileTextHeight) {
    return;
  }
  size_t index = static_cast<size_t>(row) * static_cast<size_t>(columns) + static_cast<size_t>(column);
  if (index < state->games.size()) {
    SetSelection(hwnd, state, index);
  }
}

std::wstring BuildSystemSummary(const SystemInfoData& info) {
  std::wstring summary = L"GPU: ";
  summary.append(info.gpuName.empty() ? L"Unknown" : info.gpuName);
  if (info.gpuMemoryMB != 0) {
    summary.append(L" (" + std::to_wstring(info.gpuMemoryMB) + L" MB VRAM)");
  }
  summary.append(L"\r\nCPU: ");
  summary.append(info.cpuName.empty() ? L"Unknown" : info.cpuName);
  uint32_t cores = info.cpuCores != 0 ? info.cpuCores : info.cpuThreads;
  uint32_t threads = info.cpuThreads != 0 ? info.cpuThreads : info.cpuCores;
  if (cores != 0 || threads != 0) {
    summary.append(L" (" + std::to_wstring(cores) + L"C/" + std::to_wstring(threads) + L"T)");
  }
  summary.append(L"\r\nRAM: ");
  summary.append(std::to_wstring(info.ramMB));
  summary.append(L" MB\r\nOS: ");
  summary.append(info.osVersion.empty() ? L"Windows" : info.osVersion);
  return summary;
}

std::wstring GetEditText(HWND hwnd, int control_id) {
  HWND control = GetDlgItem(hwnd, control_id);
  if (!control) {
    return {};
  }
  int length = GetWindowTextLengthW(control);
  if (length <= 0) {
    return {};
  }
  std::wstring buffer(static_cast<size_t>(length) + 1, L'\0');
  int copied = GetWindowTextW(control, buffer.data(), length + 1);
  if (copied < 0) {
    copied = 0;
  }
  buffer.resize(static_cast<size_t>(copied));
  return buffer;
}

INT_PTR CALLBACK SettingsDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_INITDIALOG: {
      auto* state = reinterpret_cast<AppState*>(lparam);
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
      if (!state) {
        EndDialog(hwnd, IDCANCEL);
        return FALSE;
      }
      CheckDlgButton(hwnd, IDC_SETTINGS_GLOBAL_CHECK, state->global_injection_enabled ? BST_CHECKED : BST_UNCHECKED);
      SetDlgItemTextW(hwnd, IDC_SETTINGS_SYSTEMINFO, BuildSystemSummary(state->system_info).c_str());
      SetDlgItemTextW(hwnd, IDC_SETTINGS_OPTISCALER_EDIT, state->optiscaler_override_dir.c_str());
      if (!state->games.empty() && state->selected_index < state->games.size()) {
        const GameEntry& game = state->games[state->selected_index];
        CheckDlgButton(hwnd, IDC_SETTINGS_GAME_CHECK, game.injectEnabled ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemTextW(hwnd, IDC_SETTINGS_PLANNED_EDIT, JoinLines(game.plannedFiles).c_str());
        std::wstring label = L"Selected game: ";
        label.append(game.name.empty() ? game.exe : game.name);
        SetDlgItemTextW(hwnd, IDC_SETTINGS_GAME_LABEL, label.c_str());
      } else {
        EnableWindow(GetDlgItem(hwnd, IDC_SETTINGS_GAME_CHECK), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_SETTINGS_PLANNED_EDIT), FALSE);
        SetDlgItemTextW(hwnd, IDC_SETTINGS_GAME_LABEL, L"No game selected.");
      }
      return TRUE;
    }
    case WM_COMMAND: {
      auto* state = reinterpret_cast<AppState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
      switch (LOWORD(wparam)) {
        case IDC_SETTINGS_BROWSE: {
          HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
          BROWSEINFOW bi = {};
          bi.hwndOwner = hwnd;
          bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
          PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
          if (pidl) {
            wchar_t path[MAX_PATH] = {};
            if (SHGetPathFromIDListW(pidl, path)) {
              SetDlgItemTextW(hwnd, IDC_SETTINGS_OPTISCALER_EDIT, path);
            }
            CoTaskMemFree(pidl);
          }
          if (SUCCEEDED(hr)) {
            CoUninitialize();
          }
          return TRUE;
        }
        case IDOK: {
          if (state) {
            state->global_injection_enabled =
                (IsDlgButtonChecked(hwnd, IDC_SETTINGS_GLOBAL_CHECK) == BST_CHECKED);
            std::wstring override_dir = Trim(GetEditText(hwnd, IDC_SETTINGS_OPTISCALER_EDIT));
            state->optiscaler_override_dir = override_dir;
            if (!state->games.empty() && state->selected_index < state->games.size()) {
              GameEntry& game = state->games[state->selected_index];
              game.injectEnabled = (IsDlgButtonChecked(hwnd, IDC_SETTINGS_GAME_CHECK) == BST_CHECKED);
              game.plannedFiles = SplitLines(GetEditText(hwnd, IDC_SETTINGS_PLANNED_EDIT));
            }
          }
          EndDialog(hwnd, IDOK);
          return TRUE;
        }
        case IDCANCEL:
          EndDialog(hwnd, IDCANCEL);
          return TRUE;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  return FALSE;
}

bool ShowSettingsDialog(HWND parent, AppState* state) {
  if (!state) {
    return false;
  }
  HINSTANCE instance = GetModuleHandleW(nullptr);
  INT_PTR result = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_SETTINGS), parent, SettingsDlgProc,
                                   reinterpret_cast<LPARAM>(state));
  return result == IDOK;
}

void OnCommand(HWND hwnd, AppState* state, WPARAM wparam) {
  const int command = LOWORD(wparam);
  switch (command) {
    case IDM_FILE_EXIT:
      PostMessageW(hwnd, WM_CLOSE, 0, 0);
      break;
    case IDM_FILE_RESCAN: {
      PerformScan(hwnd, state);
      break;
    }
    case IDM_TOOLS_SETTINGS:
      if (ShowSettingsDialog(hwnd, state)) {
        UpdateSelectionStatus(state);
        InvalidateRect(hwnd, nullptr, TRUE);
      }
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
      UpdateStatusBar(state, L"Ready.");
      PerformScan(hwnd, state);
      break;
    }
    case WM_SIZE:
      OnSize(hwnd, state, LOWORD(lparam), HIWORD(lparam));
      break;
    case WM_KEYDOWN:
      OnKeyDown(hwnd, state, wparam);
      break;
    case WM_LBUTTONDOWN:
      OnLButtonDown(hwnd, state, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
      break;
    case WM_COMMAND:
      OnCommand(hwnd, state, wparam);
      break;
    case WM_PAINT:
      OnPaint(hwnd, state);
      break;
    case WM_ERASEBKGND:
      return 1;
    case WM_DESTROY:
      if (state) {
        ReleaseGameResources(state->games);
        state->games.clear();
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
