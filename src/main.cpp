#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <cwchar>
#include <cwctype>
#include <cstdint>

#include <shlobj.h>
#include <shlwapi.h>

#include "cover_cache.h"
#include "game_types.h"
#include "igdb.h"
#include "launcher.h"
#include "renderer_factory.h"
#include "resource.h"
#include "scanner.h"
#include "systeminfo.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace optiscaler {

constexpr wchar_t kWindowClass[] = L"OptiScalerMgrLiteWindow";
constexpr int kCoverWidth = 200;
constexpr int kCoverHeight = 300;
constexpr int kTileSpacing = 24;
constexpr int kTileInfoHeight = 96;
constexpr int kDetailPanelDefaultWidth = 380;

struct AppState {
  std::vector<GameEntry> games;
  size_t selected_index = 0;
  std::unique_ptr<IRenderer> renderer;
  HWND status_bar = nullptr;
  SystemInfoData system_info;
  bool global_injection_enabled = true;
  std::wstring optiscaler_dir;
  std::vector<std::wstring> global_payload;
};

void UpdateStatusBar(AppState* state, const std::wstring& text);

struct LayoutInfo {
  int width = 0;
  int height = 0;
  int grid_left = 16;
  int grid_top = 64;
  int detail_width = kDetailPanelDefaultWidth;
  int detail_left = 0;
  bool show_detail = true;
  size_t columns = 1;
};

LayoutInfo ComputeLayout(const RECT& rc) {
  LayoutInfo layout;
  layout.width = rc.right - rc.left;
  layout.height = rc.bottom - rc.top;
  layout.detail_width = kDetailPanelDefaultWidth;
  if (layout.width < (kCoverWidth + kDetailPanelDefaultWidth + 96)) {
    if (layout.width > (kCoverWidth + 200)) {
      layout.detail_width = std::max(240, layout.width - (kCoverWidth + 80));
    } else {
      layout.detail_width = 0;
    }
  }
  layout.show_detail = layout.detail_width > 0;
  if (layout.show_detail) {
    layout.detail_left = layout.width - layout.detail_width - 16;
  } else {
    layout.detail_left = layout.width - 16;
  }
  if (layout.detail_left < layout.grid_left + kCoverWidth + 8) {
    layout.detail_left = layout.grid_left + kCoverWidth + 8;
  }
  int available_width = std::max(0, layout.detail_left - layout.grid_left);
  int pitch = kCoverWidth + kTileSpacing;
  if (available_width <= kCoverWidth + 8) {
    layout.columns = 1;
  } else {
    layout.columns = std::max<size_t>(1, static_cast<size_t>((available_width + kTileSpacing) / pitch));
  }
  return layout;
}

std::wstring CompactPath(const std::wstring& path, size_t max_chars) {
  if (path.size() <= max_chars) {
    return path;
  }
  std::wstring buffer(max_chars + 1, L'\0');
  if (PathCompactPathExW(buffer.data(), path.c_str(), static_cast<DWORD>(buffer.size()), 0)) {
    buffer.resize(wcslen(buffer.c_str()));
    return buffer;
  }
  return path.substr(path.size() - max_chars);
}

COLORREF ColorFromNameHash(const std::wstring& name) {
  uint32_t hash = 2166136261u;
  for (wchar_t ch : name) {
    hash ^= static_cast<uint32_t>(ch);
    hash *= 16777619u;
  }
  BYTE r = static_cast<BYTE>(128 + (hash & 0x7Fu));
  BYTE g = static_cast<BYTE>(128 + ((hash >> 8) & 0x7Fu));
  BYTE b = static_cast<BYTE>(128 + ((hash >> 16) & 0x7Fu));
  return RGB(r, g, b);
}

HBITMAP CreatePlaceholderCover(HWND hwnd, const std::wstring& title) {
  HDC window_dc = GetDC(hwnd);
  if (!window_dc) {
    return nullptr;
  }
  HDC memory_dc = CreateCompatibleDC(window_dc);
  if (!memory_dc) {
    ReleaseDC(hwnd, window_dc);
    return nullptr;
  }
  HBITMAP bitmap = CreateCompatibleBitmap(window_dc, kCoverWidth, kCoverHeight);
  if (!bitmap) {
    DeleteDC(memory_dc);
    ReleaseDC(hwnd, window_dc);
    return nullptr;
  }
  HGDIOBJ old_bitmap = SelectObject(memory_dc, bitmap);
  RECT rect = {0, 0, kCoverWidth, kCoverHeight};
  HBRUSH brush = CreateSolidBrush(ColorFromNameHash(title));
  FillRect(memory_dc, &rect, brush);
  DeleteObject(brush);
  SetBkMode(memory_dc, TRANSPARENT);
  SetTextColor(memory_dc, RGB(255, 255, 255));
  HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
  HGDIOBJ old_font = nullptr;
  if (font) {
    old_font = SelectObject(memory_dc, font);
  }
  RECT text_rect = {12, 12, kCoverWidth - 12, kCoverHeight - 12};
  DrawTextW(memory_dc, title.c_str(), -1, &text_rect,
            DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_VCENTER);
  if (old_font) {
    SelectObject(memory_dc, old_font);
  }
  SelectObject(memory_dc, old_bitmap);
  DeleteDC(memory_dc);
  ReleaseDC(hwnd, window_dc);
  return bitmap;
}

void ReleaseGameResources(std::vector<GameEntry>& games) {
  for (auto& game : games) {
    if (game.coverBmp) {
      DeleteObject(game.coverBmp);
      game.coverBmp = nullptr;
    }
  }
}

std::wstring BuildSupportString(const std::wstring& source) {
  std::wstring lower = source;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
  if (lower == L"steam") {
    return L"Steam integration: overlay, cloud saves, achievements";
  }
  if (lower == L"epic") {
    return L"Epic Games Launcher support";
  }
  if (lower == L"xbox") {
    return L"Microsoft Store / Xbox services";
  }
  return L"Manual install – configure OptiScaler features";
}

void InitializeGameEntry(HWND hwnd, AppState* state, GameEntry& game) {
  if (game.name.empty()) {
    game.name = L"Unnamed game";
  }
  if (game.officialSupport.empty()) {
    game.officialSupport = BuildSupportString(game.source);
  }
  game.injectEnabled = state ? state->global_injection_enabled : false;
  if (!game.coverBmp) {
    game.coverBmp = CreatePlaceholderCover(hwnd, game.name);
  }
}

std::vector<std::wstring> EnumeratePayload(const std::wstring& root) {
  std::vector<std::wstring> results;
  if (root.empty()) {
    return results;
  }
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
    if (ec) {
      break;
    }
    std::filesystem::path relative = entry.path().filename();
    if (relative.empty()) {
      continue;
    }
    std::wstring display = relative.wstring();
    std::error_code type_ec;
    if (entry.is_directory(type_ec)) {
      display.append(L"\\");
    }
    results.emplace_back(std::move(display));
  }
  std::sort(results.begin(), results.end());
  return results;
}

void UpdateGamePlanForEntry(const AppState* state, GameEntry& game) {
  game.plannedFiles.clear();
  if (!state || state->optiscaler_dir.empty() || state->global_payload.empty()) {
    return;
  }
  for (const auto& payload : state->global_payload) {
    std::wstring line = payload;
    line.append(L" → ");
    line.append(game.folder);
    game.plannedFiles.emplace_back(std::move(line));
  }
}

void UpdateGamePlans(AppState* state) {
  if (!state) {
    return;
  }
  for (auto& game : state->games) {
    UpdateGamePlanForEntry(state, game);
  }
}

void ApplyGlobalInjectionState(AppState* state) {
  if (!state) {
    return;
  }
  for (auto& game : state->games) {
    game.injectEnabled = state->global_injection_enabled;
  }
}

std::wstring BrowseForFolder(HWND owner, const wchar_t* title) {
  std::wstring selection;
  BROWSEINFOW bi = {};
  bi.hwndOwner = owner;
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
  bi.lpszTitle = title;
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
  if (pidl) {
    wchar_t path[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, path)) {
      selection = path;
    }
    CoTaskMemFree(pidl);
  }
  return selection;
}

void ChooseOptiScalerFolder(HWND hwnd, AppState* state) {
  if (!state) {
    return;
  }
  std::wstring folder = BrowseForFolder(hwnd, L"Select OptiScaler install directory");
  if (folder.empty()) {
    return;
  }
  state->optiscaler_dir = folder;
  state->global_payload = EnumeratePayload(folder);
  UpdateGamePlans(state);
  std::wstring status = L"OptiScaler folder set to ";
  status.append(folder);
  UpdateStatusBar(state, status);
  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }
}

void EnsureValidSelection(AppState* state) {
  if (!state) {
    return;
  }
  if (state->games.empty()) {
    state->selected_index = 0;
    return;
  }
  if (state->selected_index >= state->games.size()) {
    state->selected_index = state->games.size() - 1;
  }
}

void SelectGame(HWND hwnd, AppState* state, size_t index) {
  if (!state || index >= state->games.size()) {
    return;
  }
  if (state->selected_index != index) {
    state->selected_index = index;
    if (hwnd) {
      InvalidateRect(hwnd, nullptr, TRUE);
    }
  }
}

size_t HitTestGame(const RECT& rc, const AppState* state, int x, int y) {
  if (!state) {
    return static_cast<size_t>(-1);
  }
  LayoutInfo layout = ComputeLayout(rc);
  if (x < layout.grid_left || x > layout.detail_left) {
    return static_cast<size_t>(-1);
  }
  if (y < layout.grid_top) {
    return static_cast<size_t>(-1);
  }
  int relative_x = x - layout.grid_left;
  int relative_y = y - layout.grid_top;
  int pitch_x = kCoverWidth + kTileSpacing;
  int pitch_y = kCoverHeight + kTileInfoHeight;
  size_t column = static_cast<size_t>(relative_x / pitch_x);
  if (column >= layout.columns) {
    return static_cast<size_t>(-1);
  }
  size_t row = static_cast<size_t>(relative_y / pitch_y);
  size_t index = row * layout.columns + column;
  if (index >= state->games.size()) {
    return static_cast<size_t>(-1);
  }
  return index;
}

void ToggleSelectedGameInjection(HWND hwnd, AppState* state) {
  if (!state || state->selected_index >= state->games.size()) {
    return;
  }
  GameEntry& game = state->games[state->selected_index];
  game.injectEnabled = !game.injectEnabled;
  std::wstring status = game.injectEnabled ? L"Injection enabled for " : L"Injection disabled for ";
  status.append(game.name);
  UpdateStatusBar(state, status);
  if (hwnd) {
    InvalidateRect(hwnd, nullptr, TRUE);
  }
}

void HandleKeyDown(HWND hwnd, AppState* state, WPARAM key) {
  if (!state || state->games.empty()) {
    return;
  }
  RECT rc = {};
  GetClientRect(hwnd, &rc);
  LayoutInfo layout = ComputeLayout(rc);
  size_t columns = std::max<size_t>(1, layout.columns);
  size_t index = state->selected_index;
  switch (key) {
    case VK_LEFT:
      if (index > 0) {
        SelectGame(hwnd, state, index - 1);
      }
      break;
    case VK_RIGHT:
      if (index + 1 < state->games.size()) {
        SelectGame(hwnd, state, index + 1);
      }
      break;
    case VK_UP:
      if (index >= columns) {
        SelectGame(hwnd, state, index - columns);
      } else {
        SelectGame(hwnd, state, 0);
      }
      break;
    case VK_DOWN:
      if (index + columns < state->games.size()) {
        SelectGame(hwnd, state, index + columns);
      } else {
        SelectGame(hwnd, state, state->games.size() - 1);
      }
      break;
    case VK_HOME:
      SelectGame(hwnd, state, 0);
      break;
    case VK_END:
      SelectGame(hwnd, state, state->games.size() - 1);
      break;
    case VK_SPACE:
      ToggleSelectedGameInjection(hwnd, state);
      break;
    default:
      break;
  }
}

void HandleLButtonDown(HWND hwnd, AppState* state, LPARAM lparam) {
  if (!state) {
    return;
  }
  RECT rc = {};
  GetClientRect(hwnd, &rc);
  int x = GET_X_LPARAM(lparam);
  int y = GET_Y_LPARAM(lparam);
  size_t index = HitTestGame(rc, state, x, y);
  if (index != static_cast<size_t>(-1)) {
    SelectGame(hwnd, state, index);
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
  ReleaseGameResources(state->games);
  auto roots = Scanner::DefaultFolders();
  state->games = Scanner::ScanAll(roots);
  state->selected_index = 0;
  for (auto& game : state->games) {
    InitializeGameEntry(hwnd, state, game);
  }
  UpdateGamePlans(state);
  EnsureValidSelection(state);

  std::wstring status;
  if (state->games.empty()) {
    if (roots.empty()) {
      status = L"No default scan folders detected. Add custom folders from Settings when available.";
    } else {
      status = L"No games detected in default folders.";
    }
  } else {
    status = L"Found ";
    status.append(std::to_wstring(state->games.size()));
    status.append(state->games.size() == 1 ? L" game" : L" games");
    if (!roots.empty()) {
      status.append(L" across ");
      status.append(std::to_wstring(roots.size()));
      status.append(roots.size() == 1 ? L" folder" : L" folders");
    }
    if (!state->optiscaler_dir.empty()) {
      status.append(L" | OptiScaler payload ready");
    } else {
      status.append(L" | Select OptiScaler folder to preview injection files");
    }
    status.push_back(L'.');
  }
  UpdateStatusBar(state, status);

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
  const COLORREF text_color = RGB(255, 255, 255);
  const COLORREF highlight_color = RGB(255, 223, 0);
  const COLORREF path_color = RGB(200, 200, 200);
  const COLORREF meta_color = RGB(180, 220, 255);
  state->renderer->DrawText(L"OptiScaler Manager Lite", 16, 16, text_color);
  state->renderer->DrawText(L"Click a cover or use arrow keys to inspect games. Press Space to toggle injection.", 16, 36,
                            path_color);

  LayoutInfo layout = ComputeLayout(rc);
  int pitch_x = kCoverWidth + kTileSpacing;
  int pitch_y = kCoverHeight + kTileInfoHeight;

  if (!state->games.empty()) {
    for (size_t i = 0; i < state->games.size(); ++i) {
      GameEntry& game = state->games[i];
      if (!game.coverBmp) {
        game.coverBmp = CreatePlaceholderCover(hwnd, game.name);
      }
      size_t column = i % layout.columns;
      size_t row = i / layout.columns;
      int tile_x = layout.grid_left + static_cast<int>(column) * pitch_x;
      int tile_y = layout.grid_top + static_cast<int>(row) * pitch_y;
      state->renderer->DrawBitmap(game.coverBmp, tile_x, tile_y, kCoverWidth, kCoverHeight);
      if (i == state->selected_index) {
        state->renderer->DrawText(L"★", tile_x - 12, tile_y - 18, highlight_color);
      }
      int text_y = tile_y + kCoverHeight + 6;
      std::wstring title_line = std::to_wstring(i + 1);
      title_line.append(L". ");
      title_line.append(game.name);
      COLORREF line_color = (i == state->selected_index) ? highlight_color : text_color;
      state->renderer->DrawText(title_line, tile_x, text_y, line_color);
      text_y += 18;

      std::wstring meta_line;
      if (!game.source.empty()) {
        meta_line.append(game.source);
      }
      meta_line.append(game.injectEnabled ? L" • Injection on" : L" • Injection off");
      state->renderer->DrawText(meta_line, tile_x, text_y, meta_color);
      text_y += 18;

      std::wstring folder_line = L"Dir: ";
      folder_line.append(CompactPath(game.folder, 36));
      state->renderer->DrawText(folder_line, tile_x, text_y, path_color);
      text_y += 18;

      std::wstring support_line = L"Support: ";
      support_line.append(game.officialSupport);
      state->renderer->DrawText(support_line, tile_x, text_y, path_color);
    }
  } else {
    int empty_y = layout.grid_top;
    state->renderer->DrawText(L"No games found. Use File → Rescan after installing supported titles.", layout.grid_left,
                              empty_y, text_color);
    empty_y += 20;
    state->renderer->DrawText(L"Set custom folders from Tools → Select OptiScaler Folder to build your own library.",
                              layout.grid_left, empty_y, path_color);
  }

  if (layout.show_detail) {
    int panel_x = layout.detail_left + 16;
    int panel_y = layout.grid_top;
    int line_height = 18;
    state->renderer->DrawText(L"Game details", panel_x, panel_y, highlight_color);
    panel_y += line_height;
    if (!state->games.empty()) {
      const GameEntry& selected = state->games[state->selected_index];
      state->renderer->DrawText(selected.name, panel_x, panel_y, text_color);
      panel_y += line_height;

      std::wstring source_line = L"Source: ";
      source_line.append(selected.source.empty() ? L"custom" : selected.source);
      state->renderer->DrawText(source_line, panel_x, panel_y, path_color);
      panel_y += line_height;

      std::wstring exe_line = L"Executable: ";
      exe_line.append(CompactPath(selected.exe, 44));
      state->renderer->DrawText(exe_line, panel_x, panel_y, path_color);
      panel_y += line_height;

      std::wstring folder_line = L"Folder: ";
      folder_line.append(CompactPath(selected.folder, 44));
      state->renderer->DrawText(folder_line, panel_x, panel_y, path_color);
      panel_y += line_height;

      std::wstring support_line = L"Official support: ";
      support_line.append(selected.officialSupport);
      state->renderer->DrawText(support_line, panel_x, panel_y, path_color);
      panel_y += line_height;

      std::wstring inject_line = L"Injection: ";
      inject_line.append(selected.injectEnabled ? L"Enabled (Space to disable)" : L"Disabled (Space to enable)");
      state->renderer->DrawText(inject_line, panel_x, panel_y, meta_color);
      panel_y += line_height;

      if (!selected.plannedFiles.empty()) {
        state->renderer->DrawText(L"Planned files:", panel_x, panel_y, text_color);
        panel_y += line_height;
        size_t limit = 6;
        for (size_t i = 0; i < selected.plannedFiles.size() && i < limit; ++i) {
          state->renderer->DrawText(CompactPath(selected.plannedFiles[i], 48), panel_x + 12, panel_y, path_color);
          panel_y += line_height;
        }
        if (selected.plannedFiles.size() > limit) {
          state->renderer->DrawText(L"…", panel_x + 12, panel_y, path_color);
          panel_y += line_height;
        }
      } else {
        state->renderer->DrawText(L"Planned files: Set the OptiScaler folder to preview injections.", panel_x, panel_y,
                                  path_color);
        panel_y += line_height;
      }
    } else {
      state->renderer->DrawText(L"Run File → Rescan to populate your library.", panel_x, panel_y, path_color);
      panel_y += line_height;
    }

    panel_y += line_height;
    state->renderer->DrawText(L"Global settings", panel_x, panel_y, highlight_color);
    panel_y += line_height;
    std::wstring global_line = L"Global injection: ";
    global_line.append(state->global_injection_enabled ? L"On" : L"Off");
    state->renderer->DrawText(global_line, panel_x, panel_y, path_color);
    panel_y += line_height;
    std::wstring dir_line = L"OptiScaler dir: ";
    if (state->optiscaler_dir.empty()) {
      dir_line.append(L"<not set> (configure when GitHub is offline)");
    } else {
      dir_line.append(CompactPath(state->optiscaler_dir, 44));
    }
    state->renderer->DrawText(dir_line, panel_x, panel_y, path_color);
    panel_y += line_height;
    std::wstring payload_line = L"Payload items: ";
    payload_line.append(std::to_wstring(state->global_payload.size()));
    state->renderer->DrawText(payload_line, panel_x, panel_y, path_color);
    panel_y += line_height * 2;

    state->renderer->DrawText(L"System summary", panel_x, panel_y, highlight_color);
    panel_y += line_height;
    std::wstring os_line = L"OS: ";
    os_line.append(state->system_info.osVersion.empty() ? L"Unknown" : state->system_info.osVersion);
    state->renderer->DrawText(os_line, panel_x, panel_y, path_color);
    panel_y += line_height;
    std::wstring gpu_line = L"GPU: ";
    gpu_line.append(state->system_info.gpuName.empty() ? L"Unknown" : state->system_info.gpuName);
    if (state->system_info.gpuMemoryMB > 0) {
      gpu_line.append(L" (" + std::to_wstring(state->system_info.gpuMemoryMB) + L" MB VRAM)");
    }
    state->renderer->DrawText(gpu_line, panel_x, panel_y, path_color);
    panel_y += line_height;
    std::wstring cpu_line = L"CPU: ";
    cpu_line.append(state->system_info.cpuName.empty() ? L"Unknown" : state->system_info.cpuName);
    if (state->system_info.cpuCores > 0 && state->system_info.cpuThreads > 0) {
      cpu_line.append(L" (" + std::to_wstring(state->system_info.cpuCores) + L" cores / ");
      cpu_line.append(std::to_wstring(state->system_info.cpuThreads));
      cpu_line.append(L" threads)");
    }
    state->renderer->DrawText(cpu_line, panel_x, panel_y, path_color);
    panel_y += line_height;
    if (state->system_info.ramMB > 0) {
      std::wstring ram_line = L"RAM: ";
      ram_line.append(std::to_wstring(state->system_info.ramMB));
      ram_line.append(L" MB");
      state->renderer->DrawText(ram_line, panel_x, panel_y, path_color);
      panel_y += line_height;
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
    case IDM_TOOLS_SETTINGS: {
      if (!state) {
        break;
      }
      std::wstring message = L"Global injection: ";
      message.append(state->global_injection_enabled ? L"On" : L"Off");
      message.append(L"\nOptiScaler folder: ");
      message.append(state->optiscaler_dir.empty() ? L"<not set>" : state->optiscaler_dir);
      message.append(L"\nPayload files detected: ");
      message.append(std::to_wstring(state->global_payload.size()));
      if (!state->games.empty() && state->selected_index < state->games.size()) {
        const GameEntry& selected = state->games[state->selected_index];
        message.append(L"\n\nSelected game: ");
        message.append(selected.name);
        message.append(L"\nInjection status: ");
        message.append(selected.injectEnabled ? L"Enabled" : L"Disabled");
        if (!selected.plannedFiles.empty()) {
          message.append(L"\nPlanned files (preview):");
          size_t limit = 5;
          for (size_t i = 0; i < selected.plannedFiles.size() && i < limit; ++i) {
            message.append(L"\n  • ");
            message.append(selected.plannedFiles[i]);
          }
          if (selected.plannedFiles.size() > limit) {
            message.append(L"\n  …");
          }
        }
      }
      message.append(L"\n\nUse Tools → Select OptiScaler Folder to point at a local build when GitHub is offline.");
      MessageBoxW(hwnd, message.c_str(), L"OptiScaler Settings", MB_ICONINFORMATION);
      break;
    }
    case IDM_TOOLS_TOGGLE_GLOBAL: {
      if (!state) {
        break;
      }
      state->global_injection_enabled = !state->global_injection_enabled;
      ApplyGlobalInjectionState(state);
      std::wstring status = state->global_injection_enabled ? L"Global injection enabled" : L"Global injection disabled";
      UpdateStatusBar(state, status);
      InvalidateRect(hwnd, nullptr, TRUE);
      break;
    }
    case IDM_TOOLS_SELECT_OPTISCALER: {
      ChooseOptiScalerFolder(hwnd, state);
      break;
    }
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

      state->system_info = GetSystemInfo();
      state->global_payload = EnumeratePayload(state->optiscaler_dir);

      state->status_bar = CreateStatusWindowW(WS_CHILD | WS_VISIBLE, L"Ready", hwnd, IDC_STATUS_BAR);
      state->renderer = CreateRenderer(ParseRendererPreference(), hwnd);
      if (!state->renderer) {
        MessageBoxW(hwnd, L"Failed to initialize renderer.", L"OptiScaler Manager Lite", MB_ICONERROR);
        return -1;
      }
      UpdateStatusBar(state, L"Ready.");
      PerformScan(hwnd, state);
      break;
    }
    case WM_SIZE:
      OnSize(hwnd, state, LOWORD(lparam), HIWORD(lparam));
      break;
    case WM_COMMAND:
      OnCommand(hwnd, state, wparam);
      break;
    case WM_KEYDOWN:
      HandleKeyDown(hwnd, state, wparam);
      break;
    case WM_LBUTTONDOWN:
      HandleLButtonDown(hwnd, state, lparam);
      break;
    case WM_PAINT:
      OnPaint(hwnd, state);
      break;
    case WM_DESTROY:
      if (state) {
        ReleaseGameResources(state->games);
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
  HRESULT ole_init = OleInitialize(nullptr);
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
  if (SUCCEEDED(ole_init)) {
    OleUninitialize();
  }
  return static_cast<int>(msg.wParam);
}
