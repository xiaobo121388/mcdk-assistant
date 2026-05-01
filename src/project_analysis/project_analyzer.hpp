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
    int max_scope_upward_levels = 12;
    std::vector<std::string> static_third_party_packages = {"QuModLibs"};
};

class ProjectAnalyzer {
public:
    std::string analyze_behavior_pack(const std::filesystem::path& behavior_pack_path,
                                      const AnalysisOptions& options) const;

    std::string analyze_target_references(const std::filesystem::path& target_path,
                                          const AnalysisOptions& options) const;
};

} // namespace mcdk::project_analysis
