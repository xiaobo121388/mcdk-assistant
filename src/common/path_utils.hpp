#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace mcdk::path {

// 约定：模块内部尽量传 path，只有日志/协议边界再转 UTF-8 string。
inline std::filesystem::path from_utf8(std::string_view text) {
#if defined(__cpp_lib_char8_t)
    return std::filesystem::u8path(text);
#else
    return std::filesystem::u8path(text.begin(), text.end());
#endif
}

inline std::string to_utf8(const std::filesystem::path& value) {
    auto u8 = value.generic_u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

inline std::string filename_to_utf8(const std::filesystem::path& value) {
    return to_utf8(value.filename());
}

inline std::filesystem::path executable_path() {
#ifdef _WIN32
    // 用宽字符 API，避免可执行文件路径里有中文时被本地代码页截断。
    std::wstring buffer(MAX_PATH, L'\0');
    while (true) {
        DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) return {};
        if (size < buffer.size() - 1) {
            buffer.resize(size);
            return std::filesystem::path(buffer);
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    return std::filesystem::canonical("/proc/self/exe");
#endif
}

inline std::filesystem::path executable_dir() {
    return executable_path().parent_path();
}

} // namespace mcdk::path
