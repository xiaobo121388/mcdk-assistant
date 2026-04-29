#include "search_app.hpp"

namespace mcdk::search_ui {

#ifdef _WIN32
// 不再主动关闭 ENABLE_PROCESSED_INPUT。
// 之前这样做虽然能压住 Ctrl+C 的默认行为，但会让部分 cmd/IME 组合下
// 中英文切换、候选上屏出现异常。当前改为依赖 ControlHandler + FTXUI 自身事件处理。
#endif

} // namespace mcdk::search_ui

int main() {
    using namespace mcdk::search_ui;

    init_console_encoding();

    auto paths = detect_paths();
    const bool dicts_ok = has_dicts(paths.dicts_dir);
    const bool knowledge_ok = !paths.knowledge_dir.empty() && fs::exists(paths.knowledge_dir);
    const bool cache_ok = fs::is_regular_file(paths.cache_path);

    if (!dicts_ok || (!knowledge_ok && !cache_ok)) {
        auto screen = ftxui::ScreenInteractive::TerminalOutput();
        auto message = ftxui::Renderer([&] {
            return window(text(" MCDK Search startup error ") | color(ftxui::Color::RedLight) | bold,
                vbox({
                    text("无法找到运行所需资源。") | color(ftxui::Color::RedLight) | bold,
                    text(""),
                    text("需要 dicts 目录，以及 knowledge 目录或 mcdk_index_cache.bin 缓存。"),
                    text("dicts: " + mcdk::path::to_utf8(paths.dicts_dir)),
                    text("knowledge: " + mcdk::path::to_utf8(paths.knowledge_dir)),
                    text("cache: " + mcdk::path::to_utf8(paths.cache_path)),
                    text(""),
                    text("按 Esc 退出。") | color(ftxui::Color::GrayLight),
                })) | color(ftxui::Color::RedLight);
        });
        auto app = ftxui::CatchEvent(message, [](ftxui::Event e) {
            if (e == ftxui::Event::Escape || e == ftxui::Event::Return) {
                ftxui::ScreenInteractive::Active()->ExitLoopClosure()();
                return true;
            }
            return false;
        });
        screen.Loop(app);
        return 1;
    }

    bool cache_only = !knowledge_ok && cache_ok;
    std::shared_ptr<mcdk::SearchService> service;
    if (cache_only) {
        service = std::make_shared<mcdk::SearchService>(paths.dicts_dir, paths.cache_path, true);
    } else {
        service = std::make_shared<mcdk::SearchService>(paths.dicts_dir, paths.knowledge_dir, paths.cache_path, false);
    }

    // 在 VSCode 内嵌终端中 AlternateScreen 体验更好，但原生 conhost/cmd 黑窗里
    // 更容易影响中文输入法切换与候选上屏；这里改回 PrimaryScreen 提高兼容性。
    auto screen = ftxui::ScreenInteractive::FullscreenPrimaryScreen();
    screen.TrackMouse(true);
    screen.ForceHandleCtrlC(false);
    screen.SelectionChange([&screen] {
        auto selected = mcdk::search_ui::sanitize_selection_text(screen.GetSelection());
        if (!selected.empty()) {
            mcdk::search_ui::copy_text_to_clipboard(selected);
        }
    });

    SearchApp app(service, paths, cache_only);
    screen.Loop(app.component());
    return 0;
}
