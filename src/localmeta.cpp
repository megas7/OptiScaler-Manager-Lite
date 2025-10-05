#include "localmeta.h"

#include <shellapi.h>

namespace optiscaler {

HBITMAP LocalMeta::IconAsCover(const std::wstring& /*exe_path*/, int /*width*/, int /*height*/) {
  // TODO: Extract icon resources and render into a 200x300 bitmap.
  return nullptr;
}

}  // namespace optiscaler
