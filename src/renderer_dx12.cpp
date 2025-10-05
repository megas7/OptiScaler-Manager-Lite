#include "renderer_dx12.h"

namespace optiscaler {

bool RendererD3D12::Init(HWND /*hwnd*/) { return false; }
void RendererD3D12::Resize(UINT /*width*/, UINT /*height*/) {}
void RendererD3D12::Begin() {}
void RendererD3D12::DrawBitmap(HBITMAP /*bitmap*/, int /*x*/, int /*y*/, int /*width*/, int /*height*/) {}
void RendererD3D12::DrawText(const std::wstring& /*text*/, int /*x*/, int /*y*/, COLORREF /*color*/) {}
void RendererD3D12::DrawFrame(int /*x*/, int /*y*/, int /*width*/, int /*height*/, COLORREF /*color*/, int /*thickness*/) {}
void RendererD3D12::End() {}

}  // namespace optiscaler
