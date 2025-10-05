#include "systeminfo.h"

#include <windows.h>

namespace optiscaler {

SystemInfoData GetSystemInfo() {
  SystemInfoData info;
  // TODO: Query GPU, CPU, and RAM information using DXGI and Win32 APIs.
  MEMORYSTATUSEX mem = {};
  mem.dwLength = sizeof(mem);
  if (GlobalMemoryStatusEx(&mem)) {
    info.ramMB = static_cast<uint64_t>(mem.ullTotalPhys / (1024 * 1024));
  }
  SYSTEM_INFO sys_info = {};
  GetSystemInfo(&sys_info);
  info.cpuCores = sys_info.dwNumberOfProcessors;
  info.cpuThreads = sys_info.dwNumberOfProcessors;
  info.cpuName = L"Unknown CPU";
  info.gpuName = L"Unknown GPU";
  return info;
}

}  // namespace optiscaler
