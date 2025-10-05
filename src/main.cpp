#include <windows.h>
#include <commctrl.h>

#include <memory>
#include <string>
#include <vector>

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

constexpr wchar_t kWindowClass[] = L"OptiScalerMgrLiteWindow";

struct AppState {
  std::vector<GameEntry> games;
  size_t selected_index = 0;
  std::unique_ptr<IRenderer> renderer;
  HWND status_bar = nullptr;
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

void PerformScan(HWND hwnd, AppState* state) {
  if (!state) {
    return;
  }

  UpdateStatusBar(state, L"Scanning for games...");
  auto roots = Scanner::DefaultFolders();
  state->games = Scanner::ScanAll(roots);
  state->selected_index = 0;
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
  const COLORREF text_color = RGB(255, 255, 255);
  const COLORREF highlight_color = RGB(255, 223, 0);
  const COLORREF path_color = RGB(210, 210, 210);
  state->renderer->DrawText(L"OptiScaler Manager Lite (prototype)", 16, 16, text_color);

  int y = 48;
  if (!state->games.empty()) {
    const size_t max_display = 20;
    for (size_t i = 0; i < state->games.size() && i < max_display; ++i) {
      const GameEntry& game = state->games[i];
      std::wstring title_line = std::to_wstring(i + 1);
      title_line.append(L". ");
      title_line.append(game.name.empty() ? L"<Unnamed>" : game.name);
      if (!game.source.empty()) {
        title_line.append(L" [");
        title_line.append(game.source);
        title_line.append(L"]");
      }
      COLORREF line_color = (i == state->selected_index) ? highlight_color : text_color;
      state->renderer->DrawText(title_line, 16, y, line_color);
      y += 18;
      std::wstring path_line = game.exe;
      state->renderer->DrawText(path_line, 32, y, path_color);
      y += 22;
    }
    if (state->games.size() > max_display) {
      std::wstring more = L"…and ";
      more.append(std::to_wstring(state->games.size() - max_display));
      more.append(L" more. Use File → Rescan after adding games.");
      state->renderer->DrawText(more, 16, y, text_color);
    }
  } else {
    state->renderer->DrawText(L"No games found. Use File → Rescan after installing supported titles.", 16, y,
                              text_color);
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
    case IDM_TOOLS_SETTINGS:
      MessageBoxW(hwnd, L"Settings dialog not yet implemented.", L"OptiScaler Manager Lite", MB_ICONINFORMATION);
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
    case WM_PAINT:
      OnPaint(hwnd, state);
      break;
    case WM_DESTROY:
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
