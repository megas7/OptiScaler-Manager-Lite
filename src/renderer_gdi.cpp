#include "renderer_gdi.h"

#include <windowsx.h>

namespace optiscaler {

RendererGDI::RendererGDI() = default;

RendererGDI::~RendererGDI() {
  if (back_dc_) {
    if (old_bitmap_) {
      SelectObject(back_dc_, old_bitmap_);
    }
    if (back_bitmap_) {
      DeleteObject(back_bitmap_);
    }
    DeleteDC(back_dc_);
  }
}

bool RendererGDI::Init(HWND hwnd) {
  hwnd_ = hwnd;
  HDC window_dc = GetDC(hwnd_);
  back_dc_ = CreateCompatibleDC(window_dc);
  ReleaseDC(hwnd_, window_dc);
  return back_dc_ != nullptr;
}

void RendererGDI::Resize(UINT width, UINT height) {
  if (!back_dc_) {
    return;
  }
  if (back_bitmap_) {
    SelectObject(back_dc_, old_bitmap_);
    DeleteObject(back_bitmap_);
    back_bitmap_ = nullptr;
  }
  HDC window_dc = GetDC(hwnd_);
  back_bitmap_ = CreateCompatibleBitmap(window_dc, static_cast<int>(width), static_cast<int>(height));
  ReleaseDC(hwnd_, window_dc);
  old_bitmap_ = static_cast<HBITMAP>(SelectObject(back_dc_, back_bitmap_));
  width_ = width;
  height_ = height;
}

void RendererGDI::Begin() {
  if (!back_dc_) {
    return;
  }
  RECT rc = {0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
  HBRUSH brush = static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
  FillRect(back_dc_, &rc, brush);
}

void RendererGDI::DrawBitmap(HBITMAP bitmap, int x, int y, int width, int height) {
  if (!bitmap || !back_dc_) {
    return;
  }
  HDC temp_dc = CreateCompatibleDC(back_dc_);
  HGDIOBJ old = SelectObject(temp_dc, bitmap);
  BitBlt(back_dc_, x, y, width, height, temp_dc, 0, 0, SRCCOPY);
  SelectObject(temp_dc, old);
  DeleteDC(temp_dc);
}

void RendererGDI::DrawText(const std::wstring& text, int x, int y, COLORREF color) {
  if (!back_dc_) {
    return;
  }
  SetBkMode(back_dc_, TRANSPARENT);
  SetTextColor(back_dc_, color);
  TextOutW(back_dc_, x, y, text.c_str(), static_cast<int>(text.length()));
}

void RendererGDI::End() {
  if (!back_dc_) {
    return;
  }
  HDC window_dc = GetDC(hwnd_);
  BitBlt(window_dc, 0, 0, static_cast<int>(width_), static_cast<int>(height_), back_dc_, 0, 0, SRCCOPY);
  ReleaseDC(hwnd_, window_dc);
}

}  // namespace optiscaler
