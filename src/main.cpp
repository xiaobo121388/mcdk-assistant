#include "search/search_service.h"
#include <mcp_server.h>
#include <mcp_tool.h>
#include <mcp_message.h>
#include <iostream>
#include <string>
#include <filesystem>

// 获取可执行文件所在目录
static std::string get_exe_dir() {
    namespace fs = std::filesystem;
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path().string();
#else
    return fs::canonical("/proc/self/exe").parent_path().string();
#endif
}

// 解析路径：优先使用编译时宏，回退到 exe 同级目录
static std::string resolve_path(const char* compile_time_path, const std::string& fallback_name) {
    namespace fs = std::filesystem;
    if (fs::exists(compile_time_path)) return compile_time_path;
    std::string alt = get_exe_dir() + "/" + fallback_name;
    if (fs::exists(alt)) return alt;
    return compile_time_path; // 返回编译时路径，让后续报错
}

int main() {
#ifdef _WIN32
    // Windows 控制台 UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);  
#endif

    std::string dicts_dir     = resolve_path(MCDK_DICTS_DIR, "dicts");
    std::string knowledge_dir = resolve_path(MCDK_KNOWLEDGE_DIR, "knowledge");

    std::cout << "[mcdk] dicts: " << dicts_dir << std::endl;
    std::cout << "[mcdk] knowledge: " << knowledge_dir << std::endl;

    // 初始化搜索服务
    mcdk::SearchService search_svc(dicts_dir, knowledge_dir);

    // 配置 MCP 服务器
    mcp::server::configuration conf;
    conf.host    = "127.0.0.1";
    conf.port    = 8766;
    conf.name    = "mcdk-assistant";
    conf.version = "0.1.0";

    mcp::server srv(conf);

    // 注册 search_docs 工具
    auto search_tool = mcp::tool_builder("search_docs")
        .with_description("搜索 Minecraft ModAPI 文档知识库，返回相关文档片段（BM25 排序）")
        .with_string_param("keyword", "搜索关键词", true)
        .with_number_param("top_k", "返回结果数量上限，默认返回全部", false)
        .with_read_only_hint(true)
        .with_idempotent_hint(true)
        .build();

    srv.register_tool(search_tool,
        [&search_svc](const mcp::json& params, const std::string& /*session_id*/) -> mcp::json {
            std::string keyword = params.value("keyword", "");
            if (keyword.empty()) {
                throw mcp::mcp_exception(
                    mcp::error_code::invalid_params,
                    "keyword is required"
                );
            }

            int top_k = -1;
            if (params.contains("top_k") && !params["top_k"].is_null()) {
                top_k = params["top_k"].get<int>();
            }

            auto results = search_svc.search(keyword, top_k);

            mcp::json content_arr = mcp::json::array();
            for (const auto& r : results) {
                mcp::json item;
                item["type"]       = "text";
                item["text"]       = r.fragment->content;
                item["file"]       = r.fragment->file;
                item["line_start"] = r.fragment->line_start;
                item["line_end"]   = r.fragment->line_end;
                item["score"]      = r.score;
                content_arr.push_back(std::move(item));
            }

            return {{"content", content_arr}};
        }
    );

    std::cout << "[mcdk] MCP server starting on " << conf.host << ":" << conf.port << std::endl;
    std::cout << "[mcdk] docs indexed: " << search_svc.doc_count() << std::endl;

    srv.start(true); // 阻塞运行

    return 0;
}
