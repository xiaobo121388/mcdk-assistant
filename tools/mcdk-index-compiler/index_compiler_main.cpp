#include "app/runtime_paths.hpp"
#include "app/server_runtime.hpp"
#include "common/path_utils.hpp"
#include "search/search_service.hpp"

#include <filesystem>
#include <iostream>

int main() {
    mcdk::app::init_console_encoding();

    namespace fs = std::filesystem;

    const auto runtime_paths  = mcdk::app::detect_runtime_paths();
    const auto& dicts_dir     = runtime_paths.dicts_dir;
    const auto& knowledge_dir = runtime_paths.knowledge_dir;
    const auto& cache_path    = runtime_paths.cache_path;

    const bool has_dicts     = fs::exists(dicts_dir);
    const bool has_knowledge = fs::exists(knowledge_dir);
    const bool has_cache     = fs::is_regular_file(cache_path);

    std::cerr << "[MCDK-IndexCompiler] dicts: "
              << mcdk::path::to_utf8(dicts_dir)
              << (has_dicts ? "" : " (NOT FOUND)") << std::endl;
    std::cerr << "[MCDK-IndexCompiler] knowledge: "
              << mcdk::path::to_utf8(knowledge_dir)
              << (has_knowledge ? "" : " (NOT FOUND)") << std::endl;
    std::cerr << "[MCDK-IndexCompiler] cache: "
              << mcdk::path::to_utf8(cache_path)
              << (has_cache ? "" : " (NOT FOUND)") << std::endl;

    if (!has_dicts) {
        std::cerr << "[MCDK-IndexCompiler] 错误：词库目录不存在: "
                  << mcdk::path::to_utf8(dicts_dir) << std::endl;
        return 1;
    }

    if (!has_knowledge) {
        std::cerr << "[MCDK-IndexCompiler] 错误：知识库目录不存在，无法编译索引: "
                  << mcdk::path::to_utf8(knowledge_dir) << std::endl;
        return 1;
    }

    try {
        std::cerr << "[MCDK-IndexCompiler] 正在编译知识库索引，请稍候..." << std::endl;
        mcdk::SearchService search_svc(dicts_dir, knowledge_dir, cache_path, true);
        std::cerr << "[MCDK-IndexCompiler] 编译完成，docs indexed: "
                  << search_svc.doc_count() << std::endl;
        std::cerr << "[MCDK-IndexCompiler] 编译完成，game assets indexed: "
                  << search_svc.game_assets_count() << std::endl;
        std::cerr << "[MCDK-IndexCompiler] 输出缓存: "
                  << mcdk::path::to_utf8(cache_path) << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "[MCDK-IndexCompiler] 编译失败: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[MCDK-IndexCompiler] 编译失败: 未知异常" << std::endl;
        return 1;
    }

    return 0;
}
