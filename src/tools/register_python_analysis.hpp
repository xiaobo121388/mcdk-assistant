#pragma once

#include "common/path_utils.hpp"
#include "project_analysis/project_analyzer.hpp"

#include <mcp_message.h>
#include <mcp_server.h>
#include <mcp_tool.h>

#include <filesystem>
#include <string>

namespace mcdk {

inline project_analysis::AnalysisOptions read_py_analysis_options(const mcp::json& params) {
    project_analysis::AnalysisOptions options;
    options.depth = params.contains("depth") && params["depth"].is_number()
        ? params["depth"].get<int>() : 2;
    options.depth = std::max(0, std::min(options.depth, 8));
    options.include_symbols = params.value("include_symbols", true);
    options.include_unresolved = params.value("include_unresolved", true);
    options.include_indirect = params.value("include_indirect", true);
    options.include_call_hints = params.value("include_call_hints", true);
    options.max_scope_upward_levels = params.contains("max_scope_upward_levels") && params["max_scope_upward_levels"].is_number()
        ? params["max_scope_upward_levels"].get<int>() : 12;
    options.max_scope_upward_levels = std::max(1, std::min(options.max_scope_upward_levels, 32));
    if (params.contains("static_third_party_packages") && params["static_third_party_packages"].is_array()) {
        options.static_third_party_packages.clear();
        for (const auto& item : params["static_third_party_packages"]) {
            if (item.is_string()) options.static_third_party_packages.push_back(item.get<std::string>());
        }
    }
    return options;
}

inline void register_python_analysis_tools(mcp::server& srv) {
    auto analyzer = std::make_shared<project_analysis::ProjectAnalyzer>();

    auto full_tool = mcp::tool_builder("analyze_py2_behavior_pack_architecture")
        .with_description(
            "按行为包路径实时扫描其下所有 modMain.py，对每个 mod Python 包递归分析，并输出适合 AI 理解的 Python2 架构说明。"
            "标准库与 通用三方库 只输出调用信息，不参与深度依赖图。")
        .with_string_param("behavior_pack_path", "行为包根目录绝对路径", true)
        .with_number_param("depth", "分析深度（0-8，越高越展开间接依赖与模块摘要）", false)
        .with_boolean_param("include_symbols", "是否输出类/函数等符号摘要", false)
        .with_boolean_param("include_unresolved", "是否展示未解析外部引用", false)
        .with_array_param("static_third_party_packages", "静态三方库包名列表，例如 QuModLibs", "string", false)
        .with_read_only_hint(true)
        .with_idempotent_hint(true)
        .with_open_world_hint(true)
        .build();

    srv.register_tool(full_tool,
        [analyzer](const mcp::json& params, const std::string&) -> mcp::json {
            auto path_text = params.value("behavior_pack_path", "");
            if (path_text.empty()) {
                throw mcp::mcp_exception(mcp::error_code::invalid_params, "behavior_pack_path 不能为空");
            }
            auto report = analyzer->analyze_behavior_pack(mcdk::path::from_utf8(path_text), read_py_analysis_options(params));
            return {{"content", mcp::json::array({{{"type", "text"}, {"text", report}}})}};
        });

    auto ref_tool = mcp::tool_builder("analyze_py2_reference_architecture")
        .with_description(
            "按指定 py 文件或文件夹实时分析其在项目中的直接/间接引用关系。"
            "工具会自动向上寻找 modMain.py 与 manifest.json 来确定最优作用域，并避免无限上溯。")
        .with_string_param("target_path", "目标 Python 文件或文件夹绝对路径", true)
        .with_number_param("depth", "间接引用深度（0-8）", false)
        .with_boolean_param("include_indirect", "是否计算项目内间接引用", false)
        .with_boolean_param("include_unresolved", "是否展示未解析外部引用", false)
        .with_boolean_param("include_call_hints", "是否保留模糊调用说明字段", false)
        .with_number_param("max_scope_upward_levels", "向上解析最大层数，默认 12", false)
        .with_array_param("static_third_party_packages", "静态三方库包名列表，例如 QuModLibs", "string", false)
        .with_read_only_hint(true)
        .with_idempotent_hint(true)
        .with_open_world_hint(true)
        .build();

    srv.register_tool(ref_tool,
        [analyzer](const mcp::json& params, const std::string&) -> mcp::json {
            auto path_text = params.value("target_path", "");
            if (path_text.empty()) {
                throw mcp::mcp_exception(mcp::error_code::invalid_params, "target_path 不能为空");
            }
            auto report = analyzer->analyze_target_references(mcdk::path::from_utf8(path_text), read_py_analysis_options(params));
            return {{"content", mcp::json::array({{{"type", "text"}, {"text", report}}})}};
        });
}

} // namespace mcdk
