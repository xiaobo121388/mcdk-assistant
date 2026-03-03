#include "tools/ui_diagnoser.h"
#include <regex>
#include <sstream>
#include <set>

namespace mcdk {

std::vector<UiDiagnosis> diagnose_ui(const std::string& content) {
    std::vector<UiDiagnosis> results;
    std::istringstream iss(content);
    std::string line;
    int lineno = 0;
    bool has_namespace = false;
    std::set<std::string> control_keys;

    // 正则
    static const std::regex re_namespace(R"("namespace"\s*:\s*")");
    static const std::regex re_size_bad_space(R"("size"\s*:\s*\[.*\d+\s*%\s+[+\-]\s*\d+)");
    static const std::regex re_binding_name(R"re("binding_name"\s*:\s*"([^"]*)")re");
    static const std::regex re_control_key(R"re(^\s*"([^@"]+)(@[^"]*)?"\s*:\s*\{)re");
    static const std::regex re_size_unit(R"(\d+%[^+"'\]\s,}][a-zA-Z])");

    while (std::getline(iss, line)) {
        ++lineno;

        // 跳过注释行
        auto trimmed = line;
        auto pos = trimmed.find_first_not_of(" \t");
        if (pos != std::string::npos && trimmed.substr(pos, 2) == "//") continue;

        // 检查 namespace
        if (std::regex_search(line, re_namespace)) has_namespace = true;

        // 检查 size 格式错误："100% + 0px" 应为 "100%+0px"
        if (std::regex_search(line, re_size_bad_space)) {
            results.push_back({lineno, "warning", "size 值中 % 与 +/- 之间不应有空格，正确格式如 \"100%+0px\""});
        }

        // 检查 binding_name 是否以 # 开头
        std::smatch m;
        if (std::regex_search(line, m, re_binding_name)) {
            std::string bname = m[1].str();
            if (!bname.empty() && bname[0] != '#' && bname[0] != '$') {
                results.push_back({lineno, "error", "binding_name \"" + bname + "\" 应以 # 开头（数据绑定名）或 $ 开头（变量）"});
            }
        }

        // 检查 controls 数组中 key 唯一性（简化：检查同级控件定义 key）
        if (std::regex_search(line, m, re_control_key)) {
            std::string key = m[1].str();
            if (!control_keys.insert(key).second) {
                results.push_back({lineno, "warning", "控件 key \"" + key + "\" 重复，可能导致覆盖"});
            }
        }
    }

    // 全局检查
    if (!has_namespace) {
        results.push_back({0, "error", "缺少 \"namespace\" 定义，JSON UI 文件必须声明命名空间"});
    }

    return results;
}

} // namespace mcdk
