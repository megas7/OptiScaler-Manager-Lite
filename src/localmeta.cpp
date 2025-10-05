#include "localmeta.h"

#include <algorithm>
#include <shellapi.h>

namespace optiscaler {

namespace {

HICON LoadIconForExecutable(const std::wstring& exe_path) {
  if (exe_path.empty()) {
    return static_cast<HICON>(LoadIconW(nullptr, IDI_APPLICATION));
  }

  HICON large_icon = nullptr;
  UINT extracted = ExtractIconExW(exe_path.c_str(), 0, &large_icon, nullptr, 1);
  if (extracted == 0 || !large_icon) {
    SHFILEINFOW info = {};
    if (SHGetFileInfoW(exe_path.c_str(), FILE_ATTRIBUTE_NORMAL, &info, sizeof(info),
                       SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES) != 0) {
      large_icon = info.hIcon;
    }
  }

  if (!large_icon) {
    large_icon = static_cast<HICON>(LoadIconW(nullptr, IDI_APPLICATION));
  }
  return large_icon;
}

}  // namespace

HBITMAP LocalMeta::IconAsCover(const std::wstring& exe_path, int width, int height) {
  if (width <= 0 || height <= 0) {
    return nullptr;
  }

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;  // top-down DIB
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void* bits = nullptr;
  HBITMAP bitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (!bitmap) {
    return nullptr;
  }

  HDC dc = CreateCompatibleDC(nullptr);
  if (!dc) {
    DeleteObject(bitmap);
    return nullptr;
  }

  HGDIOBJ old = SelectObject(dc, bitmap);
  RECT rect = {0, 0, width, height};
  HBRUSH background = CreateSolidBrush(RGB(32, 32, 40));
  FillRect(dc, &rect, background);
  DeleteObject(background);

  // Accent band at the top for a subtle frame.
  RECT header = {0, 0, width, std::max(20, height / 12)};
  HBRUSH header_brush = CreateSolidBrush(RGB(64, 64, 88));
  FillRect(dc, &header, header_brush);
  DeleteObject(header_brush);

  HPEN border_pen = CreatePen(PS_SOLID, 1, RGB(80, 80, 96));
  HGDIOBJ old_pen = SelectObject(dc, border_pen);
  HGDIOBJ old_brush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
  Rectangle(dc, 0, 0, width, height);
  SelectObject(dc, old_pen);
  SelectObject(dc, old_brush);
  DeleteObject(border_pen);

  HICON icon = LoadIconForExecutable(exe_path);
  if (icon) {
    const int desired_icon_size = std::max(96, std::min(width - 32, height - 160));
    const int icon_x = (width - desired_icon_size) / 2;
    const int icon_y = header.bottom + (height - header.bottom - desired_icon_size) / 2 - 24;
    DrawIconEx(dc, icon_x, std::max(header.bottom + 8, icon_y), icon, desired_icon_size, desired_icon_size, 0, nullptr,
               DI_NORMAL);
    DestroyIcon(icon);
  }

  SelectObject(dc, old);
  DeleteDC(dc);
  return bitmap;
}

}  // namespace optiscaler
