#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace mcdk::project_analysis {

struct AnalysisOptions {
    int depth = 2;
    bool include_symbols = true;
    bool include_unresolved = true;
    bool include_indirect = true;
    bool include_call_hints = true;
    bool ignore_third_party_analysis = true;
    int max_scope_upward_levels = 12;
    // QuModLibs 是唯一静态三方库，判定逻辑硬编码在 [is_static_third_party_module()](src/project_analysis/project_analyzer.cpp:169) 中
};

class ProjectAnalyzer {
public:
    std::string analyze_behavior_pack(const std::filesystem::path& behavior_pack_path,
                                      const AnalysisOptions& options) const;

    std::string analyze_target_references(const std::filesystem::path& target_path,
                                          const AnalysisOptions& options) const;
};

} // namespace mcdk::project_analysis
