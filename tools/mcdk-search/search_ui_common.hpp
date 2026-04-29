#pragma once

#include "common/path_utils.hpp"
#include "search/search_service.hpp"

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/box.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <clocale>
#include <cctype>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace mcdk::search_ui {

namespace fs = std::filesystem;

using ftxui::bgcolor;
using ftxui::bold;
using ftxui::borderStyled;
using ftxui::clear_under;
using ftxui::color;
using ftxui::Element;
using ftxui::flex;
using ftxui::hbox;
using ftxui::paragraph;
using ftxui::reflect;
using ftxui::separator;
using ftxui::separatorEmpty;
using ftxui::size;
using ftxui::text;
using ftxui::vbox;
using ftxui::window;

constexpr int kResultTopK = 32;
constexpr int kResultVisible = 12;
constexpr int kContextVisible = 33;
constexpr int kMaxTitle = 58;
constexpr int kMaxPath = 76;
constexpr int kMaxSummary = 88;

inline ftxui::Color bg_col() { return ftxui::Color::RGB(7, 9, 14); }
inline ftxui::Color panel_col() { return ftxui::Color::RGB(13, 17, 28); }
inline ftxui::Color panel_hot_col() { return ftxui::Color::RGB(22, 28, 45); }
inline ftxui::Color line_col() { return ftxui::Color::RGB(47, 58, 86); }
inline ftxui::Color cyan_col() { return ftxui::Color::RGB(91, 221, 255); }
inline ftxui::Color blue_col() { return ftxui::Color::RGB(112, 154, 255); }
inline ftxui::Color purple_col() { return ftxui::Color::RGB(177, 135, 255); }
inline ftxui::Color muted_col() { return ftxui::Color::RGB(117, 125, 149); }

struct RuntimePaths {
    fs::path exe_dir;
    fs::path dicts_dir;
    fs::path knowledge_dir;
    fs::path cache_path;
};

struct SearchItem {
    std::string file;
    std::string content;
    std::string context_content;
    int line_start = 0;
    int line_end = 0;
    int context_line_start = 1;
    int context_total_lines = 0;
    double score = 0.0;
};

struct SearchPayload {
    int generation = 0;
    std::string mode;
    std::string keyword;
    std::vector<SearchItem> results;
    std::string error;
    long long elapsed_ms = 0;
};

struct AsyncState {
    std::mutex mutex;
    std::vector<SearchPayload> completed;
    std::atomic_bool alive{true};
};

struct CommandSpec {
    std::string name;
    std::string title;
    std::string hint;
};

struct ParsedQuery {
    std::string mode = "/all";
    std::string keyword;
};

inline const std::vector<CommandSpec>& commands() {
    static const std::vector<CommandSpec> specs = {
        {"/api", "ModAPI 接口", "搜索网易 ModAPI 接口文档"},
        {"/event", "ModAPI 事件", "搜索网易 ModAPI 事件文档"},
        {"/enum", "ModAPI 枚举", "搜索网易 ModAPI 枚举值文档"},
        {"/wiki", "Bedrock Wiki", "搜索 Bedrock Wiki 文档，英文关键词效果更好"},
        {"/dev", "BedrockDev", "搜索 bedrock.dev 官方格式文档"},
        {"/qumod", "QuMod", "搜索 QuModLibs 框架资料"},
        {"/netease", "网易教程", "搜索网易 MC 独占教程资料"},
        {"/all", "全库", "搜索全部文档；不写命令时默认使用此模式"},
    };
    return specs;
}

#ifdef _WIN32
inline BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
        return TRUE;
    }
    return FALSE;
}
#endif

inline bool init_console_encoding() {
    std::setlocale(LC_ALL, ".UTF-8");
#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    return SetConsoleOutputCP(CP_UTF8) != 0 && SetConsoleCP(CP_UTF8) != 0;
#else
    return true;
#endif
}

inline bool is_valid_utf8(std::string_view s) {
    size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        size_t extra = 0;
        if (c <= 0x7F) extra = 0;
        else if ((c & 0xE0) == 0xC0) { if (c < 0xC2) return false; extra = 1; }
        else if ((c & 0xF0) == 0xE0) extra = 2;
        else if ((c & 0xF8) == 0xF0) { if (c > 0xF4) return false; extra = 3; }
        else return false;
        if (i + extra >= s.size()) return false;
        for (size_t j = 1; j <= extra; ++j) {
            if ((static_cast<unsigned char>(s[i + j]) & 0xC0) != 0x80) return false;
        }
        i += extra + 1;
    }
    return true;
}

inline bool looks_like_incomplete_utf8(std::string_view s) {
    if (s.empty()) return false;
    size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        size_t extra = 0;
        if (c <= 0x7F) extra = 0;
        else if ((c & 0xE0) == 0xC0) { if (c < 0xC2) return false; extra = 1; }
        else if ((c & 0xF0) == 0xE0) extra = 2;
        else if ((c & 0xF8) == 0xF0) { if (c > 0xF4) return false; extra = 3; }
        else return false;
        if (i + extra >= s.size()) {
            for (size_t j = i + 1; j < s.size(); ++j) {
                if ((static_cast<unsigned char>(s[j]) & 0xC0) != 0x80) return false;
            }
            return true;
        }
        for (size_t j = 1; j <= extra; ++j) {
            if ((static_cast<unsigned char>(s[i + j]) & 0xC0) != 0x80) return false;
        }
        i += extra + 1;
    }
    return false;
}

#ifdef _WIN32
inline std::string codepage_to_utf8(std::string_view bytes, UINT codepage) {
    if (bytes.empty()) return {};
    int wide_len = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (wide_len <= 0) return {};
    std::wstring wide(static_cast<size_t>(wide_len), L'\0');
    wide_len = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), wide.data(), wide_len);
    if (wide_len <= 0) return {};
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, nullptr, 0, nullptr, nullptr);
    if (utf8_len <= 0) return {};
    std::string out(static_cast<size_t>(utf8_len), '\0');
    utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, out.data(), utf8_len, nullptr, nullptr);
    if (utf8_len <= 0) return {};
    return out;
}
#endif

inline bool maybe_ansi_multibyte(std::string_view s) {
    if (s.size() < 2) return false;
    for (unsigned char c : s) {
        if (c >= 0x80) return true;
    }
    return false;
}

inline void pop_utf8_codepoint(std::string& value) {
    if (value.empty()) return;
    size_t pos = value.size() - 1;
    while (pos > 0 && (static_cast<unsigned char>(value[pos]) & 0xC0) == 0x80) --pos;
    value.erase(pos);
}

inline std::string trim(std::string_view text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return std::string(text.substr(begin, end - begin));
}

inline std::string lower_ascii(std::string value) {
    for (auto& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

inline std::string compact_line(std::string value) {
    for (auto& ch : value) {
        if (ch == '\r' || ch == '\n' || ch == '\t') ch = ' ';
    }
    while (value.find("  ") != std::string::npos) {
        value.replace(value.find("  "), 2, " ");
    }
    return trim(value);
}

inline std::string shorten_ascii_safe(std::string value, size_t max_bytes) {
    value = compact_line(std::move(value));
    if (value.size() <= max_bytes) return value;
    if (max_bytes <= 3) return value.substr(0, max_bytes);
    size_t cut = max_bytes - 3;
    while (cut > 0 && (static_cast<unsigned char>(value[cut]) & 0xC0) == 0x80) --cut;
    return value.substr(0, cut) + "...";
}

inline std::vector<std::string> split_lines(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    if (lines.empty()) lines.push_back("");
    return lines;
}

inline fs::path executable_dir() {
    auto p = mcdk::path::executable_path();
    if (!p.empty()) return p.parent_path();
    return fs::current_path();
}

inline bool has_dicts(const fs::path& p) {
    return fs::is_regular_file(p / "jieba.dict.utf8")
        && fs::is_regular_file(p / "hmm_model.utf8")
        && fs::is_regular_file(p / "idf.utf8")
        && fs::is_regular_file(p / "stop_words.utf8");
}

inline RuntimePaths detect_paths() {
    RuntimePaths paths;
    paths.exe_dir = executable_dir();

    std::vector<fs::path> bases;
    bases.push_back(paths.exe_dir);
    bases.push_back(fs::current_path());
    if (paths.exe_dir.has_parent_path()) bases.push_back(paths.exe_dir.parent_path());
    if (paths.exe_dir.has_parent_path() && paths.exe_dir.parent_path().has_parent_path()) {
        bases.push_back(paths.exe_dir.parent_path().parent_path());
    }

    for (const auto& base : bases) {
        auto dicts = base / "dicts";
        auto knowledge = base / "knowledge";
        if (has_dicts(dicts) && fs::exists(knowledge)) {
            paths.dicts_dir = dicts;
            paths.knowledge_dir = knowledge;
            paths.cache_path = base / "mcdk_index_cache.bin";
            return paths;
        }
    }

    for (const auto& base : bases) {
        auto dicts = base / "dicts";
        auto cache = base / "mcdk_index_cache.bin";
        if (has_dicts(dicts) && fs::is_regular_file(cache)) {
            paths.dicts_dir = dicts;
            paths.cache_path = cache;
            return paths;
        }
    }

    paths.dicts_dir = paths.exe_dir / "dicts";
    paths.knowledge_dir = paths.exe_dir / "knowledge";
    paths.cache_path = paths.exe_dir / "mcdk_index_cache.bin";
    return paths;
}

inline std::string mode_title(const std::string& mode) {
    for (const auto& c : commands()) {
        if (c.name == mode) return c.title;
    }
    return "全库";
}

inline ParsedQuery parse_query(const std::string& input) {
    ParsedQuery parsed;
    auto value = trim(input);
    if (!value.empty() && value[0] == '/') {
        auto pos = value.find_first_of(" \t");
        auto cmd = lower_ascii(pos == std::string::npos ? value : value.substr(0, pos));
        bool known = false;
        for (const auto& spec : commands()) {
            if (spec.name == cmd) {
                parsed.mode = cmd;
                known = true;
                break;
            }
        }
        if (known) parsed.keyword = trim(pos == std::string::npos ? std::string_view{} : std::string_view(value).substr(pos + 1));
        else parsed.keyword = value;
    } else {
        parsed.keyword = value;
    }
    return parsed;
}

inline std::string active_slash_token(const std::string& input) {
    if (input.empty() || input[0] != '/') return {};
    auto pos = input.find_first_of(" \t");
    if (pos == std::string::npos) return lower_ascii(input);
    return lower_ascii(input.substr(0, pos));
}

inline std::vector<CommandSpec> completions_for(const std::string& input) {
    std::vector<CommandSpec> out;
    auto token = active_slash_token(input);
    if (token.empty()) return out;
    for (const auto& spec : commands()) {
        if (spec.name.find(token) == 0) out.push_back(spec);
    }
    return out;
}

inline std::string apply_completion(const std::string& input, const CommandSpec& spec) {
    auto pos = input.find_first_of(" \t");
    if (pos == std::string::npos) return spec.name + " ";
    return spec.name + input.substr(pos);
}

inline Element pill(const std::string& label, ftxui::Color fg, ftxui::Color bg) {
    return text(" " + label + " ") | color(fg) | bgcolor(bg) | bold;
}

inline Element muted(const std::string& value) {
    return text(value) | color(muted_col());
}

inline Element card(const std::string& title, Element body, ftxui::Color accent = line_col()) {
    return window(text(" " + title + " ") | color(accent) | bold, body, ftxui::ROUNDED)
        | borderStyled(ftxui::ROUNDED, accent)
        | bgcolor(panel_col());
}

inline Element gradient_title() {
    return hbox({
        text(" ✦ ") | color(purple_col()) | bold,
        text("M") | color(cyan_col()) | bold,
        text("C") | color(ftxui::Color::DeepSkyBlue1) | bold,
        text("D") | color(blue_col()) | bold,
        text("K") | color(purple_col()) | bold,
        text(" Search") | color(ftxui::Color::White) | bold,
        text("  terminal knowledge engine") | color(muted_col()),
    });
}

inline std::string spinner_frame(int tick) {
    static const char* frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    return frames[static_cast<size_t>(tick) % (sizeof(frames) / sizeof(frames[0]))];
}

inline bool copy_text_to_clipboard(const std::string& text) {
#ifdef _WIN32
    if (text.empty()) return false;

    int wide_len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (wide_len <= 0) return false;

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>((wide_len + 1) * sizeof(wchar_t)));
    if (!mem) return false;

    auto* wide = static_cast<wchar_t*>(GlobalLock(mem));
    if (!wide) {
        GlobalFree(mem);
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide, wide_len);
    wide[wide_len] = L'\0';
    GlobalUnlock(mem);

    if (!OpenClipboard(nullptr)) {
        GlobalFree(mem);
        return false;
    }

    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, mem)) {
        CloseClipboard();
        GlobalFree(mem);
        return false;
    }

    CloseClipboard();
    return true;
#else
    (void)text;
    return false;
#endif
}

inline std::string sanitize_selection_text(const std::string& text) {
    std::istringstream iss(text);
    std::string line;
    std::string out;
    bool first = true;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        auto bar = line.find("│");
        if (bar != std::string::npos) {
            bool numeric_prefix = true;
            for (size_t i = 0; i < bar; ++i) {
                const unsigned char c = static_cast<unsigned char>(line[i]);
                if (!std::isspace(c) && !std::isdigit(c)) {
                    numeric_prefix = false;
                    break;
                }
            }
            if (numeric_prefix) {
                line = line.substr(bar + std::string("│").size());
                while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) {
                    line.erase(line.begin());
                }
            }
        }

        if (!first) out += '\n';
        out += line;
        first = false;
    }
    return out;
}

inline std::string mode_from_file(const std::string& file) {
    if (file.find("ModAPI/接口") != std::string::npos) return "API";
    if (file.find("ModAPI/事件") != std::string::npos) return "EVENT";
    if (file.find("ModAPI/枚举") != std::string::npos) return "ENUM";
    if (file.find("BedrockWiki") != std::string::npos) return "WIKI";
    if (file.find("BedrockDev") != std::string::npos) return "DEV";
    if (file.find("QuMod") != std::string::npos) return "QUMOD";
    if (file.find("NeteaseGuide") != std::string::npos) return "NETEASE";
    return "DOC";
}

inline std::vector<SearchItem> search_with_service(const std::shared_ptr<mcdk::SearchService>& service,
                                                   const ParsedQuery& parsed) {
    std::vector<mcdk::SearchResult> raw;
    if (parsed.mode == "/api") raw = service->search_api(parsed.keyword, kResultTopK);
    else if (parsed.mode == "/event") raw = service->search_event(parsed.keyword, kResultTopK);
    else if (parsed.mode == "/enum") raw = service->search_enum(parsed.keyword, kResultTopK);
    else if (parsed.mode == "/wiki") raw = service->search_wiki(parsed.keyword, kResultTopK);
    else if (parsed.mode == "/dev") raw = service->search_bedrock_dev(parsed.keyword, kResultTopK);
    else if (parsed.mode == "/qumod") raw = service->search_qumod(parsed.keyword, kResultTopK);
    else if (parsed.mode == "/netease") raw = service->search_netease_guide(parsed.keyword, kResultTopK);
    else raw = service->search_all(parsed.keyword, kResultTopK);

    std::vector<SearchItem> items;
    items.reserve(raw.size());
    for (const auto& r : raw) {
        if (!r.fragment) continue;
        SearchItem item;
        item.file = r.fragment->file;
        item.content = r.fragment->content;
        item.line_start = r.fragment->line_start;
        item.line_end = r.fragment->line_end;
        item.score = r.score;
        items.push_back(std::move(item));
    }
    return items;
}

inline void load_full_context_for_item(const std::shared_ptr<mcdk::SearchService>& service, SearchItem& item) {
    if (!item.context_content.empty()) return;
    auto full = service->read_cached_file(item.file, 1, INT_MAX);
    if (full.found && !full.content.empty()) {
        item.context_content = std::move(full.content);
        item.context_line_start = 1;
        item.context_total_lines = full.total_lines;
    }
}

} // namespace mcdk::search_ui
