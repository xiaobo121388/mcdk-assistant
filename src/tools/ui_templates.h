#pragma once
// ui_templates.h — generate_ui_fullstack 工具：全栈 JSON UI + Python 模板生成
#include <string>

namespace mcdk {

// 模板类型枚举（对应 generate_ui_fullstack 的 template_type 参数）
// screen_basic | screen_list | screen_grid | screen_form | screen_tabbed
// hud_overlay  | widget_button | widget_progress
struct UiTemplateResult {
    std::string json_ui;        // JSON UI 文件内容
    std::string python_class;   // Python UI 类文件内容（widget 类型为空）
    std::string ui_defs_entry;  // 需要添加到 _ui_defs.json 的条目
    std::string error;          // 非空时表示生成失败
};

// 生成全栈 UI 模板
// template_type: 见上方枚举
// ns:           JSON UI 命名空间，如 "MY_MOD_UI"
// mod_name:     Python 模块名，如 "mymod"
UiTemplateResult generate_ui_fullstack(
    const std::string& template_type,
    const std::string& ns,
    const std::string& mod_name
);

} // namespace mcdk
