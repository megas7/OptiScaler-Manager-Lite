#pragma once

#include "renderer.h"

namespace optiscaler {

class RendererD3D11 : public IRenderer {
 public:
  bool Init(HWND hwnd) override;
  void Resize(UINT width, UINT height) override;
  void Begin() override;
  void DrawBitmap(HBITMAP bitmap, int x, int y, int width, int height) override;
  void DrawText(const std::wstring& text, int x, int y, COLORREF color) override;
  void End() override;
};

}  // namespace optiscaler
