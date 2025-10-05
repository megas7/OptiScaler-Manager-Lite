#include "cache.h"

#include <shlobj.h>
#include <windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace optiscaler {
namespace {

std::wstring WideFromUtf8(const std::string& input) {
  if (input.empty()) {
    return {};
  }
  int count = MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0);
  std::wstring wide(count, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), wide.data(), count);
  return wide;
}

std::string Utf8FromWide(const std::wstring& input) {
  if (input.empty()) {
    return {};
  }
  int count = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
  std::string utf8(count, '\0');
  WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), utf8.data(), count, nullptr, nullptr);
  return utf8;
}

}  // namespace

std::wstring Cache::AppDataRoot() {
  PWSTR path = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &path))) {
    std::wstring result(path);
    CoTaskMemFree(path);
    result += L"\\OptiScalerMgrLite";
    return result;
  }
  return L"";
}

bool Cache::EnsureDirectory(const std::wstring& path) {
  if (path.empty()) {
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return !ec;
}

bool Cache::WriteText(const std::wstring& path, const std::wstring& text) {
  if (path.empty()) {
    return false;
  }
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }
  const std::string utf8 = Utf8FromWide(text);
  DWORD written = 0;
  BOOL ok = TRUE;
  if (!utf8.empty()) {
    ok = WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
  }
  CloseHandle(file);
  return ok == TRUE;
}

bool Cache::ReadText(const std::wstring& path, std::wstring& text_out) {
  text_out.clear();
  if (path.empty()) {
    return false;
  }
  HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return false;
  }
  LARGE_INTEGER size = {};
  if (!GetFileSizeEx(file, &size)) {
    CloseHandle(file);
    return false;
  }
  std::vector<char> buffer(static_cast<size_t>(size.QuadPart));
  DWORD read = 0;
  BOOL ok = TRUE;
  if (!buffer.empty()) {
    ok = ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr);
  }
  CloseHandle(file);
  if (ok != TRUE) {
    return false;
  }
  text_out = WideFromUtf8(std::string(buffer.begin(), buffer.begin() + read));
  return true;
}

}  // namespace optiscaler
