#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "core/active_time_tracker.h"
#include "core/difficulty.h"
#include "core/game_session.h"
#include "core/settings.h"
#include "persist/paths.h"
#include "persist/store.h"
#include "ui/app.h"
#include "ui/screens/board_screen.h"
#include "ui/screens/new_game_screen.h"
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
    AppImpl(minesweeper::Renderer& r, minesweeper::ui::Theme theme,
            minesweeper::persist::Paths paths)
        : renderer_(r), theme_(std::move(theme)), paths_(std::move(paths)) {
        if (auto text = minesweeper::persist::loadFile(paths_.settings)) {
            try {
                settings_ = minesweeper::core::Settings::fromJson(*text);
            } catch (const std::exception& e) {
                std::fprintf(stderr, "settings.json rejected: %s\n", e.what());
                minesweeper::persist::removeFile(paths_.settings);
            }
        }
        minesweeper::ui::applyColorMode(theme_, renderer_.info(), settings_.colorMode);
        minesweeper::ui::applyScreenRefreshInterval(renderer_, settings_.screenRefreshInterval);

        if (auto text = minesweeper::persist::loadFile(paths_.game)) {
            try {
                session_ = minesweeper::core::GameSession::fromJson(*text);
            } catch (const std::exception& e) {
                // Corrupt save: log, drop the file, degrade to a fresh NotStarted session.
                // settings_ above is loaded/validated independently, so it is never affected.
                std::fprintf(stderr, "game.json rejected: %s\n", e.what());
                minesweeper::persist::removeFile(paths_.game);
            }
        }
    }

    minesweeper::Renderer& renderer() override { return renderer_; }
    const minesweeper::ui::Theme& theme() const override { return theme_; }

    minesweeper::core::GameSession& session() override { return session_; }
    void autosaveSession() override {
        minesweeper::persist::saveFileAtomic(paths_.game, session_.toJson());
    }
    bool hasInProgressGame() const override {
        return session_.status() == minesweeper::core::Board::Status::InProgress;
    }
    void startNewGame(minesweeper::core::DifficultyConfig cfg) override {
        session_ = minesweeper::core::GameSession(cfg);
        autosaveSession();
    }

    minesweeper::core::Settings& settings() override { return settings_; }
    void autosaveSettings() override {
        minesweeper::persist::saveFileAtomic(paths_.settings, settings_.toJson());
        minesweeper::ui::applyColorMode(theme_, renderer_.info(), settings_.colorMode);
        minesweeper::ui::applyScreenRefreshInterval(renderer_, settings_.screenRefreshInterval);
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
    void returnToMainMenu() override {
        session_ = minesweeper::core::GameSession();  // fresh NotStarted session (FR-006)
        autosaveSession();
        stack_.clear();
        push(std::make_unique<minesweeper::ui::NewGameScreen>(*this));
    }

    // --- app-shell surface (not part of the Screen-facing interface) ---
    minesweeper::ui::Screen* top() { return stack_.empty() ? nullptr : stack_.back().get(); }
    bool exitRequested() const { return exitRequested_; }
    bool consumeNavDirty() { bool v = navDirty_; navDirty_ = false; return v; }

private:
    minesweeper::Renderer& renderer_;
    minesweeper::ui::Theme theme_;
    minesweeper::persist::Paths paths_;

    minesweeper::core::GameSession session_;
    minesweeper::core::Settings settings_;

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

    if (app.hasInProgressGame() ||
        app.session().status() == minesweeper::core::Board::Status::Won ||
        app.session().status() == minesweeper::core::Board::Status::Lost) {
        app.push(std::make_unique<minesweeper::ui::BoardScreen>(app));
    } else {
        app.push(std::make_unique<minesweeper::ui::NewGameScreen>(app));
    }
    app.consumeNavDirty();
    app.top()->draw();
    renderer.flushFull();

    const int kTimeoutMs = 20000;       // wakes the loop for periodic housekeeping
    const int kSleepGapMs = 45000;      // wall-clock gap => device slept
    auto lastWall = std::chrono::system_clock::now();
    minesweeper::core::ActiveTimeTracker activeTimeTracker(std::chrono::steady_clock::now());

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
        lastWall = nowWall;
        bool slept = wallMs > kTimeoutMs + kSleepGapMs || wallMs < 0;

        minesweeper::ui::Screen* screen = app.top();

        if (slept || sdlRedraw) {
            // Wake-from-sleep: the sleep screen may cover us; repaint fully.
            sdlRedraw = false;
            screen->draw();
            renderer.flushFull();
        }

        // Active-time bookkeeping runs every iteration -- tap or not -- so no wall-clock
        // interval between two loop iterations is ever silently dropped (previously only
        // no-tap iterations fed onTick, undercounting elapsed time during tap-heavy play; see
        // specs/003-fix-timer-hide-option/research.md #1). Only screens that count play time
        // accumulate active seconds, and device-sleep gaps are excluded (FR-016 clarified pause
        // semantics), matching Screen::countsPlayTime()'s contract.
        uint32_t activeSeconds =
            activeTimeTracker.tick(nowSteady, !slept && screen->countsPlayTime());
        screen->onTick(activeSeconds);

        if (tap) {
            lastTapSteady = nowSteady;
            screen->onTap(*tap);
        } else {
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
    app.autosaveSession();
    app.autosaveSettings();
    return 0;
#endif
}
