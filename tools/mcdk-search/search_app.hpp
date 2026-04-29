#pragma once

#include "search_ui_common.hpp"

namespace mcdk::search_ui {

class SearchApp {
public:
    SearchApp(std::shared_ptr<mcdk::SearchService> service, RuntimePaths paths, bool cache_only)
        : service_(std::move(service)), paths_(std::move(paths)), cache_only_(cache_only), async_(std::make_shared<AsyncState>()) {
        last_tick_ = std::chrono::steady_clock::now();
    }

    ~SearchApp() {
        async_->alive = false;
        ++search_generation_;
    }

    ftxui::Component component() {
        return ftxui::CatchEvent(ftxui::Renderer([this] { return render(); }), [this](ftxui::Event event) {
            return on_event(event);
        });
    }

private:
    std::shared_ptr<mcdk::SearchService> service_;
    RuntimePaths paths_;
    bool cache_only_ = false;
    std::shared_ptr<AsyncState> async_;
    mutable ftxui::Box results_box_;
    mutable ftxui::Box context_box_;

    std::string input_;
    std::string last_query_ = "输入 /api AddItem 或直接输入关键词搜索全库";
    std::string status_ = "Ready";
    std::string error_;
    std::vector<SearchItem> results_;
    int selected_ = 0;
    int result_scroll_ = 0;
    int preview_scroll_ = 0;
    int completion_index_ = 0;
    bool show_completion_ = false;
    std::string pending_input_bytes_;
    std::atomic_bool searching_{false};
    std::atomic_int search_generation_{0};
    int spinner_tick_ = 0;
    std::chrono::steady_clock::time_point last_tick_{};

    bool on_event(ftxui::Event event) {
        pull_completed_searches();
        if (event.is_mouse()) {
            const auto& mouse = event.mouse();
            if (mouse.button == ftxui::Mouse::WheelUp || mouse.button == ftxui::Mouse::WheelDown) {
                const bool wheel_up = mouse.button == ftxui::Mouse::WheelUp;
                if (context_box_.Contain(mouse.x, mouse.y)) {
                    scroll_preview(wheel_up ? -3 : 3);
                    return true;
                }
                if (results_box_.Contain(mouse.x, mouse.y)) {
                    move_selection(wheel_up ? -1 : 1);
                    return true;
                }
                scroll_preview(wheel_up ? -3 : 3);
                return true;
            }
        }
        if (event == ftxui::Event::CtrlC) return true;
        if (event == ftxui::Event::Escape) {
            ftxui::ScreenInteractive::Active()->ExitLoopClosure()();
            return true;
        }
        if (event == ftxui::Event::Return) { run_search(); return true; }
        if (event == ftxui::Event::Tab) { apply_selected_completion(); return true; }
        if (event == ftxui::Event::ArrowLeft) { scroll_preview(-1); return true; }
        if (event == ftxui::Event::ArrowRight) { scroll_preview(1); return true; }
        if (event == ftxui::Event::ArrowUp) { move_selection(-1); return true; }
        if (event == ftxui::Event::ArrowDown) { move_selection(1); return true; }
        if (event == ftxui::Event::PageUp) { scroll_preview(-8); return true; }
        if (event == ftxui::Event::PageDown) { scroll_preview(8); return true; }
        if (event == ftxui::Event::Character('u')) { scroll_preview(-16); return true; }
        if (event == ftxui::Event::Character('d')) { scroll_preview(16); return true; }
        if (event == ftxui::Event::Character('g')) { preview_scroll_ = 0; return true; }
        if (event == ftxui::Event::Character('G')) { preview_scroll_ = preview_max_scroll(); return true; }
        if (event == ftxui::Event::Home) { selected_ = 0; focus_preview_on_selected(); adjust_result_scroll(); return true; }
        if (event == ftxui::Event::End) {
            if (!results_.empty()) selected_ = static_cast<int>(results_.size()) - 1;
            focus_preview_on_selected();
            adjust_result_scroll();
            return true;
        }
        if (event == ftxui::Event::Backspace) {
            pending_input_bytes_.clear();
            pop_utf8_codepoint(input_);
            update_completion();
            return true;
        }
        if (event.is_character()) {
            append_typed_text(event.character());
            update_completion();
            return true;
        }
        return false;
    }

    void append_typed_text(const std::string& raw) {
        if (raw.empty()) return;
        if (pending_input_bytes_.empty() && is_valid_utf8(raw)) {
            input_ += raw;
            return;
        }
        pending_input_bytes_ += raw;
        if (is_valid_utf8(pending_input_bytes_)) {
            input_ += pending_input_bytes_;
            pending_input_bytes_.clear();
            return;
        }
        if (looks_like_incomplete_utf8(pending_input_bytes_)) return;
#ifdef _WIN32
        if (maybe_ansi_multibyte(pending_input_bytes_)) {
            if (auto converted = codepage_to_utf8(pending_input_bytes_, CP_ACP); !converted.empty() && is_valid_utf8(converted)) {
                input_ += converted;
                pending_input_bytes_.clear();
                return;
            }
            if (auto converted = codepage_to_utf8(pending_input_bytes_, 936); !converted.empty() && is_valid_utf8(converted)) {
                input_ += converted;
                pending_input_bytes_.clear();
                return;
            }
        }
#endif
        input_ += pending_input_bytes_;
        pending_input_bytes_.clear();
    }

    void update_completion() {
        auto matches = completions_for(input_);
        show_completion_ = !matches.empty() && input_.find_first_of(" \t") == std::string::npos;
        if (completion_index_ >= static_cast<int>(matches.size())) completion_index_ = 0;
    }

    void apply_selected_completion() {
        auto matches = completions_for(input_);
        if (matches.empty()) return;
        completion_index_ = std::clamp(completion_index_, 0, static_cast<int>(matches.size()) - 1);
        input_ = apply_completion(input_, matches[completion_index_]);
        show_completion_ = false;
    }

    void move_selection(int delta) {
        if (show_completion_) {
            auto matches = completions_for(input_);
            if (!matches.empty()) {
                completion_index_ = (completion_index_ + delta + static_cast<int>(matches.size())) % static_cast<int>(matches.size());
            }
            return;
        }
        if (results_.empty()) return;
        selected_ = std::clamp(selected_ + delta, 0, static_cast<int>(results_.size()) - 1);
        ensure_selected_context_loaded();
        focus_preview_on_selected();
        adjust_result_scroll();
    }

    void adjust_result_scroll() {
        if (selected_ < result_scroll_) result_scroll_ = selected_;
        if (selected_ >= result_scroll_ + kResultVisible) result_scroll_ = selected_ - kResultVisible + 1;
        result_scroll_ = std::max(0, result_scroll_);
    }

    void ensure_selected_context_loaded() {
        if (results_.empty()) return;
        auto& item = results_[std::clamp(selected_, 0, static_cast<int>(results_.size()) - 1)];
        load_full_context_for_item(service_, item);
    }

    void focus_preview_on_selected() {
        if (results_.empty()) {
            preview_scroll_ = 0;
            return;
        }
        const auto& item = results_[std::clamp(selected_, 0, static_cast<int>(results_.size()) - 1)];
        const int base = item.context_content.empty() ? item.line_start : item.context_line_start;
        preview_scroll_ = std::max(0, item.line_start - base - 8);
    }

    void scroll_preview(int delta) {
        ensure_selected_context_loaded();
        preview_scroll_ = std::clamp(preview_scroll_ + delta, 0, preview_max_scroll());
    }

    int preview_max_scroll() const {
        if (results_.empty()) return 0;
        const auto& item = results_[std::clamp(selected_, 0, static_cast<int>(results_.size()) - 1)];
        const bool has_full_context = !item.context_content.empty();
        auto lines = split_lines(has_full_context ? item.context_content : item.content);
        return std::max(0, static_cast<int>(lines.size()) - kContextVisible);
    }

    void run_search() {
        auto parsed = parse_query(input_);
        error_.clear();
        if (parsed.keyword.empty()) {
            error_ = "请输入搜索关键词，例如 /api AddItem 或 /event damage";
            status_ = "No query";
            return;
        }

        const int generation = ++search_generation_;
        searching_ = true;
        show_completion_ = false;
        status_ = "Searching " + mode_title(parsed.mode) + " · " + parsed.keyword;
        last_query_ = parsed.mode + " " + parsed.keyword;

        auto service = service_;
        auto async = async_;
        auto screen = ftxui::ScreenInteractive::Active();
        std::thread([service, async, screen, parsed, generation] {
            SearchPayload payload;
            payload.generation = generation;
            payload.mode = parsed.mode;
            payload.keyword = parsed.keyword;
            auto start = std::chrono::steady_clock::now();
            try {
                payload.results = search_with_service(service, parsed);
            } catch (const std::exception& ex) {
                payload.error = ex.what();
            } catch (...) {
                payload.error = "未知搜索异常";
            }
            payload.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
            if (!async || !async->alive.load()) return;
            {
                std::lock_guard<std::mutex> lock(async->mutex);
                async->completed.push_back(std::move(payload));
            }
            if (screen) screen->PostEvent(ftxui::Event::Custom);
        }).detach();
    }

    void pull_completed_searches() {
        std::vector<SearchPayload> payloads;
        {
            std::lock_guard<std::mutex> lock(async_->mutex);
            payloads.swap(async_->completed);
        }
        for (auto& payload : payloads) {
            if (payload.generation != search_generation_.load()) continue;
            results_ = std::move(payload.results);
            selected_ = 0;
            result_scroll_ = 0;
            ensure_selected_context_loaded();
            focus_preview_on_selected();
            if (!payload.error.empty()) {
                error_ = payload.error;
                status_ = "Search failed · " + std::to_string(payload.elapsed_ms) + "ms";
            } else {
                error_ = results_.empty() ? "没有找到相关结果，尝试更短关键词或切换 /all。" : "";
                status_ = mode_title(payload.mode) + " · " + std::to_string(results_.size()) + " results · " + std::to_string(payload.elapsed_ms) + "ms";
            }
            searching_ = false;
        }
    }

    void tick_animation() {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count() >= 90) {
            ++spinner_tick_;
            last_tick_ = now;
        }
    }

    Element render() {
        pull_completed_searches();
        tick_animation();
        auto body = vbox({
            render_header(),
            separatorEmpty(),
            render_input(),
            render_completion(),
            hbox({
                render_results() | size(ftxui::WIDTH, ftxui::EQUAL, 48) | flex,
                separator() | color(line_col()),
                render_preview() | flex,
            }) | flex,
            separatorEmpty(),
            render_footer(),
        });
        if (searching_) ftxui::animation::RequestAnimationFrame();
        return body | bgcolor(bg_col()) | color(ftxui::Color::White) | clear_under;
    }

    Element render_header() const {
        std::ostringstream docs;
        docs << service_->doc_count() << " docs";
        std::ostringstream assets;
        assets << service_->game_assets_count() << " assets";
        return hbox({
            gradient_title() | flex,
            pill(cache_only_ ? "CACHE" : "LIVE", ftxui::Color::Black, cache_only_ ? ftxui::Color::Yellow : ftxui::Color::GreenLight),
            text(" "),
            pill(docs.str(), ftxui::Color::Black, ftxui::Color::Cyan),
            text(" "),
            pill(assets.str(), ftxui::Color::Black, ftxui::Color::BlueLight),
        });
    }

    Element render_input() const {
        auto parsed = parse_query(input_);
        auto prompt = hbox({
            text("  ❯ ") | color(cyan_col()) | bold,
            text(input_.empty() ? last_query_ : input_) | color(input_.empty() ? muted_col() : ftxui::Color::White) | flex,
            searching_ ? (text(" " + spinner_frame(spinner_tick_) + " ") | color(cyan_col()) | bold)
                       : (text(" █ ") | color(cyan_col())),
        });
        auto meta = hbox({
            text("  mode ") | color(muted_col()),
            pill(mode_title(parsed.mode), ftxui::Color::Black, cyan_col()),
            text("  /api /event /enum /wiki /dev /qumod /netease /all") | color(muted_col()),
        });
        return card("Query", vbox({prompt, meta}), searching_ ? cyan_col() : blue_col());
    }

    Element render_completion() const {
        if (!show_completion_) return text("");
        auto matches = completions_for(input_);
        if (matches.empty()) return text("");
        std::vector<Element> rows;
        for (int i = 0; i < static_cast<int>(matches.size()); ++i) {
            const auto& m = matches[i];
            auto row = hbox({
                text(i == completion_index_ ? "  › " : "    ") | color(cyan_col()),
                text(m.name) | bold | color(ftxui::Color::GreenLight),
                text("  "),
                text(m.title) | color(ftxui::Color::White),
                text("  "),
                text(shorten_ascii_safe(m.hint, 64)) | color(muted_col()),
            });
            if (i == completion_index_) row = row | bgcolor(panel_hot_col());
            rows.push_back(row);
        }
        return card("Completions", vbox(std::move(rows)), ftxui::Color::GreenLight);
    }

    Element render_results() const {
        std::vector<Element> rows;
        if (results_.empty()) {
            rows.push_back(text("  还没有结果") | color(muted_col()));
            rows.push_back(text("  输入关键词后按 Enter 搜索") | color(muted_col()));
        } else {
            int end = std::min(static_cast<int>(results_.size()), result_scroll_ + kResultVisible);
            for (int i = result_scroll_; i < end; ++i) {
                rows.push_back(render_result_row(i));
            }
            if (end < static_cast<int>(results_.size())) {
                rows.push_back(text("  ↓ more") | color(muted_col()));
            }
        }
        auto title = searching_ ? ("Results  " + spinner_frame(spinner_tick_) + " searching") : "Results";
        return card(title, vbox(std::move(rows)) | flex, purple_col()) | reflect(results_box_) | flex;
    }

    Element render_result_row(int index) const {
        const auto& item = results_[index];
        std::ostringstream score;
        score << std::fixed << std::setprecision(2) << item.score;
        std::string summary = shorten_ascii_safe(item.content, kMaxSummary);
        std::string file = shorten_ascii_safe(item.file, kMaxTitle);
        auto row = vbox({
            hbox({
                text(index == selected_ ? "❯ " : "  ") | color(cyan_col()) | bold,
                text(std::to_string(index + 1)) | color(ftxui::Color::GrayLight),
                text(" "),
                text(file) | color(ftxui::Color::DeepSkyBlue1) | bold,
                text(":" + std::to_string(item.line_start)) | color(muted_col()),
                text(" ") | flex,
                text(score.str()) | color(ftxui::Color::Yellow),
            }),
            hbox({text("   "), paragraph(summary) | color(ftxui::Color::GrayLight)}),
        });
        if (index == selected_) row = row | bgcolor(panel_hot_col());
        return row;
    }

    Element render_preview() const {
        if (results_.empty()) {
            std::vector<Element> intro = {
                text("像 MCP 一样直接搜索开发知识库") | bold | color(ftxui::Color::Cyan),
                text(""),
                text("命令示例：") | color(ftxui::Color::GrayLight),
                text("  /api SetBlock") | color(ftxui::Color::GreenLight),
                text("  /event damage") | color(ftxui::Color::GreenLight),
                text("  AddItem") | color(ftxui::Color::GreenLight),
                text(""),
                text("输入 / 可弹出命令补全，Tab 直接补全。") | color(muted_col()),
            };
            if (!error_.empty()) intro.push_back(text(error_) | color(ftxui::Color::RedLight) | bold);
            return card("Preview", vbox(std::move(intro)) | flex, cyan_col()) | flex;
        }

        const auto& item = results_[std::clamp(selected_, 0, static_cast<int>(results_.size()) - 1)];
        const bool has_full_context = !item.context_content.empty();
        auto lines = split_lines(has_full_context ? item.context_content : item.content);
        int max_scroll = std::max(0, static_cast<int>(lines.size()) - kContextVisible);
        int scroll = std::clamp(preview_scroll_, 0, max_scroll);
        int end = std::min(static_cast<int>(lines.size()), scroll + kContextVisible);
        int line_base = has_full_context ? item.context_line_start : item.line_start;

        std::vector<Element> content;
        content.push_back(hbox({
            pill(mode_from_file(item.file), ftxui::Color::Black, cyan_col()),
            text(" "),
            text(shorten_ascii_safe(item.file, kMaxPath)) | color(ftxui::Color::DeepSkyBlue1) | bold,
            text("  hit " + std::to_string(item.line_start) + "-" + std::to_string(item.line_end)) | color(muted_col()),
            text(" ") | flex,
            text(std::to_string(scroll + 1) + "-" + std::to_string(end) + "/" + std::to_string(lines.size())) | color(muted_col()),
        }));
        content.push_back(separator() | color(line_col()));
        for (int i = scroll; i < end; ++i) {
            std::ostringstream no;
            no << std::setw(4) << (line_base + i);
            content.push_back(hbox({
                text(no.str() + " │ ") | color(muted_col()),
                text(shorten_ascii_safe(lines[i], 120)) | color(ftxui::Color::GrayLight),
            }));
        }
        if (end < static_cast<int>(lines.size())) content.push_back(text("      ↓ PageDown 查看更多") | color(muted_col()));
        if (scroll > 0) content.push_back(text("      ↑ PageUp 查看上文") | color(muted_col()));
        content.push_back(text("      ←/→ 微调滚动  u/d 快速滚动  g/G 跳到顶部/底部") | color(muted_col()));
        return card("Context", vbox(std::move(content)) | flex, cyan_col()) | reflect(context_box_) | flex;
    }

    Element render_footer() const {
        auto status = hbox({
            pill("Enter", ftxui::Color::Black, ftxui::Color::Cyan), text(" 搜索  "),
            pill("/", ftxui::Color::Black, ftxui::Color::GreenLight), text(" 命令  "),
            pill("Tab", ftxui::Color::Black, ftxui::Color::Yellow), text(" 补全  "),
            pill("↑↓", ftxui::Color::Black, ftxui::Color::Magenta), text(" 结果  "),
            pill("PgUp/PgDn", ftxui::Color::Black, ftxui::Color::BlueLight), text(" 上下文  "),
            pill("g/G", ftxui::Color::Black, ftxui::Color::Cyan), text(" 跳转  "),
            pill("Esc", ftxui::Color::Black, ftxui::Color::RedLight), text(" 退出"),
            text("  " + status_) | color(ftxui::Color::GrayLight),
            error_.empty() ? text("") : (text("  " + error_) | color(ftxui::Color::RedLight) | bold),
        });
        return vbox({status});
    }
};

} // namespace mcdk::search_ui
