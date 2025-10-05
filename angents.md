# Build a Windows C++17 “OptiScaler Manager Lite” app (Win32 GUI, DX12/DX11 fallback, IGDB covers, per-game cache, launch games)

## Goal
Create a **standalone** Windows desktop app (no .NET/MFC) that scans installed PC games (Steam/Epic/custom folders; basic Xbox/MS Store support), shows a grid of covers with details, can **launch** games, manages **per-game injection settings** for “OptiScaler”, and fetches + caches IGDB metadata and images. It should **persist covers** across runs, and **auto-refresh** the IGDB OAuth token using Client ID + Client Secret. The UI must be non-flickery (double-buffered) and have a simple Settings dialog.

## Tech constraints
- Language: **C++17**
- UI: **pure Win32** (CreateWindowEx, menus, common controls). No MFC/.NET.
- Rendering: default **GDI double-buffer**; provide an optional renderer interface with implementations for **D3D12**, **D3D11** (selected at runtime; fallback to GDI if init fails).
- HTTP: **WinHTTP**
- Image load/encode: **WIC (WindowsCodecs)**. No external image libs.
- System info: **DXGI** (GPU); Win32 for CPU/RAM.
- File I/O & COM: standard Win32/COM. No heavy deps.
- JSON: use **single-header** `nlohmann/json.hpp` (bundle into `third_party/`) for IGDB/Twitch responses.

# VS 2022 .sln + .vcxproj prompt (no CMake)
Paste this into Codex to generate a solution + project wired for OptiScalerMgr Lite.

- One Win32 GUI app project
- C++17, Unicode, /permissive-, /utf-8, /Zc:__cplusplus
- Include dirs: $(ProjectDir)src; $(ProjectDir)third_party
- Link libs: comctl32; Windowscodecs; Shell32; Shlwapi; Winhttp; Version; Ole32; Uuid; d3d12; d3d11; dxgi; d2d1; Dwrite
- Recursive item includes and app.rc as resource

Templates are included in this zip as: OptiScalerMgrLite.vcxproj and OptiScalerMgrLite.vcxproj.filters.

## UI requirements
- Main window with:
  - **Menu**: File (Rescan, Exit), Tools (Settings…), Help (Open Logs Folder).
  - **Toolbar**: “Rescan”, “Fetch IGDB”, “Launch”.
  - **Grid** of up to N game tiles (200×300 covers). Smooth **double-buffer** (no flicker).
  - **Details panel**: name, exe path, folder, source (steam/epic/xbox/custom), basic system requirements (min/recommended if available), and **injection preview** (what files would be copied) with checkbox to enable/override per game.
  - **Status bar** line for async messages (e.g., “Prefetch complete”, “IGDB: unauthorized”).
- **Settings dialog**:
  - OptiScaler folder (browse), “Auto-update OptiScaler” toggle.
  - IGDB **Client ID** and **Client Secret** (securely stored in `settings.ini`).
  - Button: “Fetch Token” (calls Twitch/IGDB OAuth client-credentials, stores `igdbToken`).
  - Custom scan folders (list + add/remove).
  - Renderer preference (Auto/D3D12/D3D11/GDI).
- **Launch** button for the selected game:
  - Steam: `steam://run/<appId>` (if known) or Steam.exe + `-applaunch`.
  - Epic: use manifest’s `LaunchExecutable` or Epic URI if available.
  - Custom: `CreateProcess()` with working directory.

## Game discovery (modules & rules)
- **Steam**:
  - Read `%ProgramFiles(x86)%\Steam\steamapps\libraryfolders.vdf` to enumerate libraries.
  - Parse `appmanifest_*.acf` to get install dir & exe hints.
  - Optionally map exe→AppID to enable Steam run URI.
- **Epic**:
  - Parse `%ProgramData%\Epic\EpicGamesLauncher\Data\Manifests\*.item` JSON files for install directory and executable.
- **Xbox/MS Store** (best effort):
  - Prefer shortcuts/Start Menu links under `%ProgramData%\Microsoft\Windows\Start Menu\Programs`.
  - For UWP/MS Store, launching is out of scope; still list entries (source=`xbox`), with “Launch” disabled if not a classic exe.
- **Custom folders**:
  - User-added roots in Settings. Recursively scan for `*.exe` under common subfolders (`Binaries`, `Win64`, etc.).
  - Exclude obvious launchers (e.g., `unins*.exe`, `UnityCrashHandler64.exe`).
- **De-dupe** by normalized exe path. Build `GameEntry` for each found title.

## Data model
```cpp
struct GameEntry {
  std::wstring name;
  std::wstring exe;        // full path to executable
  std::wstring folder;     // game root dir
  std::wstring source;     // "steam" | "epic" | "xbox" | "custom"
  std::optional<uint32_t> steamAppId;
  HBITMAP coverBmp = nullptr;  // 200x300 cached bitmap
  // Injection config:
  bool injectEnabled = false;  // per-game override
  std::vector<std::wstring> plannedFiles; // preview: files that would be injected
};
```

## IGDB integration
- **Token flow (client-credentials)**:
  - POST `https://id.twitch.tv/oauth2/token?client_id=<ID>&client_secret=<SECRET>&grant_type=client_credentials`
  - Store `access_token` (string) and `expires_in` (seconds) in `settings.ini`, plus `token_acquired_utc`.
  - Before each IGDB call: if token missing/expired (or HTTP 401), refresh automatically.
- **Search & cover**:
  - Endpoint: `POST https://api.igdb.com/v4/games`
  - Headers: `Client-ID: <ID>`, `Authorization: Bearer <token>`
  - Body example: `fields name,cover.*,total_rating,platforms.name,genres.name; search "<GameName>"; limit 1;`
  - **Resolve image URL**: if `cover.image_id` exists, build `https://images.igdb.com/igdb/image/upload/t_cover_big/<image_id>.jpg`.
- **Prefetch thread**:
  - Background worker enumerates `g_Games` and for any **missing cover**:
    1) Check **disk cache** for that exe (see cache spec below).
    2) Try IGDB; if 401 → refresh token and retry once.
    3) If IGDB fails, try Steam library cover (grid icons) if AppID known.
    4) Fallback: extract EXE **icon** and render into a 200×300 placeholder.
  - For each success, **save** to cache and post `WM_APP_COVER_READY`.
  - On completion, post `WM_APP_PREFETCH_DONE`.

## Persistent cover & text cache
- Base: `%AppData%\OptiScalerMgrLite\cache`
- **Per-game cover**: `covers\by_game\<FNV1a64(hash of full exe path)>.png`
  - APIs:
    - `std::wstring CoverCache::PathForExe(const std::wstring& exePath);`
    - `HBITMAP CoverCache::LoadForExe(const std::wstring& exePath, int w, int h);`
    - `bool CoverCache::SaveForExe(HBITMAP, const std::wstring& exePath);`
  - Implement with **WIC** encoder/decoder. Always overwrite.
- **Text cache** helpers:
  - `bool Cache::WriteText(path, text)`; `bool Cache::ReadText(path, out)`.
  - Fix read by using `std::istreambuf_iterator<wchar_t>` for both iterators.

## OptiScaler management
- **Settings**: choose OptiScaler path (folder). If “Auto-update” enabled:
  - On startup or on “Check updates”, download the latest **from a configurable URL** or from GitHub Releases JSON (if you include it), then unpack into the chosen folder. If remote unavailable, allow the user to browse to a local zip.
- **Injection preview**:
  - For the selected game, compute which files (e.g., DLLs/configs) would be copied into the game folder. Show list + destination. Let user toggle “Enable for this game” and **edit mapping** (simple key→value pairs).
  - Do not actually inject until user clicks “Apply”.

## System info panel
- **GPU**: enumerate via DXGI (`IDXGIFactory1::EnumAdapters1`), pick default (LUID match with desktop), show name + VRAM MB and feature levels.
- **CPU**: `GetSystemInfo` for model string via registry or `__cpuid` fallback; show core/thread count.
- **RAM**: `GlobalMemoryStatusEx`.
- Display in Settings or sidebar.

## Renderer abstraction (optional but scaffold it)
- `IRenderer` interface: `bool Init(HWND)`, `void Resize(UINT,UINT)`, `void Begin()`, `void DrawBitmap(HBITMAP,int,int,int,int)`, `void DrawText(wstring,int,int,COLORREF)`, `void End()`.
- Implement:
  - `RendererD3D12` (swapchain, RTV heap).
  - `RendererD3D11` (swapchain, RTV).
  - `RendererGDI` (double-buffer DIBSection).  
- At runtime, try **D3D12 → D3D11 → GDI** in that order (or according to Settings).

## Logging
- File: `%AppData%\OptiScalerMgrLite\logs\app.log`
- Simple `Log(const wchar_t* fmt, ...)` with timestamps. Log key events: scan start/end, token fetch, HTTP status for IGDB, cache hits/misses, launch attempts.

## Settings persistence (`settings.ini`)
- Path: `%AppData%\OptiScalerMgrLite\settings.ini`
- Keys (one per line, UTF-16LE or UTF-8 BOM):
  ```
  optiDir=<full path>
  autoUpdate=0|1
  renderer=auto|d3d12|d3d11|gdi
  igdbClientId=<...>
  igdbSecret=<...>
  igdbToken=<...>
  igdbTokenExpiresUtc=<ISO8601>
  folder+<path>    // repeatable for custom roots
  ```
- On first run, create a minimal template (empty values).

## Modules & files to generate
```
src/
  main.cpp               // WinMain, WndProc, grid UI, double-buffer, commands
  app.rc                 // icons, version, menu
  scanner.h / scanner.cpp
  cover_cache.h / cover_cache.cpp
  cache.h / cache.cpp
  igdb.h / igdb.cpp      // OAuth + search; WinHTTP; JSON parsing
  steam_cover.h/.cpp     // optional: read Steam grid images, exe→AppID helper
  steam_store.h/.cpp     // optional: fetch basic store fields if desired
  localmeta.h/.cpp       // EXE icon → 200x300 HBITMAP via WIC encoder
  gameconfig.h/.cpp      // per-game injection model + serialize to disk
  optiscaler.h/.cpp      // auto-update & “Apply injection” implementation
  systeminfo.h/.cpp      // GPU/CPU/RAM detection
  renderer_factory.h/.cpp
  renderer_dx12.h/.cpp
  renderer_dx11.h/.cpp
  renderer_gdi.h/.cpp
third_party/
  json.hpp               // nlohmann/json single header
CMakeLists.txt
```

## Key function signatures (build these)
```cpp
// Scanner
std::vector<GameEntry> Scanner::ScanAll(const std::vector<std::wstring>& roots);
std::vector<std::wstring> Scanner::DefaultFolders(); // Steam/Epic roots + common locations

// IGDB
struct IgdbGame { std::wstring name; std::wstring imageId; /* … more fields */ };
bool IGDB::EnsureAccessToken(std::wstring clientId, std::wstring clientSecret,
                             std::wstring& token, std::wstring& expiresUtcIso, std::wstring& err);
std::optional<IgdbGame> IGDB::SearchOne(const std::wstring& clientId,
                                        const std::wstring& token,
                                        const std::wstring& name);
std::wstring IGDB::ImageUrlCoverBig(const std::wstring& imageId);
std::wstring IGDB::CacheImage(const std::wstring& url); // downloads to cache/images/<hash>.jpg

// Cover cache
std::wstring CoverCache::PathForExe(const std::wstring& exePath);
HBITMAP CoverCache::LoadForExe(const std::wstring& exePath, int w, int h);
bool CoverCache::SaveForExe(HBITMAP hbmp, const std::wstring& exePath);

// Local meta (EXE icon → cover)
HBITMAP LocalMeta::IconAsCover(const std::wstring& exePath, int w, int h);

// System info
SystemInfo GetSystemInfo(); // GPU name/VRAM, CPU name/cores/threads, RAM MB

// Launch
bool Launcher::Run(const GameEntry& g, std::wstring& err); // handles steam/epic/custom
```

## Threading & messages
- Background tasks (scan, IGDB prefetch) run on worker threads.
- Post results to UI via custom messages:
  - `WM_APP + 1`: cover ready (wParam = index)
  - `WM_APP + 2`: prefetch done
  - `WM_APP + 3`: scan done
- UI must invalidate the grid area only; redraw via backbuffer.

## CMake (generate this)
- Ready-to-open in VS 2022:
```cmake
cmake_minimum_required(VERSION 3.20)
project(OptiScalerMgrLite LANGUAGES CXX RC)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 -D_CRT_SECURE_NO_WARNINGS)
file(GLOB SRC CONFIGURE_DEPENDS "src/*.cpp" "src/*.rc" "src/*.h")
add_executable(${PROJECT_NAME} WIN32 ${SRC})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_link_libraries(${PROJECT_NAME} PRIVATE
  comctl32 Windowscodecs Shell32 Shlwapi Winhttp Version Ole32 uuid d3d12 d3d11 dxgi d2d1 dwrite)
if (MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W3 /permissive- /Zc:__cplusplus /utf-8)
endif()
```

## Acceptance criteria
- First run creates `settings.ini` under `%AppData%\OptiScalerMgrLite\`.
- Settings dialog can **obtain token** using `<IGDB_CLIENT_ID>` + `<IGDB_CLIENT_SECRET>`, saves token + expiry; reuses/refreshes automatically on 401/expired.
- Rescan finds Steam/Epic/custom games; shows at least 6 tiles if available.
- Clicking “Fetch IGDB” on a selected game fetches details + cover and **writes PNG** to `covers\by_game\<hash>.png`. Subsequent launches load from disk with **no network**.
- Status line shows “Prefetch complete.” after background worker finishes.
- “Launch” starts the selected game (Steam/Epic/custom exe).
- Renderer preference works: attempts DX12 → DX11 → GDI with clear fallback.
- No flicker on resize/paint (double-buffered GDI path).
- Logs written to `%AppData%\OptiScalerMgrLite\logs\app.log`.

## Notes / edge cases
- If IGDB or Twitch is offline, show a friendly message and fall back to **Steam cover** or **icon placeholder**; still **persist** placeholder to disk so the UI doesn’t rework every run.
- Handle **UTF-16** paths everywhere (use wide WinHTTP + WIC filename APIs).
- Clean up GDI objects (HBITMAP/HDC) to avoid leaks; RAII wrappers are welcome.
- Keep network & file I/O off the UI thread.
