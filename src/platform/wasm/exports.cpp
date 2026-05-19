// exports.cpp — wasm-side implementations of the mcl_* exports the host
// (SPS plugin via WAMR) calls. Wraps desktop_entry.cpp's mcl_desktop_*
// trampolines so the host doesn't need to know about the "desktop_" name
// (kept for source-shared compatibility with the static-link path).
//
// Input state (encoder/button) is push-model: host invokes
// mcl_input_set_*; this file writes directly into MCL's Encoders/Buttons
// globals. MCL reads from those without a wasm→host crossing.
//
// Single-threaded — see ABI.md.
#include "wasm_exports.h"
#include "host_imports.h"   // mcl_midi_port_t
#include "desktop_entry.h"

#include "oled.h"
#include "MidiUart.h"
#include "GUI_hardware.h"

#include <stdint.h>
#include <string.h>

// MCL globals provided by global.cpp / GUI_hardware.cpp.
extern Oled             oled_display;
extern MidiUartClass    MidiUart;
extern MidiUartClass    MidiUart2;
extern MidiUartUSBClass MidiUartUSB;
extern EncodersClass    Encoders;
extern ButtonsClass     Buttons;

// ABI version. Bump major when removing/renaming/changing signatures.
static constexpr uint16_t MCL_ABI_MAJOR = 1;
static constexpr uint16_t MCL_ABI_MINOR = 0;

// ---- Lifecycle -----------------------------------------------------------

extern "C" void mcl_setup(void) { mcl_desktop_setup(); }
extern "C" void mcl_tick (void) { mcl_desktop_tick();  }

// ---- Framebuffer ---------------------------------------------------------
//
// The Oled placeholder in oled.h holds an internal 128×64×1bpp buffer.
// On wasm we expose its offset; the host's wasm_runtime_addr_app_to_native()
// turns that into a real pointer it can read pixels from.

extern "C" uint32_t mcl_framebuffer_offset(void) {
    return (uint32_t)(uintptr_t)oled_display.getBuffer();
}
extern "C" uint32_t mcl_framebuffer_stride(void) { return OLED_WIDTH / 8; }
extern "C" uint32_t mcl_framebuffer_width (void) { return OLED_WIDTH;     }
extern "C" uint32_t mcl_framebuffer_height(void) { return OLED_HEIGHT;    }

// ---- MIDI bridge ---------------------------------------------------------

static MidiUartClass* uart_for_port(int32_t port) {
    switch (port) {
    case MCL_MIDI_UART:  return &MidiUart;
    case MCL_MIDI_UART2: return &MidiUart2;
    case MCL_MIDI_USB:   return (MidiUartClass*)&MidiUartUSB;
    default:             return nullptr;
    }
}

extern "C" int32_t mcl_midi_in_push(int32_t port, uint8_t byte_val) {
    auto* u = uart_for_port(port);
    if (!u) return 0;
    u->desktop_ingress(&byte_val, 1);
    return 1;
}

extern "C" int32_t mcl_midi_out_pop(int32_t port) {
    auto* u = uart_for_port(port);
    if (!u) return -1;
    uint8_t b;
    size_t n = u->desktop_egress(&b, 1);
    return n ? (int32_t)b : -1;
}

// ---- Input -------------------------------------------------------------
//
// Push-model: host writes; MCL reads via its normal accessors. Encoder
// `normal` is the rotational delta and is consumed by MCL's poll/event
// loop — we *add* deltas so multiple host pushes within a tick coalesce.
//
// Button mask: low bit per button index. Maps directly into
// ButtonsClass::buttons[i].status's B_BIT_CURRENT (active-low: bit set
// means released). So we invert: host's "pressed" bit → status bit
// cleared.

extern "C" void mcl_input_set_button_mask(uint32_t mask) {
    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; ++i) {
        bool pressed = (mask >> i) & 1u;
        if (pressed) {
            Buttons.buttons[i].status &= (uint8_t)~(1u << B_BIT_CURRENT);
        } else {
            Buttons.buttons[i].status |= (uint8_t)(1u << B_BIT_CURRENT);
        }
    }
}

extern "C" void mcl_input_add_encoder_delta(int32_t idx, int8_t delta) {
    if (idx < 0 || idx >= GUI_NUM_ENCODERS) return;
    int new_val = (int)Encoders.encoders[idx].normal + (int)delta;
    if (new_val > 127)  new_val = 127;
    if (new_val < -128) new_val = -128;
    Encoders.encoders[idx].normal = (int8_t)new_val;
}

extern "C" void mcl_input_set_encoder_button(int32_t idx, uint8_t pressed) {
    if (idx < 0 || idx >= GUI_NUM_ENCODERS) return;
    Encoders.encoders[idx].button = pressed ? 1 : 0;
}

// ---- Version stamp -------------------------------------------------------

extern "C" uint32_t mcl_abi_version(void) {
    return ((uint32_t)MCL_ABI_MAJOR << 16) | MCL_ABI_MINOR;
}
