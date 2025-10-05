#include "renderer_dx11.h"

namespace optiscaler {

bool RendererD3D11::Init(HWND /*hwnd*/) { return false; }
void RendererD3D11::Resize(UINT /*width*/, UINT /*height*/) {}
void RendererD3D11::Begin() {}
void RendererD3D11::DrawBitmap(HBITMAP /*bitmap*/, int /*x*/, int /*y*/, int /*width*/, int /*height*/) {}
void RendererD3D11::DrawText(const std::wstring& /*text*/, int /*x*/, int /*y*/, COLORREF /*color*/) {}
void RendererD3D11::DrawFrame(int /*x*/, int /*y*/, int /*width*/, int /*height*/, COLORREF /*color*/, int /*thickness*/) {}
void RendererD3D11::End() {}

}  // namespace optiscaler
