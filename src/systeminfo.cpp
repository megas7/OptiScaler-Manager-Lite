#include "systeminfo.h"

#include <dxgi1_6.h>
#include <windows.h>

#include <memory>
#include <string>
#include <vector>

namespace optiscaler {

namespace {

std::wstring Trimmed(const std::wstring& value) {
  const wchar_t* whitespace = L" \t\n\r";
  size_t start = value.find_first_not_of(whitespace);
  if (start == std::wstring::npos) {
    return L"";
  }
  size_t end = value.find_last_not_of(whitespace);
  return value.substr(start, end - start + 1);
}

uint32_t CountPhysicalCores() {
  DWORD length = 0;
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length) &&
      GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    SYSTEM_INFO sys = {};
    GetSystemInfo(&sys);
    return sys.dwNumberOfProcessors;
  }

  std::vector<uint8_t> buffer(length);
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
                                        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
                                        &length)) {
    SYSTEM_INFO sys = {};
    GetSystemInfo(&sys);
    return sys.dwNumberOfProcessors;
  }

  uint32_t cores = 0;
  size_t offset = 0;
  while (offset < buffer.size()) {
    auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset);
    if (info->Relationship == RelationProcessorCore) {
      ++cores;
    }
    offset += info->Size;
  }
  return cores;
}

void PopulateCpuInfo(SystemInfoData& info) {
  info.cpuThreads = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  info.cpuCores = CountPhysicalCores();

  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) ==
      ERROR_SUCCESS) {
    wchar_t buffer[256] = {};
    DWORD size = sizeof(buffer);
    if (RegQueryValueExW(key, L"ProcessorNameString", nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer), &size) ==
        ERROR_SUCCESS) {
      info.cpuName = Trimmed(buffer);
    }
    RegCloseKey(key);
  }

  if (info.cpuName.empty()) {
    SYSTEM_INFO sys_info = {};
    GetSystemInfo(&sys_info);
    info.cpuName = L"CPU";
    info.cpuCores = sys_info.dwNumberOfProcessors;
    info.cpuThreads = sys_info.dwNumberOfProcessors;
  }
}

void PopulateGpuInfo(SystemInfoData& info) {
  IDXGIFactory1* factory = nullptr;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    info.gpuName = L"Unknown GPU";
    return;
  }

  SIZE_T best_memory = 0;
  DXGI_ADAPTER_DESC1 best_desc = {};
  for (UINT index = 0;; ++index) {
    IDXGIAdapter1* adapter = nullptr;
    if (factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) {
      break;
    }
    DXGI_ADAPTER_DESC1 desc = {};
    if (SUCCEEDED(adapter->GetDesc1(&desc))) {
      if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
        if (desc.DedicatedVideoMemory > best_memory) {
          best_memory = desc.DedicatedVideoMemory;
          best_desc = desc;
        }
      }
    }
    adapter->Release();
  }
  factory->Release();

  if (best_memory > 0) {
    info.gpuName = Trimmed(best_desc.Description);
    info.gpuMemoryMB = static_cast<uint64_t>(best_desc.DedicatedVideoMemory / (1024ull * 1024ull));
  } else {
    info.gpuName = L"Unknown GPU";
  }
}

}  // namespace

SystemInfoData GetSystemInfo() {
  SystemInfoData info;
  MEMORYSTATUSEX mem = {};
  mem.dwLength = sizeof(mem);
  if (GlobalMemoryStatusEx(&mem)) {
    info.ramMB = static_cast<uint64_t>(mem.ullTotalPhys / (1024ull * 1024ull));
  }

  PopulateCpuInfo(info);
  PopulateGpuInfo(info);
  return info;
}

}  // namespace optiscaler
