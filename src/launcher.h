#pragma once

#include <string>

#include "game_types.h"

namespace optiscaler {

class Launcher {
 public:
  static bool Run(const GameEntry& game, std::wstring& error_out);
};

}  // namespace optiscaler
