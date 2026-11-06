#include <stddef.h>
#include <stdint.h>
#if defined(__AVR__)
#include <avr/io.h>
static_assert(sizeof(size_t) == 2, "regression requires real AVR size_t");
#else
#include <stdio.h>
#endif

// GRID_CONSTANTS
using GridRow = uint8_t;

// A streaming file double avoids storing a 64-KiB row in AVR's 8-KiB SRAM.
// The expected byte depends on its absolute position, including row and slot.
static uint8_t pattern(uint32_t at) {
    return uint8_t(1u + (at ^ (at >> 8) ^ (at >> 16)) % 255u);
}
struct Grid {
    uint32_t position = 0, bytes = 0;
    uint16_t calls = 0, fail_at = 65535;
    bool seek_ok = true;
    bool seek(uint8_t column, GridRow row) {
        position = ((uint32_t(row) * 17u) + column + 1u) * 4096u;
        return seek_ok;
    }
    bool read(void* data, size_t size) {
        if (calls++ == fail_at) return false;
        auto* out = static_cast<uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) out[i] = pattern(position++);
        bytes += size;
        return true;
    }
    bool write(void* data, size_t size) {
        if (calls++ == fail_at) return false;
        const auto* in = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i)
            if (in[i] != pattern(position++)) return false;
        bytes += size;
        return true;
    }
};

// ROW_COPIER

static bool test() {
    const uint8_t rows[]{0, 64, 127};
    for (uint8_t row : rows) {
        Grid src, dst;
        if (!copy_grid_row_slots_raw(src, dst, row) || src.bytes != 65536UL ||
            dst.bytes != 65536UL || src.position != dst.position) return false;
    }
    for (uint8_t failure = 0; failure < 4; ++failure) {
        Grid src, dst;
        if (failure == 0) src.seek_ok = false;
        if (failure == 1) dst.seek_ok = false;
        if (failure == 2) src.fail_at = 129; // Beyond halfway through the row.
        if (failure == 3) dst.fail_at = 255; // Last chunk must not be skipped.
        if (copy_grid_row_slots_raw(src, dst, 127)) return false;
        if (failure < 2 && (src.bytes || dst.bytes)) return false;
        if (failure == 2 && (src.bytes != 33024UL || dst.bytes != 33024UL)) return false;
        if (failure == 3 && (src.bytes != 65536UL || dst.bytes != 65280UL)) return false;
    }
    return true;
}

int main() {
    const bool ok = test();
    const char* message = ok ? "PASS: complete rows and I/O failures\n"
                             : "FAIL: row copy lost data or ignored an I/O failure\n";
#if defined(__AVR__)
    UBRR0 = 103;
    UCSR0B = 1 << TXEN0;
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    while (*message) {
        while (!(UCSR0A & (1 << UDRE0))) {}
        UDR0 = *message++;
    }
    for (;;) {}
#else
    fputs(message, stdout);
    return ok ? 0 : 1;
#endif
}
