#pragma once

#include <filesystem>

namespace mcdk::app {

struct RuntimePaths {
    std::filesystem::path exe_dir;
    std::filesystem::path dicts_dir;
    std::filesystem::path knowledge_dir;
    std::filesystem::path cache_path;
};

RuntimePaths detect_runtime_paths();

} // namespace mcdk::app
