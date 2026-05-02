#include "project_analysis/project_analyzer.hpp"
#include "common/path_utils.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

struct InteractiveOptions {
    fs::path mod_path;
    fs::path target_path;
    int depth = 4;
    bool include_symbols = true;
    bool include_unresolved = true;
    bool include_indirect = true;
    bool include_call_hints = true;
    bool ignore_third_party_analysis = true;
    int max_scope_upward_levels = 12;
};

std::string trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string unquote_path_text(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2) {
        const char first = value.front();
        const char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            value = value.substr(1, value.size() - 2);
        }
    }
    return trim(std::move(value));
}

fs::path from_user_path(const std::string& text) {
    return mcdk::path::from_utf8(unquote_path_text(text));
}

std::string to_utf8(const fs::path& path) {
    return mcdk::path::to_utf8(path);
}

std::string read_line(std::string_view prompt) {
    std::cout << prompt;
    std::cout.flush();

    std::string line;
    if (!std::getline(std::cin, line)) return {};
    return trim(std::move(line));
}

bool parse_bool_answer(const std::string& text, bool default_value) {
    auto value = trim(text);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (value.empty()) return default_value;
    if (value == "1" || value == "y" || value == "yes" || value == "true" || value == "on") return true;
    if (value == "0" || value == "n" || value == "no" || value == "false" || value == "off") return false;
    throw std::invalid_argument("无法解析布尔输入: " + text);
}

int parse_int_answer(const std::string& text, int default_value, int min_value, int max_value) {
    const auto value = trim(text);
    if (value.empty()) return default_value;

    try {
        size_t used = 0;
        int parsed = std::stoi(value, &used, 10);
        if (used != value.size()) throw std::invalid_argument("trailing characters");
        return std::max(min_value, std::min(parsed, max_value));
    } catch (const std::exception&) {
        throw std::invalid_argument("无法解析整数输入: " + text);
    }
}

void print_banner() {
    std::cout
        << "Python2 MOD 分析交互自测命令\n"
        << "- 输入用户实际 MOD 目录或行为包目录后，会返回完整 scan_py2_mod_architecture 诊断。\n"
        << "- 继续输入目标 py 文件或子目录后，会返回完整 scan_py2_import_chain 诊断。\n"
        << "- target_path 留空时默认分析 MOD 路径本身。\n"
        << "- MOD 路径处输入空行、q、quit 或 exit 可退出。\n\n";
}

void print_section_title(std::string_view title) {
    std::cout << "\n========== " << title << " ==========\n\n";
}

mcdk::project_analysis::AnalysisOptions to_analysis_options(const InteractiveOptions& input) {
    mcdk::project_analysis::AnalysisOptions options;
    options.depth = std::max(0, std::min(input.depth, 8));
    options.include_symbols = input.include_symbols;
    options.include_unresolved = input.include_unresolved;
    options.include_indirect = input.include_indirect;
    options.include_call_hints = input.include_call_hints;
    options.ignore_third_party_analysis = input.ignore_third_party_analysis;
    options.max_scope_upward_levels = std::max(1, std::min(input.max_scope_upward_levels, 32));
    return options;
}

bool path_exists(const fs::path& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

fs::path absolute_path(const fs::path& path) {
    std::error_code ec;
    auto abs = fs::absolute(path, ec);
    if (ec) return path.lexically_normal();
    return abs.lexically_normal();
}

InteractiveOptions prompt_options() {
    InteractiveOptions input;

    while (true) {
        const auto mod_text = read_line(">>> MOD/行为包路径: ");
        const auto lowered = [&]() {
            auto value = trim(mod_text);
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }();

        if (lowered.empty() || lowered == "q" || lowered == "quit" || lowered == "exit") {
            input.mod_path.clear();
            return input;
        }

        input.mod_path = from_user_path(mod_text);
        if (path_exists(input.mod_path)) break;
        std::cout << "[test] 路径不存在，请重新输入: " << to_utf8(input.mod_path) << "\n";
    }

    const auto target_text = read_line(">>> target_path，可填 py 文件/目录，留空则使用 MOD 路径: ");
    input.target_path = target_text.empty() ? input.mod_path : from_user_path(target_text);
    if (!path_exists(input.target_path)) {
        throw std::invalid_argument("target_path 不存在: " + to_utf8(input.target_path));
    }

    input.depth = parse_int_answer(read_line(">>> depth [4] (0-8): "), 4, 0, 8);

    const bool advanced = parse_bool_answer(read_line(">>> 是否配置高级选项? [N/y]: "), false);
    if (advanced) {
        input.ignore_third_party_analysis = parse_bool_answer(
            read_line(">>> ignore_third_party_analysis，跳过 QuModLibs 深度分析 [Y/n]: "), true);
        input.include_symbols = parse_bool_answer(read_line(">>> include_symbols [Y/n]: "), true);
        input.include_unresolved = parse_bool_answer(read_line(">>> include_unresolved [Y/n]: "), true);
        input.include_indirect = parse_bool_answer(read_line(">>> include_indirect [Y/n]: "), true);
        input.include_call_hints = parse_bool_answer(read_line(">>> include_call_hints [Y/n]: "), true);
        input.max_scope_upward_levels = parse_int_answer(
            read_line(">>> max_scope_upward_levels [12] (1-32): "), 12, 1, 32);
    }

    return input;
}

void print_selected_options(const InteractiveOptions& input,
                            const mcdk::project_analysis::AnalysisOptions& options) {
    std::cout << "\n[test] mod_path                    : " << to_utf8(absolute_path(input.mod_path)) << "\n";
    std::cout << "[test] target_path                 : " << to_utf8(absolute_path(input.target_path)) << "\n";
    std::cout << "[test] depth                       : " << options.depth << "\n";
    std::cout << "[test] include_symbols             : " << (options.include_symbols ? "true" : "false") << "\n";
    std::cout << "[test] include_unresolved          : " << (options.include_unresolved ? "true" : "false") << "\n";
    std::cout << "[test] include_indirect            : " << (options.include_indirect ? "true" : "false") << "\n";
    std::cout << "[test] include_call_hints          : " << (options.include_call_hints ? "true" : "false") << "\n";
    std::cout << "[test] ignore_third_party_analysis : " << (options.ignore_third_party_analysis ? "true" : "false") << "\n";
    std::cout << "[test] max_scope_upward_levels     : " << options.max_scope_upward_levels << "\n";
}

void run_analysis_once(const InteractiveOptions& input) {
    const auto options = to_analysis_options(input);
    mcdk::project_analysis::ProjectAnalyzer analyzer;

    print_selected_options(input, options);

    print_section_title("scan_py2_mod_architecture 完整诊断");
    std::cout << analyzer.analyze_behavior_pack(input.mod_path, options) << "\n";

    print_section_title("scan_py2_import_chain 完整诊断");
    std::cout << analyzer.analyze_target_references(input.target_path, options) << "\n";

    std::cout << "\n[test] 本轮分析完成。\n\n";
}

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    print_banner();

    while (true) {
        try {
            auto input = prompt_options();
            if (input.mod_path.empty()) break;
            run_analysis_once(input);
        } catch (const std::exception& ex) {
            std::cerr << "[test] ERROR: " << ex.what() << "\n\n";
        }
    }

    std::cout << "[test] exit\n";
    return 0;
}
