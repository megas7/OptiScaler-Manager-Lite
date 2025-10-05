#include "cover_cache.h"

#include <cwctype>
#include <filesystem>
#include <vector>

#include <wincodec.h>
#include <wrl/client.h>

#include "cache.h"

namespace optiscaler {

namespace {

using Microsoft::WRL::ComPtr;

constexpr uint64_t kHashOffset = 1469598103934665603ull;
constexpr uint64_t kHashPrime = 1099511628211ull;

uint64_t HashPath(const std::wstring& path) {
  uint64_t hash = kHashOffset;
  for (wchar_t ch : path) {
    hash ^= static_cast<uint64_t>(towlower(ch));
    hash *= kHashPrime;
  }
  return hash;
}

struct ScopedCoInit {
  ScopedCoInit() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
      initialized_ = true;
      hr_ = hr;
    } else if (hr == RPC_E_CHANGED_MODE) {
      initialized_ = false;
      hr_ = S_OK;
    } else {
      initialized_ = false;
      hr_ = hr;
    }
  }

  ~ScopedCoInit() {
    if (initialized_) {
      CoUninitialize();
    }
  }

  bool Succeeded() const { return SUCCEEDED(hr_); }

 private:
  bool initialized_ = false;
  HRESULT hr_ = S_OK;
};

HBITMAP LoadBitmapInternal(const std::wstring& path, int width, int height) {
  if (path.empty()) {
    return nullptr;
  }
  ScopedCoInit coinit;
  if (!coinit.Succeeded()) {
    return nullptr;
  }
  ComPtr<IWICImagingFactory> factory;
  HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    return nullptr;
  }

  ComPtr<IWICBitmapDecoder> decoder;
  hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
  if (FAILED(hr)) {
    return nullptr;
  }

  ComPtr<IWICBitmapFrameDecode> frame;
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr)) {
    return nullptr;
  }

  UINT frame_width = 0;
  UINT frame_height = 0;
  frame->GetSize(&frame_width, &frame_height);
  int target_width = width > 0 ? width : static_cast<int>(frame_width);
  int target_height = height > 0 ? height : static_cast<int>(frame_height);
  if (target_width <= 0 || target_height <= 0) {
    return nullptr;
  }

  ComPtr<IWICBitmapSource> source = frame;
  if (static_cast<UINT>(target_width) != frame_width || static_cast<UINT>(target_height) != frame_height) {
    ComPtr<IWICBitmapScaler> scaler;
    hr = factory->CreateBitmapScaler(&scaler);
    if (SUCCEEDED(hr)) {
      hr = scaler->Initialize(frame.Get(), target_width, target_height, WICBitmapInterpolationModeFant);
      if (SUCCEEDED(hr)) {
        source = scaler;
      }
    }
  }

  ComPtr<IWICFormatConverter> converter;
  hr = factory->CreateFormatConverter(&converter);
  if (FAILED(hr)) {
    return nullptr;
  }

  hr = converter->Initialize(source.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f,
                             WICBitmapPaletteTypeCustom);
  if (FAILED(hr)) {
    return nullptr;
  }

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = target_width;
  bmi.bmiHeader.biHeight = -target_height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void* bits = nullptr;
  HBITMAP bitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (!bitmap || !bits) {
    if (bitmap) {
      DeleteObject(bitmap);
    }
    return nullptr;
  }

  const UINT stride = static_cast<UINT>(target_width) * 4;
  const UINT buffer_size = stride * static_cast<UINT>(target_height);
  hr = converter->CopyPixels(nullptr, stride, buffer_size, static_cast<BYTE*>(bits));
  if (FAILED(hr)) {
    DeleteObject(bitmap);
    return nullptr;
  }

  return bitmap;
}

bool SaveBitmapInternal(HBITMAP bitmap, const std::wstring& path) {
  if (!bitmap || path.empty()) {
    return false;
  }

  BITMAP bm = {};
  if (GetObjectW(bitmap, sizeof(bm), &bm) == 0) {
    return false;
  }
  const int width = bm.bmWidth;
  const int height = bm.bmHeight >= 0 ? bm.bmHeight : -bm.bmHeight;
  if (width <= 0 || height <= 0) {
    return false;
  }

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  std::vector<BYTE> pixels(static_cast<size_t>(width) * height * 4);
  HDC dc = CreateCompatibleDC(nullptr);
  if (!dc) {
    return false;
  }
  BOOL got_bits = GetDIBits(dc, bitmap, 0, static_cast<UINT>(height), pixels.data(), &bmi, DIB_RGB_COLORS);
  DeleteDC(dc);
  if (!got_bits) {
    return false;
  }

  ScopedCoInit coinit;
  if (!coinit.Succeeded()) {
    return false;
  }

  ComPtr<IWICImagingFactory> factory;
  HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    return false;
  }

  ComPtr<IWICStream> stream;
  hr = factory->CreateStream(&stream);
  if (FAILED(hr)) {
    return false;
  }
  hr = stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
  if (FAILED(hr)) {
    return false;
  }

  ComPtr<IWICBitmapEncoder> encoder;
  hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
  if (FAILED(hr)) {
    return false;
  }
  hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
  if (FAILED(hr)) {
    return false;
  }

  ComPtr<IWICBitmapFrameEncode> frame;
  ComPtr<IPropertyBag2> props;
  hr = encoder->CreateNewFrame(&frame, &props);
  if (FAILED(hr)) {
    return false;
  }
  hr = frame->Initialize(props.Get());
  if (FAILED(hr)) {
    return false;
  }
  hr = frame->SetSize(static_cast<UINT>(width), static_cast<UINT>(height));
  if (FAILED(hr)) {
    return false;
  }
  WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
  hr = frame->SetPixelFormat(&format);
  if (FAILED(hr)) {
    return false;
  }

  const UINT stride = static_cast<UINT>(width) * 4;
  hr = frame->WritePixels(static_cast<UINT>(height), stride, static_cast<UINT>(pixels.size()), pixels.data());
  if (FAILED(hr)) {
    return false;
  }
  hr = frame->Commit();
  if (FAILED(hr)) {
    return false;
  }
  hr = encoder->Commit();
  if (FAILED(hr)) {
    return false;
  }
  return true;
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

HBITMAP CoverCache::LoadImage(const std::wstring& path, int width, int height) {
  return LoadBitmapInternal(path, width, height);
}

HBITMAP CoverCache::LoadForExe(const std::wstring& exe_path, int width, int height) {
  return LoadBitmapInternal(PathForExe(exe_path), width, height);
}

bool CoverCache::SaveForExe(HBITMAP bitmap, const std::wstring& exe_path) {
  std::wstring path = PathForExe(exe_path);
  if (path.empty()) {
    return false;
  }
  return SaveBitmapInternal(bitmap, path);
}

}  // namespace optiscaler
