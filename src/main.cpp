#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "core/counter.h"
#include "persist/paths.h"
#include "persist/store.h"
#include "ui/app.h"
#include "ui/screens/counter_screen.h"
#include "ui/theme.h"

#if defined(MINESWEEPER_BACKEND_SDL)
#include "platform/sdl/mouse_touch.h"
#include "platform/sdl/sdl_renderer.h"
#elif defined(MINESWEEPER_BACKEND_FBINK)
#include "platform/kobo/evdev_touch.h"
#include "platform/kobo/fbink_renderer.h"
#endif

namespace {

volatile std::sig_atomic_t g_signalled = 0;
void onSignal(int) { g_signalled = 1; }  // persist-and-exit (device sleep/power)

struct Options {
    int width = 1264, height = 1680, dpi = 300;  // Kobo Libra Colour geometry
    bool color = true;
    const char* dataDir = nullptr;
    const char* assetsDir = nullptr;
};

Options parseArgs(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        auto next = [&](int& out) { if (i + 1 < argc) out = std::atoi(argv[++i]); };
        if (!std::strcmp(argv[i], "--width")) next(o.width);
        else if (!std::strcmp(argv[i], "--height")) next(o.height);
        else if (!std::strcmp(argv[i], "--dpi")) next(o.dpi);
        else if (!std::strcmp(argv[i], "--gray")) o.color = false;
        else if (!std::strcmp(argv[i], "--data-dir") && i + 1 < argc) o.dataDir = argv[++i];
        else if (!std::strcmp(argv[i], "--assets") && i + 1 < argc) o.assetsDir = argv[++i];
    }
    return o;
}

std::string resolveAssetsDir(const Options& o) {
    if (o.assetsDir) return o.assetsDir;
    if (const char* env = std::getenv("MINESWEEPER_ASSETS_DIR"); env && *env) return env;
#if defined(MINESWEEPER_BACKEND_FBINK)
    return "/mnt/onboard/.adds/minesweeper/assets";
#else
    return "dist/.adds/minesweeper/assets";
#endif
}

class AppImpl : public minesweeper::ui::App {
public:
    AppImpl(minesweeper::Renderer& r, const minesweeper::ui::Theme& theme,
            minesweeper::persist::Paths paths)
        : renderer_(r), theme_(theme), paths_(std::move(paths)) {
        if (auto text = minesweeper::persist::loadFile(paths_.counter)) {
            try {
                counter_ = minesweeper::core::Counter::fromJson(*text);
            } catch (const std::exception& e) {
                // Corrupt save: log, drop the file, degrade to a fresh counter.
                std::fprintf(stderr, "counter.json rejected: %s\n", e.what());
                minesweeper::persist::removeFile(paths_.counter);
            }
        }
    }

    minesweeper::Renderer& renderer() override { return renderer_; }
    const minesweeper::ui::Theme& theme() const override { return theme_; }

    minesweeper::core::Counter& counter() override { return counter_; }
    void autosave() override {
        minesweeper::persist::saveFileAtomic(paths_.counter, counter_.toJson());
    }

    void push(std::unique_ptr<minesweeper::ui::Screen> s) override {
        stack_.push_back(std::move(s));
        navDirty_ = true;
    }
    void pop() override {
        if (!stack_.empty()) stack_.pop_back();
        navDirty_ = true;
    }
    void requestExit() override { exitRequested_ = true; }

    // --- app-shell surface (not part of the Screen-facing interface) ---
    minesweeper::ui::Screen* top() { return stack_.empty() ? nullptr : stack_.back().get(); }
    bool exitRequested() const { return exitRequested_; }
    bool consumeNavDirty() { bool v = navDirty_; navDirty_ = false; return v; }

private:
    minesweeper::Renderer& renderer_;
    const minesweeper::ui::Theme& theme_;
    minesweeper::persist::Paths paths_;

    minesweeper::core::Counter counter_;

    std::vector<std::unique_ptr<minesweeper::ui::Screen>> stack_;
    bool navDirty_ = false;
    bool exitRequested_ = false;
};

}  // namespace

int main(int argc, char** argv) {
    Options opt = parseArgs(argc, argv);
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    std::string assets = resolveAssetsDir(opt);
    minesweeper::persist::Paths paths = minesweeper::persist::resolveDataDir(opt.dataDir);

    bool sdlQuit = false, sdlRedraw = false;

#if defined(MINESWEEPER_BACKEND_SDL)
    minesweeper::SdlRenderer renderer;
    minesweeper::ui::Theme theme = minesweeper::ui::makeTheme(
        {opt.width, opt.height, opt.dpi, opt.color}, assets);
    if (!renderer.init(opt.width, opt.height, opt.dpi, opt.color, theme.fontPath,
                       theme.fontBoldPath)) {
        std::fprintf(stderr, "renderer init failed (fonts at %s?)\n", assets.c_str());
        return 1;
    }
    minesweeper::MouseTouch touch(&sdlQuit, &sdlRedraw);
#elif defined(MINESWEEPER_BACKEND_FBINK)
    minesweeper::FbinkRenderer renderer;
    if (!renderer.init(assets)) {
        std::fprintf(stderr, "FBInk init failed\n");
        return 1;
    }
    minesweeper::ui::Theme theme = minesweeper::ui::makeTheme(renderer.info(), assets);
    minesweeper::EvdevTouch touch;
    if (!touch.init(renderer.info())) {
        std::fprintf(stderr, "touch input init failed\n");
        return 1;
    }
#else
    (void)sdlQuit; (void)sdlRedraw;
    std::fprintf(stderr, "built without a backend (MINESWEEPER_BACKEND=none)\n");
    return 1;
#endif

#if defined(MINESWEEPER_BACKEND_SDL) || defined(MINESWEEPER_BACKEND_FBINK)
    AppImpl app(renderer, theme, paths);
    app.push(std::make_unique<minesweeper::ui::CounterScreen>(app));
    app.consumeNavDirty();
    app.top()->draw();
    renderer.flushFull();

    const int kTimeoutMs = 20000;       // wakes the loop for periodic housekeeping
    const int kSleepGapMs = 45000;      // wall-clock gap => device slept
    auto lastSteady = std::chrono::steady_clock::now();
    auto lastWall = std::chrono::system_clock::now();

#if defined(MINESWEEPER_BACKEND_FBINK)
    // Nickel is paused for as long as we're in the foreground (start.sh), so
    // it can't run its own inactivity/sleep timer either. Staying frozen
    // indefinitely risks a lower-level watchdog forcing a hard power-off
    // instead of a graceful suspend, so give control back on our own after a
    // stretch of no taps. Override via env var; 0 disables.
    int64_t idleExitMs = 300000;  // 5 min
    if (const char* v = std::getenv("MINESWEEPER_IDLE_EXIT_SEC"); v && *v) {
        int sec = std::atoi(v);
        idleExitMs = sec > 0 ? static_cast<int64_t>(sec) * 1000 : 0;
    }
#else
    int64_t idleExitMs = 0;
#endif
    auto lastTapSteady = std::chrono::steady_clock::now();

    while (!app.exitRequested() && !g_signalled && !sdlQuit && app.top()) {
        std::optional<minesweeper::Tap> tap = touch.waitForTap(kTimeoutMs);

        auto nowSteady = std::chrono::steady_clock::now();
        auto nowWall = std::chrono::system_clock::now();
        int64_t wallMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(nowWall - lastWall).count();
        lastSteady = nowSteady;
        lastWall = nowWall;
        bool slept = wallMs > kTimeoutMs + kSleepGapMs || wallMs < 0;

        minesweeper::ui::Screen* screen = app.top();

        if (slept || sdlRedraw) {
            // Wake-from-sleep: the sleep screen may cover us; repaint fully.
            sdlRedraw = false;
            screen->draw();
            renderer.flushFull();
        }

        if (tap) {
            lastTapSteady = nowSteady;
            screen->onTap(*tap);
        } else {
            screen->onTick(0);
#if defined(MINESWEEPER_BACKEND_FBINK)
            // Any touchscreen activity counts, not just a completed tap (a
            // stray touch, a drag, or sensor noise while resting a finger
            // never reaches screen->onTap otherwise, but the user is still
            // clearly present).
            if (touch.lastActivity() > lastTapSteady) lastTapSteady = touch.lastActivity();
#endif
            int64_t idleMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  nowSteady - lastTapSteady)
                                  .count();
            if (idleExitMs > 0 && idleMs >= idleExitMs) {
                std::fprintf(stderr, "idle-exit: %lld ms since last activity (limit %lld)\n",
                             static_cast<long long>(idleMs), static_cast<long long>(idleExitMs));
                app.requestExit();
            }
        }

        if (app.consumeNavDirty()) {
            if (!app.top()) break;
            app.top()->draw();
            renderer.flushFull();  // screen transition: clean full refresh
        }
    }

    // Never lose progress: persist on every exit path (Constitution V).
    app.autosave();
    return 0;
#endif
}
