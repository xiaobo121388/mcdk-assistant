#pragma once
// ui_diagnoser.h — diagnose_ui 工具：JSON UI 错误诊断
#include <string>
#include <vector>

namespace mcdk {

struct UiDiagnosis {
    int line;              // 问题所在行号（0 表示全局问题）
    std::string severity;  // "error" | "warning"
    std::string message;
};

// 对 JSON UI 内容进行静态检查，返回诊断结果列表
std::vector<UiDiagnosis> diagnose_ui(const std::string& json_content);

} // namespace mcdk
