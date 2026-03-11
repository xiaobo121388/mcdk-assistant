#include "search/search_service.hpp"
#include "search/index_cache.hpp"
#include "tools/register_search.hpp"
#include "tools/register_netease.hpp"
#ifndef MCDK_LITE
#include "tools/register_jsonui.hpp"
#include "tools/register_pixel_art.hpp"
#include "tools/register_model.hpp"
#include "tools/register_animation.hpp"
#endif
#include <mcp_server.h>
#include <iostream>
#include <string>
#include <memory>
#include <filesystem>
#include <thread>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

static std::filesystem::path get_exe_dir() {
    namespace fs = std::filesystem;
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path();
#else
    return fs::canonical("/proc/self/exe").parent_path();
#endif
}

static std::string fsToUtf8(const std::filesystem::path& p) {
    auto u8 = p.generic_u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    namespace fs = std::filesystem;

    auto exe_dir       = get_exe_dir();
    auto dicts_dir     = exe_dir / "dicts";
    auto knowledge_dir = exe_dir / "knowledge";
    auto cache_path    = exe_dir / "mcdk_index_cache.bin";
    // 保持ANSI编码确保兼容性
    std::string knowledge_dir_ansi = knowledge_dir.string();
    std::string cache_path_ansi    = cache_path.string();
    std::string dicts_dir_ansi = dicts_dir.string();

    bool has_dicts     = fs::exists(dicts_dir);
    bool has_knowledge = fs::exists(knowledge_dir);
    bool has_cache     = fs::is_regular_file(cache_path);

    std::cout << "[MCDK] dicts: "     << fsToUtf8(dicts_dir)     << (has_dicts     ? "" : " (NOT FOUND)") << std::endl;
    std::cout << "[MCDK] knowledge: " << fsToUtf8(knowledge_dir) << (has_knowledge ? "" : " (NOT FOUND)") << std::endl;
    std::cout << "[MCDK] cache: "     << fsToUtf8(cache_path)    << (has_cache     ? "" : " (NOT FOUND)") << std::endl;

    // 词库目录是必须的
    if (!has_dicts) {
        std::cerr << "[MCDK] 错误：词库目录不存在: " << fsToUtf8(dicts_dir) << std::endl;
        return 1;
    }

    // 仅缓存模式：knowledge 目录不存在但缓存文件存在
    bool cache_only_mode = !has_knowledge && has_cache;

    if (cache_only_mode) {
        std::cout << "[MCDK] 缓存解析：无外部资料，正在从索引文件解析..." << std::endl;
    } else if (!has_knowledge && !has_cache) {
        std::cerr << "[MCDK] 错误：缺少知识库目录和缓存文件，无法启动。" << std::endl;
        return 1;
    } else {
        std::cout << "[MCDK] 正在初始化知识库索引，请稍候..." << std::endl;
    }

    // 根据模式构造 SearchService
    std::unique_ptr<mcdk::SearchService> search_svc;
    if (cache_only_mode) {
        search_svc = std::make_unique<mcdk::SearchService>(dicts_dir_ansi, cache_path_ansi, /*cache_only=*/true);
    } else {
        search_svc = std::make_unique<mcdk::SearchService>(dicts_dir_ansi, knowledge_dir_ansi, cache_path_ansi);
    }

    mcp::server::configuration conf;
    conf.host    = "127.0.0.1";
    conf.port    = 18766;
#ifdef MCDK_LITE
    conf.name    = "mcdk-asst-lite";
    conf.version = "0.5.0-lite";
#else
    conf.name    = "mcdk-assistant";
    conf.version = "0.5.0";
#endif
    // 线程池最少 6 线程，避免 2 核 ECS 上只有 2 线程导致耗时工具阻塞搜索请求
    conf.threadpool_size = std::max(std::thread::hardware_concurrency(), 6u);

    mcp::server srv(conf);

    // 仅缓存模式下 knowledge_dir 传空字符串，工具内部会回退到缓存读取
    std::string effective_knowledge_dir = cache_only_mode ? "" : knowledge_dir.string();
    mcdk::register_search_tools(srv, *search_svc, effective_knowledge_dir);
    mcdk::register_netease_tools(srv);
#ifndef MCDK_LITE
    mcdk::register_jsonui_tools(srv);
    mcdk::register_pixel_art_tools(srv);
    mcdk::register_model_tools(srv);
    mcdk::register_animation_tools(srv);
#endif

#ifdef MCDK_LITE
    std::cout << "[MCDK-Lite] ";
#else
    std::cout << "[MCDK] ";
#endif
    std::cout << "MCP server starting on " << conf.host << ":" << conf.port << std::endl;
    std::cout << "[MCDK] docs indexed: "         << search_svc->doc_count()         << std::endl;
    std::cout << "[MCDK] game assets indexed: "  << search_svc->game_assets_count() << std::endl;
    if (cache_only_mode) {
        std::cout << "[MCDK] 已解析缓存索引库（缓存模式）" << std::endl;
    }

    srv.start(true);
    return 0;
}
