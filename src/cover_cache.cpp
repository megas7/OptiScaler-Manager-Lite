#include "cover_cache.h"

#include <filesystem>
#include <cwctype>

#include <cstdint>

#include <wincodec.h>

#include "cache.h"

namespace optiscaler {

namespace {
constexpr uint64_t kHashOffset = 1469598103934665603ull;
constexpr uint64_t kHashPrime = 1099511628211ull;
=======
constexpr int kHashOffset = 1469598103934665603ull;
constexpr int kHashPrime = 1099511628211ull;

uint64_t HashPath(const std::wstring& path) {
  uint64_t hash = kHashOffset;
  for (wchar_t ch : path) {
    hash ^= static_cast<uint64_t>(towlower(ch));
    hash *= kHashPrime;
  }
  return hash;
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

HBITMAP CoverCache::LoadForExe(const std::wstring& /*exe_path*/, int /*width*/, int /*height*/) {
  // TODO: Implement WIC decoding for cached bitmaps.
  return nullptr;
}

bool CoverCache::SaveForExe(HBITMAP /*bitmap*/, const std::wstring& /*exe_path*/) {
  // TODO: Implement WIC encoding for cached bitmaps.
  return false;
}

}  // namespace optiscaler
