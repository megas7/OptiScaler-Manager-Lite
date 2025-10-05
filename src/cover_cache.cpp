#include "cover_cache.h"

#include <cwctype>
#include <cstdint>
#include <filesystem>

#include <wincodec.h>

#include "cache.h"

namespace optiscaler {
namespace {

constexpr uint64_t kHashOffset = 1469598103934665603ull;
constexpr uint64_t kHashPrime = 1099511628211ull;

class ScopedCoInit {
 public:
  ScopedCoInit() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    initialized_ = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    uninitialize_ = (hr == S_OK || hr == S_FALSE);
  }
  ~ScopedCoInit() {
    if (uninitialize_) {
      CoUninitialize();
    }
  }
  bool Succeeded() const { return initialized_; }

 private:
  bool initialized_ = false;
  bool uninitialize_ = false;
};

uint64_t HashPath(const std::wstring& path) {
  uint64_t hash = kHashOffset;
  for (wchar_t ch : path) {
    hash ^= static_cast<uint64_t>(towlower(ch));
    hash *= kHashPrime;
  }
  return hash;
}

HBITMAP CreateBitmapFromSource(IWICImagingFactory* factory,
                               IWICBitmapSource* source,
                               UINT target_width,
                               UINT target_height) {
  if (!factory || !source) {
    return nullptr;
  }

  IWICBitmapScaler* scaler = nullptr;
  IWICBitmapSource* used_source = source;
  UINT src_w = 0;
  UINT src_h = 0;
  source->GetSize(&src_w, &src_h);
  UINT width = target_width > 0 ? target_width : src_w;
  UINT height = target_height > 0 ? target_height : src_h;
  if ((src_w != width || src_h != height) && width > 0 && height > 0) {
    if (SUCCEEDED(factory->CreateBitmapScaler(&scaler))) {
      if (SUCCEEDED(scaler->Initialize(source, width, height, WICBitmapInterpolationModeFant))) {
        used_source = scaler;
      }
    }
  }

  IWICFormatConverter* converter = nullptr;
  HBITMAP bitmap = nullptr;
  if (SUCCEEDED(factory->CreateFormatConverter(&converter))) {
    if (SUCCEEDED(converter->Initialize(used_source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                        WICBitmapPaletteTypeCustom))) {
      UINT final_w = 0;
      UINT final_h = 0;
      converter->GetSize(&final_w, &final_h);
      BITMAPINFO bmi = {};
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = static_cast<LONG>(final_w);
      bmi.bmiHeader.biHeight = -static_cast<LONG>(final_h);
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;
      void* bits = nullptr;
      bitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
      if (bitmap && bits) {
        const UINT stride = final_w * 4;
        if (FAILED(converter->CopyPixels(nullptr, stride, stride * final_h, reinterpret_cast<BYTE*>(bits)))) {
          DeleteObject(bitmap);
          bitmap = nullptr;
        }
      } else if (bitmap) {
        DeleteObject(bitmap);
        bitmap = nullptr;
      }
    }
    converter->Release();
  }

  if (scaler) {
    scaler->Release();
  }
  return bitmap;
}

bool SaveSourceToPng(IWICImagingFactory* factory,
                     IWICBitmapSource* source,
                     const std::wstring& output_path,
                     UINT width,
                     UINT height) {
  if (!factory || !source) {
    return false;
  }

  IWICBitmapScaler* scaler = nullptr;
  IWICBitmapSource* used_source = source;
  UINT src_w = 0;
  UINT src_h = 0;
  source->GetSize(&src_w, &src_h);
  if ((src_w != width || src_h != height) && width > 0 && height > 0) {
    if (SUCCEEDED(factory->CreateBitmapScaler(&scaler))) {
      if (SUCCEEDED(scaler->Initialize(source, width, height, WICBitmapInterpolationModeFant))) {
        used_source = scaler;
      }
    }
  }

  IWICFormatConverter* converter = nullptr;
  IWICStream* stream = nullptr;
  IWICBitmapEncoder* encoder = nullptr;
  IWICBitmapFrameEncode* frame = nullptr;
  bool success = false;
  if (SUCCEEDED(factory->CreateFormatConverter(&converter)) &&
      SUCCEEDED(converter->Initialize(used_source, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                      WICBitmapPaletteTypeCustom)) &&
      SUCCEEDED(factory->CreateStream(&stream)) &&
      SUCCEEDED(stream->InitializeFromFilename(output_path.c_str(), GENERIC_WRITE)) &&
      SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) &&
      SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) &&
      SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)) &&
      SUCCEEDED(frame->Initialize(nullptr))) {
    UINT final_w = width;
    UINT final_h = height;
    converter->GetSize(&final_w, &final_h);
    frame->SetSize(final_w, final_h);
    GUID format = GUID_WICPixelFormat32bppPBGRA;
    frame->SetPixelFormat(&format);
    if (SUCCEEDED(frame->WriteSource(converter, nullptr)) && SUCCEEDED(frame->Commit()) &&
        SUCCEEDED(encoder->Commit())) {
      success = true;
    }
  }

  if (frame) {
    frame->Release();
  }
  if (encoder) {
    encoder->Release();
  }
  if (stream) {
    stream->Release();
  }
  if (converter) {
    converter->Release();
  }
  if (scaler) {
    scaler->Release();
  }
  return success;
}

}  // namespace

std::wstring CoverCache::PathForExe(const std::wstring& exe_path) {
  const std::wstring root = Cache::AppDataRoot();
  if (root.empty()) {
    return {};
  }
  std::filesystem::path path(root);
  path /= L"cache";
  path /= L"covers";
  path /= L"by_game";
  Cache::EnsureDirectory(path.wstring());
  wchar_t buffer[32];
  swprintf(buffer, 32, L"%016llx.png", static_cast<unsigned long long>(HashPath(exe_path)));
  path /= buffer;
  return path.wstring();
}

HBITMAP CoverCache::LoadForExe(const std::wstring& exe_path, int width, int height) {
  std::wstring path = PathForExe(exe_path);
  if (path.empty() || !std::filesystem::exists(path)) {
    return nullptr;
  }

  ScopedCoInit coinit;
  if (!coinit.Succeeded()) {
    return nullptr;
  }

  IWICImagingFactory* factory = nullptr;
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
    return nullptr;
  }

  IWICBitmapDecoder* decoder = nullptr;
  HBITMAP bitmap = nullptr;
  if (SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                                   &decoder))) {
    IWICBitmapFrameDecode* frame = nullptr;
    if (SUCCEEDED(decoder->GetFrame(0, &frame))) {
      bitmap = CreateBitmapFromSource(factory, frame, static_cast<UINT>(width), static_cast<UINT>(height));
      frame->Release();
    }
    decoder->Release();
  }
  factory->Release();
  return bitmap;
}

bool CoverCache::SaveImageFileForExe(const std::wstring& image_path,
                                     const std::wstring& exe_path,
                                     int width,
                                     int height) {
  if (image_path.empty() || exe_path.empty() || width <= 0 || height <= 0) {
    return false;
  }
  std::wstring dest = PathForExe(exe_path);
  if (dest.empty()) {
    return false;
  }

  ScopedCoInit coinit;
  if (!coinit.Succeeded()) {
    return false;
  }

  IWICImagingFactory* factory = nullptr;
  if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
    return false;
  }

  bool success = false;
  IWICBitmapDecoder* decoder = nullptr;
  if (SUCCEEDED(factory->CreateDecoderFromFilename(image_path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                                   &decoder))) {
    IWICBitmapFrameDecode* frame = nullptr;
    if (SUCCEEDED(decoder->GetFrame(0, &frame))) {
      success = SaveSourceToPng(factory, frame, dest, static_cast<UINT>(width), static_cast<UINT>(height));
      frame->Release();
    }
    decoder->Release();
  }
  factory->Release();
  return success;
}

}  // namespace optiscaler
