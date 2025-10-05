#pragma once

#include <string>
#include <vector>

#include "game_types.h"

namespace optiscaler {

class Scanner {
 public:
  static std::vector<GameEntry> ScanAll(const std::vector<std::wstring>& roots);
  static std::vector<std::wstring> DefaultFolders();
};

}  // namespace optiscaler
