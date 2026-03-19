#include "app/runtime_paths.hpp"
#include "app/server_runtime.hpp"
#include "common/path_utils.hpp"
#include "search/search_service.hpp"
#include <mcp_server.h>
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>

int main() {
    mcdk::app::init_console_encoding();

    namespace fs = std::filesystem;

    const auto runtime_paths = mcdk::app::detect_runtime_paths();
    const auto& exe_dir = runtime_paths.exe_dir;
    const auto& dicts_dir = runtime_paths.dicts_dir;
    const auto& knowledge_dir = runtime_paths.knowledge_dir;
    const auto& cache_path = runtime_paths.cache_path;

    bool has_dicts     = fs::exists(dicts_dir);
    bool has_knowledge = fs::exists(knowledge_dir);
    bool has_cache     = fs::is_regular_file(cache_path);

    std::cout << "[MCDK] dicts: "     << mcdk::path::to_utf8(dicts_dir)     << (has_dicts     ? "" : " (NOT FOUND)") << std::endl;
    std::cout << "[MCDK] knowledge: " << mcdk::path::to_utf8(knowledge_dir) << (has_knowledge ? "" : " (NOT FOUND)") << std::endl;
    std::cout << "[MCDK] cache: "     << mcdk::path::to_utf8(cache_path)    << (has_cache     ? "" : " (NOT FOUND)") << std::endl;

    if (!has_dicts) {
        std::cerr << "[MCDK] 错误：词库目录不存在: " << mcdk::path::to_utf8(dicts_dir) << std::endl;
        return 1;
    }

    bool cache_only_mode = !has_knowledge && has_cache;

    if (cache_only_mode) {
        std::cout << "[MCDK] 缓存解析：无外部资料，正在从索引文件解析..." << std::endl;
    } else if (!has_knowledge && !has_cache) {
#ifndef MCDK_SERVER
        std::cerr << "[MCDK] 错误：缺少知识库目录和缓存文件，无法启动。" << std::endl;
        return 1;
#endif
    } else {
        std::cout << "[MCDK] 正在初始化知识库索引，请稍候..." << std::endl;
    }

    // 提供仅缓存模式和完整模式两套构造函数，以便工程使用/分发版优化启动速度。
    std::unique_ptr<mcdk::SearchService> search_svc;
    if (cache_only_mode) {
        search_svc = std::make_unique<mcdk::SearchService>(dicts_dir, cache_path, true);
    } else {
        search_svc = std::make_unique<mcdk::SearchService>(dicts_dir, knowledge_dir, cache_path);
    }

    mcp::server::configuration conf = mcdk::app::make_server_config();

    mcp::server srv(conf);

    mcdk::app::register_server_endpoints(srv, exe_dir, conf.port);
    mcdk::app::register_tools(srv, *search_svc, knowledge_dir, cache_only_mode);
    mcdk::app::log_startup_banner(conf, *search_svc, cache_only_mode);

    srv.start(true);

    return 0;
}
