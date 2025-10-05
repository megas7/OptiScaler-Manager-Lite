#include "systeminfo.h"

#include <dxgi1_6.h>
#include <windows.h>

#include <vector>

#include <winreg.h>

namespace optiscaler {

namespace {

std::wstring ReadRegistryString(HKEY root, const wchar_t* path, const wchar_t* value_name) {
  HKEY key = nullptr;
  std::wstring result;
  if (RegOpenKeyExW(root, path, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) {
    return result;
  }
  wchar_t buffer[512] = {};
  DWORD size = sizeof(buffer);
  DWORD type = 0;
  if (RegGetValueW(key, nullptr, value_name, RRF_RT_REG_SZ, &type, buffer, &size) == ERROR_SUCCESS) {
    result.assign(buffer);
  }
  RegCloseKey(key);
  return result;
}

uint32_t CountPhysicalCores() {
  DWORD bytes = 0;
  GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
  if (bytes == 0) {
    return 0;
  }
  std::vector<uint8_t> buffer(bytes);
  if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
                                        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bytes)) {
    return 0;
  }
  uint32_t cores = 0;
  uint8_t* ptr = buffer.data();
  uint8_t* end = ptr + bytes;
  while (ptr < end) {
    auto* info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
    if (info->Relationship == RelationProcessorCore) {
      ++cores;
    }
    ptr += info->Size;
  }
  return cores;
}

void PopulateGpuInfo(SystemInfoData& info) {
  IDXGIFactory1* factory = nullptr;
  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
    return;
  }
  IDXGIAdapter1* adapter = nullptr;
  UINT index = 0;
  while (factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND) {
    DXGI_ADAPTER_DESC1 desc = {};
    if (SUCCEEDED(adapter->GetDesc1(&desc))) {
      if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 || !info.gpuName.empty()) {
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0 && !info.gpuName.empty()) {
          adapter->Release();
          ++index;
          continue;
        }
        info.gpuName.assign(desc.Description);
        info.gpuMemoryMB = static_cast<uint64_t>(desc.DedicatedVideoMemory / (1024ull * 1024ull));
        adapter->Release();
        break;
      }
      if (info.gpuName.empty()) {
        info.gpuName.assign(desc.Description);
        info.gpuMemoryMB = static_cast<uint64_t>(desc.DedicatedVideoMemory / (1024ull * 1024ull));
      }
    }
    adapter->Release();
    ++index;
  }
  factory->Release();
}

void PopulateCpuInfo(SystemInfoData& info) {
  info.cpuName = ReadRegistryString(HKEY_LOCAL_MACHINE,
                                    L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                                    L"ProcessorNameString");
  if (info.cpuName.empty()) {
    info.cpuName = L"Unknown CPU";
  }
  uint32_t cores = CountPhysicalCores();
  if (cores != 0) {
    info.cpuCores = cores;
  }
  DWORD logical = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if (logical != 0) {
    info.cpuThreads = logical;
  }
  if (info.cpuCores == 0) {
    info.cpuCores = info.cpuThreads;
  }
}

void PopulateOsInfo(SystemInfoData& info) {
  std::wstring product =
      ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ProductName");
  std::wstring display =
      ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
  if (display.empty()) {
    display = ReadRegistryString(HKEY_LOCAL_MACHINE,
                                 L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"ReleaseId");
  }
  std::wstring build =
      ReadRegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuild");
  if (!product.empty()) {
    info.osVersion = product;
    if (!display.empty()) {
      info.osVersion.append(L" ").append(display);
    }
    if (!build.empty()) {
      info.osVersion.append(L" (Build ").append(build).append(L")");
    }
  }
  if (info.osVersion.empty()) {
    info.osVersion = L"Windows";
  }
}

}  // namespace

SystemInfoData GetSystemInfo() {
  SystemInfoData info;
  MEMORYSTATUSEX mem = {};
  mem.dwLength = sizeof(mem);
  if (GlobalMemoryStatusEx(&mem)) {
    info.ramMB = static_cast<uint64_t>(mem.ullTotalPhys / (1024 * 1024));
  }
  PopulateGpuInfo(info);
  PopulateCpuInfo(info);
  PopulateOsInfo(info);
  if (info.gpuName.empty()) {
    info.gpuName = L"Unknown GPU";
  }
  return info;
}

}  // namespace optiscaler
