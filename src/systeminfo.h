#pragma once

#include <string>

namespace optiscaler {

struct SystemInfoData {
  std::wstring gpuName;
  uint64_t gpuMemoryMB = 0;
  std::wstring cpuName;
  uint32_t cpuCores = 0;
  uint32_t cpuThreads = 0;
  uint64_t ramMB = 0;
  std::wstring osVersion;
};

SystemInfoData GetSystemInfo();

}  // namespace optiscaler
