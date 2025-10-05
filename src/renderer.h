#pragma once

#include <string>

#include <windows.h>

namespace optiscaler {

class IRenderer {
 public:
  virtual ~IRenderer() = default;
  virtual bool Init(HWND hwnd) = 0;
  virtual void Resize(UINT width, UINT height) = 0;
  virtual void Begin() = 0;
  virtual void DrawBitmap(HBITMAP bitmap, int x, int y, int width, int height) = 0;
  virtual void DrawText(const std::wstring& text, int x, int y, COLORREF color) = 0;
  virtual void DrawFrame(int x, int y, int width, int height, COLORREF color, int thickness) = 0;
  virtual void End() = 0;
};

}  // namespace optiscaler
