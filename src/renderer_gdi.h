#pragma once

#include "renderer.h"

namespace optiscaler {

class RendererGDI : public IRenderer {
 public:
  RendererGDI();
  ~RendererGDI() override;

  bool Init(HWND hwnd) override;
  void Resize(UINT width, UINT height) override;
  void Begin() override;
  void DrawBitmap(HBITMAP bitmap, int x, int y, int width, int height) override;
  void DrawText(const std::wstring& text, int x, int y, COLORREF color) override;
  void End() override;

 private:
  HWND hwnd_ = nullptr;
  HDC back_dc_ = nullptr;
  HBITMAP back_bitmap_ = nullptr;
  HBITMAP old_bitmap_ = nullptr;
  UINT width_ = 0;
  UINT height_ = 0;
};

}  // namespace optiscaler
