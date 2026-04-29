#include "search_app.hpp"

namespace mcdk::search_ui {

#ifdef _WIN32
struct ConsoleInputModeGuard {
    HANDLE handle = INVALID_HANDLE_VALUE;
    DWORD original_mode = 0;
    bool active = false;

    void disable_processed_input() {
        handle = GetStdHandle(STD_INPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE) return;
        if (!GetConsoleMode(handle, &original_mode)) return;
        DWORD mode = original_mode;
        mode &= ~ENABLE_PROCESSED_INPUT;
        if (SetConsoleMode(handle, mode)) active = true;
    }

    ~ConsoleInputModeGuard() {
        if (active && handle != INVALID_HANDLE_VALUE) {
            SetConsoleMode(handle, original_mode);
        }
    }
};
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

    auto screen = ftxui::ScreenInteractive::FullscreenAlternateScreen();
    screen.TrackMouse(false);
    screen.ForceHandleCtrlC(false);

#ifdef _WIN32
    ConsoleInputModeGuard guard;
    guard.disable_processed_input();
#endif

    SearchApp app(service, paths, cache_only);
    screen.Loop(app.component());
    return 0;
}
