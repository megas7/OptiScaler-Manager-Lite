#include "scanner.h"

#include <algorithm>

#include "cache.h"

namespace optiscaler {

std::vector<GameEntry> Scanner::ScanAll(const std::vector<std::wstring>& /*roots*/) {
  // TODO: Implement platform-specific discovery for Steam, Epic, Xbox, and custom folders.
  return {};
}

std::vector<std::wstring> Scanner::DefaultFolders() {
  // TODO: Return sensible defaults for Steam, Epic Games Store, and Xbox installations.
  return {};
}

}  // namespace optiscaler
