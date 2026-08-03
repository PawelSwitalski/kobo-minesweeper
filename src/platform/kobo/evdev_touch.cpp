#include "platform/kobo/evdev_touch.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

// The koxtoolchain cross-sysroot's linux/input.h predates the MT-B slot protocol constant;
// 0x2f is ABS_MT_SLOT's stable, longstanding value from the upstream kernel UAPI header
// (include/uapi/linux/input-event-codes.h), safe to hardcode as a fallback.
#ifndef ABS_MT_SLOT
#define ABS_MT_SLOT 0x2f
#endif

namespace minesweeper {

namespace {
bool hasAbsAxis(int fd, unsigned axis) {
    unsigned char bits[(ABS_MAX + 7) / 8] = {};
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof bits), bits) < 0) return false;
    return bits[axis / 8] & (1u << (axis % 8));
}

bool envFlag(const char* name) {
    const char* v = getenv(name);
    return v && *v && *v != '0';
}

// A Kobo touch panel's slot count is small in practice (2-10); this is a defensive cap, not a
// hardware limit -- extra slots beyond it are simply never tracked (equivalent to a very large
// number of "ignored third+ fingers", already a defined case for GestureRecognizer).
constexpr int kMaxSlots = 16;

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}  // namespace

EvdevTouch::~EvdevTouch() {
    if (fd_ >= 0) close(fd_);
}

bool EvdevTouch::init(const DisplayInfo& display) {
    viewW_ = display.width;
    viewH_ = display.height;
    swapXY_ = envFlag("MINESWEEPER_TOUCH_SWAP_XY");
    mirrorX_ = envFlag("MINESWEEPER_TOUCH_MIRROR_X");
    mirrorY_ = envFlag("MINESWEEPER_TOUCH_MIRROR_Y");
    debug_ = envFlag("MINESWEEPER_TOUCH_DEBUG");

    for (int i = 0; i < 12; ++i) {
        char path[32];
        std::snprintf(path, sizeof path, "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        bool mt = hasAbsAxis(fd, ABS_MT_POSITION_X);
        bool st = hasAbsAxis(fd, ABS_X);
        if (!mt && !st) {
            close(fd);
            continue;
        }
        unsigned xAxis = mt ? ABS_MT_POSITION_X : ABS_X;
        unsigned yAxis = mt ? ABS_MT_POSITION_Y : ABS_Y;
        input_absinfo ax{}, ay{};
        if (ioctl(fd, EVIOCGABS(xAxis), &ax) < 0 || ioctl(fd, EVIOCGABS(yAxis), &ay) < 0) {
            close(fd);
            continue;
        }
        // Nickel keeps running underneath (nothing stops it); without an
        // exclusive grab it reads the same taps we do, so touches leak
        // through to the library/Settings behind our screen.
        if (ioctl(fd, EVIOCGRAB, 1) < 0 && debug_)
            std::fprintf(stderr, "touch: %s EVIOCGRAB failed: %s\n", path, strerror(errno));

        fd_ = fd;
        multiTouch_ = mt;
        rawMinX_ = ax.minimum; rawMaxX_ = ax.maximum;
        rawMinY_ = ay.minimum; rawMaxY_ = ay.maximum;

        int slotCount = 1;
        if (mt) {
            input_absinfo aslot{};
            if (ioctl(fd, EVIOCGABS(ABS_MT_SLOT), &aslot) == 0 && aslot.maximum >= aslot.minimum) {
                slotCount = aslot.maximum - aslot.minimum + 1;
            } else {
                slotCount = 2;  // conservative fallback: enough to recognize a pinch
            }
            slotCount = std::min(slotCount, kMaxSlots);
        }
        slots_.assign(static_cast<size_t>(slotCount), Slot{});
        currentSlot_ = 0;

        if (debug_)
            std::fprintf(stderr, "touch: %s mt=%d slots=%d x[%d..%d] y[%d..%d]\n", path, mt,
                         slotCount, rawMinX_, rawMaxX_, rawMinY_, rawMaxY_);
        return true;
    }
    std::fprintf(stderr, "touch: no touchscreen found under /dev/input\n");
    return false;
}

std::pair<int, int> EvdevTouch::toDisplay(int rawX, int rawY) const {
    int x = rawX, y = rawY;
    int maxX = rawMaxX_, minX = rawMinX_, maxY = rawMaxY_, minY = rawMinY_;
    if (swapXY_) {
        int t = x; x = y; y = t;
        t = maxX; maxX = maxY; maxY = t;
        t = minX; minX = minY; minY = t;
    }
    int px = (maxX > minX) ? static_cast<int>(static_cast<long long>(x - minX) * (viewW_ - 1) /
                                               (maxX - minX))
                            : x;
    int py = (maxY > minY) ? static_cast<int>(static_cast<long long>(y - minY) * (viewH_ - 1) /
                                               (maxY - minY))
                            : y;
    if (mirrorX_) px = viewW_ - 1 - px;
    if (mirrorY_) py = viewH_ - 1 - py;
    return {px, py};
}

std::optional<std::variant<Tap, core::GestureEvent>> EvdevTouch::waitForEvent(int timeoutMs) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    while (true) {
        int remaining = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now())
                .count());
        // A held touch, a slow drag, or sensor noise can keep producing
        // events without ever completing a down+up cycle; without capping
        // to the caller's original budget here, that would block the main
        // loop (and its idle-exit check) far longer than timeoutMs.
        if (remaining <= 0) return std::nullopt;

        pollfd pfd{fd_, POLLIN, 0};
        int rc = poll(&pfd, 1, remaining);
        if (rc <= 0) return std::nullopt;  // timeout, or EINTR (SIGTERM path)

        input_event ev[32];
        ssize_t n = read(fd_, ev, sizeof ev);
        if (n <= 0) return std::nullopt;
        lastActivity_ = std::chrono::steady_clock::now();

        for (size_t k = 0; k < static_cast<size_t>(n) / sizeof(input_event); ++k) {
            const input_event& e = ev[k];
            if (e.type == EV_ABS) {
                if (multiTouch_) {
                    switch (e.code) {
                        case ABS_MT_SLOT:
                            currentSlot_ = std::clamp(static_cast<int>(e.value), 0,
                                                       static_cast<int>(slots_.size()) - 1);
                            break;
                        case ABS_MT_TRACKING_ID:
                            slots_[currentSlot_].trackingId = e.value;  // < 0 means lifted
                            break;
                        case ABS_MT_POSITION_X: slots_[currentSlot_].rawX = e.value; break;
                        case ABS_MT_POSITION_Y: slots_[currentSlot_].rawY = e.value; break;
                        default: break;
                    }
                } else {
                    switch (e.code) {
                        case ABS_X: slots_[0].rawX = e.value; break;
                        case ABS_Y: slots_[0].rawY = e.value; break;
                        default: break;
                    }
                }
            } else if (e.type == EV_KEY && e.code == BTN_TOUCH && !multiTouch_) {
                slots_[0].trackingId = (e.value != 0) ? 0 : -1;
            } else if (e.type == EV_SYN && e.code == SYN_REPORT) {
                std::vector<core::TouchPoint> points;
                for (size_t s = 0; s < slots_.size(); ++s) {
                    if (slots_[s].trackingId < 0) continue;
                    auto [px, py] = toDisplay(slots_[s].rawX, slots_[s].rawY);
                    points.push_back({static_cast<int>(s), px, py});
                }

                auto result = gestureRecognizer_.feed(points, nowMs());
                if (!result) continue;

                if (auto* tap = std::get_if<core::GestureTap>(&*result)) {
                    if (debug_)
                        std::fprintf(stderr, "tap=(%d,%d) longPress=%d\n", tap->x, tap->y,
                                     tap->longPress);
                    return Tap{tap->x, tap->y, tap->longPress};
                }
                auto* gesture = std::get_if<core::GestureEvent>(&*result);
                if (debug_)
                    std::fprintf(stderr, "gesture kind=%d zoomDelta=%d d=(%d,%d)\n",
                                 static_cast<int>(gesture->kind), gesture->zoomDelta,
                                 gesture->dxPx, gesture->dyPx);
                return *gesture;
            }
        }
    }
}

}  // namespace minesweeper
