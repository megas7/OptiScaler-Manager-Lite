#include "systeminfo.h"

#include <windows.h>
#include <dxgi1_4.h>
#include <winreg.h>

#include <vector>

namespace {

uint32_t CountBits(KAFFINITY mask) {
  uint32_t count = 0;
  while (mask) {
    count += static_cast<uint32_t>(mask & 1);
    mask >>= 1;
  }
  return count;
}

}  // namespace

namespace optiscaler {

SystemInfoData GetSystemInfo() {
  SystemInfoData info;
  MEMORYSTATUSEX mem = {};
  mem.dwLength = sizeof(mem);
  if (GlobalMemoryStatusEx(&mem)) {
    info.ramMB = static_cast<uint64_t>(mem.ullTotalPhys / (1024 * 1024));
  }

  // CPU name and topology.
  HKEY cpu_key = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                    0, KEY_READ, &cpu_key) == ERROR_SUCCESS) {
    wchar_t buffer[256];
    DWORD size = sizeof(buffer);
    if (RegGetValueW(cpu_key, nullptr, L"ProcessorNameString", RRF_RT_REG_SZ, nullptr,
                     buffer, &size) == ERROR_SUCCESS) {
      info.cpuName = buffer;
    }
    RegCloseKey(cpu_key);
  }

  SYSTEM_INFO sys_info = {};
  GetSystemInfo(&sys_info);
  info.cpuThreads = sys_info.dwNumberOfProcessors;

  DWORD length = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
  if (length > 0) {
    std::vector<uint8_t> buffer(length);
    if (GetLogicalProcessorInformationEx(RelationProcessorCore,
                                         reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()),
                                         &length)) {
      uint32_t cores = 0;
      uint32_t logical = 0;
      uint8_t* ptr = buffer.data();
      uint8_t* end = buffer.data() + length;
      while (ptr < end) {
        auto* info_ex = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
        if (info_ex->Relationship == RelationProcessorCore) {
          ++cores;
          for (WORD group_index = 0; group_index < info_ex->Processor.GroupCount; ++group_index) {
            logical += CountBits(info_ex->Processor.GroupMask[group_index].Mask);
          }
        }
        ptr += info_ex->Size;
      }
      if (cores > 0) {
        info.cpuCores = cores;
      }
      if (logical > 0) {
        info.cpuThreads = logical;
      }
    }
  }

  IDXGIFactory1* factory = nullptr;
  if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
    SIZE_T best_memory = 0;
    IDXGIAdapter1* adapter = nullptr;
    for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index) {
      DXGI_ADAPTER_DESC1 desc = {};
      if (SUCCEEDED(adapter->GetDesc1(&desc))) {
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && desc.DedicatedVideoMemory >= best_memory) {
          best_memory = desc.DedicatedVideoMemory;
          info.gpuName = desc.Description;
          info.gpuMemoryMB = static_cast<uint64_t>(desc.DedicatedVideoMemory / (1024 * 1024));
        }
      }
      adapter->Release();
    }
    factory->Release();
  }

  OSVERSIONINFOEXW os = {};
  os.dwOSVersionInfoSize = sizeof(os);
  if (GetVersionExW(reinterpret_cast<LPOSVERSIONINFOW>(&os))) {
    wchar_t version_buffer[128];
    swprintf(version_buffer, 128, L"Windows %u.%u (build %u)", os.dwMajorVersion, os.dwMinorVersion,
             os.dwBuildNumber);
    info.osVersion = version_buffer;
  } else {
    info.osVersion = L"Windows (version unavailable)";
  }

  if (info.cpuName.empty()) {
    info.cpuName = L"Unknown CPU";
  }
  if (info.gpuName.empty()) {
    info.gpuName = L"Unknown GPU";
  }
  return info;
}

}  // namespace optiscaler
