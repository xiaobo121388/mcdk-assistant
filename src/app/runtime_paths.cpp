#include "app/runtime_paths.hpp"

#include "common/path_utils.hpp"

namespace mcdk::app {

RuntimePaths detect_runtime_paths() {
    RuntimePaths paths;
    // 运行期资源统一按 exe 相邻目录查找，避免依赖当前工作目录。
    paths.exe_dir = mcdk::path::executable_path().parent_path();
    paths.dicts_dir = paths.exe_dir / "dicts";
    paths.knowledge_dir = paths.exe_dir / "knowledge";
    paths.cache_path = paths.exe_dir / "mcdk_index_cache.bin";
    return paths;
}

} // namespace mcdk::app
