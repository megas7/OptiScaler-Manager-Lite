#pragma once

#include <memory>
#include <string>

#include "renderer.h"

namespace optiscaler {

enum class RendererPreference {
  kAuto,
  kD3D12,
  kD3D11,
  kGDI,
};

std::unique_ptr<IRenderer> CreateRenderer(RendererPreference preference, HWND hwnd);

}  // namespace optiscaler
