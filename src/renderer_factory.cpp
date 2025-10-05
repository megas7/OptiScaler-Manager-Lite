#include "renderer_factory.h"

#include <memory>

#include "renderer_dx11.h"
#include "renderer_dx12.h"
#include "renderer_gdi.h"

namespace optiscaler {

namespace {

std::unique_ptr<IRenderer> TryCreate(std::unique_ptr<IRenderer> renderer, HWND hwnd) {
  if (renderer && renderer->Init(hwnd)) {
    return renderer;
  }
  return nullptr;
}

}  // namespace

std::unique_ptr<IRenderer> CreateRenderer(RendererPreference preference, HWND hwnd) {
  if (preference == RendererPreference::kAuto || preference == RendererPreference::kD3D12) {
    if (auto renderer = TryCreate(std::make_unique<RendererD3D12>(), hwnd)) {
      return renderer;
    }
    if (preference == RendererPreference::kD3D12) {
      return nullptr;
    }
  }
  if (preference == RendererPreference::kAuto || preference == RendererPreference::kD3D11) {
    if (auto renderer = TryCreate(std::make_unique<RendererD3D11>(), hwnd)) {
      return renderer;
    }
    if (preference == RendererPreference::kD3D11) {
      return nullptr;
    }
  }
  auto gdi = std::make_unique<RendererGDI>();
  if (gdi->Init(hwnd)) {
    return gdi;
  }
  return nullptr;
}

}  // namespace optiscaler
