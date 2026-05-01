#pragma once

#include "common/path_utils.hpp"
#include "project_analysis/project_analyzer.hpp"

#include <mcp_message.h>
#include <mcp_server.h>
#include <mcp_tool.h>

#include <algorithm>
#include <memory>
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
    options.ignore_third_party_analysis = params.value("ignore_third_party_analysis", true);
    options.max_scope_upward_levels = params.contains("max_scope_upward_levels") && params["max_scope_upward_levels"].is_number()
        ? params["max_scope_upward_levels"].get<int>() : 12;
    options.max_scope_upward_levels = std::max(1, std::min(options.max_scope_upward_levels, 32));
    return options;
}

inline void register_python_analysis_tools(mcp::server& srv) {
    auto analyzer = std::make_shared<project_analysis::ProjectAnalyzer>();

    auto full_tool = mcp::tool_builder("scan_py2_mod_architecture")
        .with_description(
            "扫描指定行为包，解析其下所有 modMain.py 包并生成 Python 2 架构说明。"
            "输出适合 AI 阅读的模块分层、入口链与依赖图摘要。"
            "标准库与 QuModLibs 仅给出调用分布，不参与深度依赖计算。")
        .with_string_param("behavior_pack_path", "行为包根目录绝对路径", true)
        .with_number_param("depth", "分析深度（0-8，越高越展开间接依赖与模块摘要）", false)
        .with_boolean_param("include_symbols", "是否输出类/函数等符号摘要", false)
        .with_boolean_param("include_unresolved", "是否展示未解析外部引用", false)
        .with_boolean_param("ignore_third_party_analysis", "是否忽略 QuModLibs 深度分析，仅保留其调用摘要，默认 true", false)
        .with_read_only_hint(true)
        .with_idempotent_hint(true)
        .with_open_world_hint(true)
        .build();

    srv.register_tool(
        full_tool,
        [analyzer](const mcp::json& params, const std::string&) -> mcp::json {
            auto path_text = params.value("behavior_pack_path", "");
            if (path_text.empty()) {
                throw mcp::mcp_exception(mcp::error_code::invalid_params, "behavior_pack_path 不能为空");
            }
            auto report = analyzer->analyze_behavior_pack(
                mcdk::path::from_utf8(path_text),
                read_py_analysis_options(params));
            return {{"content", mcp::json::array({{{"type", "text"}, {"text", report}}})}};
        });

    auto ref_tool = mcp::tool_builder("scan_py2_import_chain")
        .with_description(
            "分析指定 Python 文件或目录在其所属项目中的引用关系。"
            "工具自动向上寻找 modMain.py 与 manifest.json 确定作用域，并避免无限上溯。"
            "输出直接引用者、间接引用链与目标模块的对外依赖摘要。")
        .with_string_param("target_path", "目标 Python 文件或文件夹绝对路径", true)
        .with_number_param("depth", "间接引用深度（0-8）", false)
        .with_boolean_param("include_indirect", "是否计算项目内间接引用", false)
        .with_boolean_param("include_unresolved", "是否展示未解析外部引用", false)
        .with_boolean_param("include_call_hints", "是否保留模糊调用说明字段", false)
        .with_boolean_param("ignore_third_party_analysis", "是否忽略 QuModLibs 深度分析，仅保留其调用摘要，默认 true", false)
        .with_number_param("max_scope_upward_levels", "向上解析最大层数，默认 12", false)
        .with_read_only_hint(true)
        .with_idempotent_hint(true)
        .with_open_world_hint(true)
        .build();

    srv.register_tool(
        ref_tool,
        [analyzer](const mcp::json& params, const std::string&) -> mcp::json {
            auto path_text = params.value("target_path", "");
            if (path_text.empty()) {
                throw mcp::mcp_exception(mcp::error_code::invalid_params, "target_path 不能为空");
            }
            auto report = analyzer->analyze_target_references(
                mcdk::path::from_utf8(path_text),
                read_py_analysis_options(params));
            return {{"content", mcp::json::array({{{"type", "text"}, {"text", report}}})}};
        });
}

} // namespace mcdk
