#pragma once
#include <string>

#include "fbink.h"
#include "platform/canvas_renderer.h"

namespace minesweeper {

// Device backend: the shared canvas is pushed to the e-ink framebuffer via
// FBInk (fbink_print_raw_data + fbink_refresh). Owns the waveform choice and
// the ghosting policy: after kGhostingPartials partial refreshes, the next
// flushPartial is promoted to a flashing full refresh.
class FbinkRenderer : public CanvasRenderer {
public:
    ~FbinkRenderer() override;

    bool init(const std::string& assetsDir);

    DisplayInfo info() const override { return info_; }
    void flushPartial(Rect r) override;
    void flushFull() override;
    void setGhostingInterval(int n) override;

private:
    void pushRegion(Rect r, bool flash);

    int fbfd_ = -1;
    DisplayInfo info_{};
    int partialCount_ = 0;
    int ghostingPartials_ = 12;  // set from settings/config at startup, if any

    // Snapshot of Nickel's screen taken in init(), restored on teardown.
    // Nickel is SIGSTOPped while we run (start.sh) and does not repaint on
    // SIGCONT, so we have to put its pixels back ourselves before exiting.
    FBInkDump nickelDump_{};
    bool haveNickelDump_ = false;
};

}  // namespace minesweeper
