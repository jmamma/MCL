#include <stdint.h>
#include <stdio.h>
#include "MCL/Sequencer/SeqExtStepTypes.h"
#if defined(__AVR__)
#include <avr/io.h>
static_assert(sizeof(int) == 2, "AVR test must exercise 16-bit arithmetic");
#endif
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

static void report(const char* text) {
#if defined(__AVR__)
    while (*text) {
        while (!(UCSR0A & (1 << UDRE0))) {}
        UDR0 = *text++;
    }
#else
    fputs(text, stdout);
#endif
}

struct Link {
    uint8_t row = 0, length = 16, loops = 1, speed = 12;
    uint8_t speed_value() const { return speed & 0x7f; }
};
struct GridTrack {
    Link link;
    bool load_sound() const { return !(link.speed & 0x80); }
    void set_load_sound(bool value) {
        link.speed = value ? link.speed & 0x7f : link.speed | 0x80;
    }
};
struct SeqTrack {
    static uint8_t get_speed_multiplier_int(uint8_t speed) { return speed; }
};
// SLOT_FLAGS
// SLOT_UPDATE

static bool test_loops() {
    const uint8_t lengths[]{1, 2, 3, 16, 64, 128};
    const uint8_t repeats[]{0, 1, 2, 8, 32, 63};
    const uint8_t speeds[]{3, 12, 96};
    for (uint8_t source_length : lengths) for (uint8_t target_length : lengths)
    for (uint8_t source_speed : speeds) for (uint8_t target_speed : speeds)
    for (uint8_t loops : repeats) {
        GridTrack source, target;
        source.link.length = source_length; source.link.speed = source_speed;
        source.link.loops = loops;
        target.link.length = target_length; target.link.speed = target_speed;
        target.link.loops = 19;
        const uint32_t source_duration = uint32_t(source_length) * source_speed * loops / 12;
        const uint32_t target_duration = uint32_t(target_length) * target_speed / 12;
        const bool changed = apply_slot_dirty_fields(target, source, SLOT_DIRTY_LOOPS);
        if (!loops) {
            if (!changed || target.link.loops != 0) return false;
        } else if (!target_duration) {
            if (changed || target.link.loops != 19) return false;
        } else {
            uint32_t expected = source_duration / target_duration;
            if (source_duration % target_duration || expected == 0 || expected > 255)
                expected = loops; // Same fallback as a nonintegral duration match.
            if (!changed || target.link.loops != expected) return false;
        }
        if (target.link.length != target_length || target.link.speed != target_speed)
            return false;
    }
    // The highest storable count is valid; the next value must not wrap.
    GridTrack source, target;
    source.link.length = 85; source.link.loops = 3;
    target.link.length = 1;
    if (!apply_slot_dirty_fields(target, source, SLOT_DIRTY_LOOPS) ||
        target.link.loops != 255) return false;
    source.link.length = 64; source.link.loops = 4;
    if (!apply_slot_dirty_fields(target, source, SLOT_DIRTY_LOOPS) ||
        target.link.loops != 4) return false;
    return true;
}

constexpr uint8_t WHITE = 1, INVERT = 2, kExtStepVisibleSteps = 16;
struct ActiveTrack {
    uint8_t length_ = 128, ticks_ = 12, step_ = 0, mod_ = 0;
    uint8_t length() const { return length_; }
    uint16_t ticks_per_step() const { return ticks_; }
    uint8_t step_count() const { return step_; }
    uint8_t mod_ticks() const { return mod_; }
} active;
static ActiveTrack active_ext_step_track() { return active; }
struct Display {
    int16_t start = 0, span = 0, playhead = 0;
    void drawRect(int16_t, int16_t, int16_t, int16_t, uint8_t) {}
    void drawFastHLine(int16_t x, int16_t, int16_t w, uint8_t) { start = x; span = w; }
    void drawPixel(int16_t x, int16_t, uint8_t) { playhead = x; }
} oled_display;
struct SeqExtStepPage {
    // MINIMAP_WIDTH
    static constexpr uint8_t pidx_x0 = 0, pidx_y = 0, pidx_h = 4;
    seq_extstep_tick_t fov_offset = 0, fov_length = 0;
    void draw_viewport_minimap();
};
// MINIMAP

static bool test_minimap() {
    const uint8_t lengths[]{16, 32, 64, 128};
    const uint8_t speeds[]{3, 6, 8, 12, 16, 24, 48, 96};
    const uint8_t zooms[]{16, 32};
    for (uint8_t length : lengths) for (uint8_t speed : speeds)
    for (uint8_t zoom : zooms) {
        active.length_ = length; active.ticks_ = speed;
        SeqExtStepPage page;
        page.fov_length = min(zoom, length) * speed;
        for (uint8_t section = 0; section < 4; ++section) {
            page.fov_offset = (uint16_t(length) * speed - page.fov_length) * section / 3;
            for (uint8_t phase = 0; phase < 3; ++phase) {
                active.step_ = (length - 1) * phase / 2;
                active.mod_ = speed - 1;
                page.draw_viewport_minimap();
                const uint32_t width = SeqExtStepPage::pidx_w * 4 + 3;
                const uint32_t end = uint32_t(length) * speed;
                const uint32_t tick = uint32_t(active.step_) * speed + active.mod_;
                if (oled_display.start != 1 + uint32_t(page.fov_offset) * (width - 1) / end ||
                    oled_display.span != uint32_t(page.fov_length) * (width - 2) / end ||
                    oled_display.playhead != 1 + tick * (width - 1) / end) return false;
            }
        }
    }
    return true;
}

int main() {
#if defined(__AVR__)
    UBRR0 = 103;
    UCSR0B = 1 << TXEN0;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
#endif
    const bool loops = test_loops(), minimap = test_minimap();
    report(loops ? "PASS: loop counts\n" : "FAIL: loop counts\n");
    report(minimap ? "PASS: minimap\n" : "FAIL: minimap\n");
#if defined(__AVR__)
    for (;;) {}
#else
    return loops && minimap ? 0 : 1;
#endif
}
