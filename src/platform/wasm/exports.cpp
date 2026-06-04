// exports.cpp — wasm-side implementations of the mcl_* exports the host
// calls through WAMR. Wraps desktop_entry.cpp's mcl_desktop_* trampolines
// so the host doesn't need to know about the "desktop_" name (kept for
// source-shared compatibility with the static-link path).
//
// Input state (encoder/button) is normally pull-model: GUI_hardware.poll()
// asks the host via host_input_* so modal loops can receive events while
// wasm is already executing. mcl_input_set_* remains as a compatibility
// export for non-modal hosts.
//
// Single-threaded — see ABI.md.
#include "wasm_exports.h"
#include "host_imports.h"   // mcl_midi_port_t
#include "desktop_entry.h"

#include "oled.h"
#include "MidiUart.h"
#include "MidiSysex.h"
#include "GUI_hardware.h"
#include "MidiClock.h"
#include "MCL.h"
#include "MCLArrangement.h"
#include "MCLSeq.h"
#include "GridPages.h"
#include "GridIOPage.h"
#include "KeyInterface.h"
#include "NoteInterface.h"
#include "DeviceManager.h"
#include "MCLSysConfig.h"
#include "MCLMenus.h"
#include "MD.h"
#include "MDSysex.h"
#include "platform.h"
#ifdef PLATFORM_TBD
#include "GridIOOverlay.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// MCL globals provided by global.cpp / GUI_hardware.cpp.
extern Oled             oled_display;
extern MidiUartClass    MidiUart;
extern MidiUartClass    MidiUart2;
extern MidiUartUSBClass MidiUartUSB;
extern MidiUartClass    seq_tx1;
extern MidiUartClass    seq_tx2;
extern MidiUartClass    seq_tx3;
extern MidiUartClass    seq_tx4;
extern EncodersClass    Encoders;
extern ButtonsClass     Buttons;
extern uint64_t         mcl_desktop_button_mask;
extern MidiClockClass   MidiClock;
extern MCLSeq           mcl_seq;
extern MCL              mcl;
extern void handleIncomingMidi();

extern volatile uint16_t g_clock_ms;
extern volatile uint16_t g_clock_fast;
extern volatile uint16_t g_clock_ticks;
extern volatile uint16_t g_clock_minutes;

// ABI version. Bump major when removing/renaming/changing signatures.
static constexpr uint16_t MCL_ABI_MAJOR = 1;
static constexpr uint16_t MCL_ABI_MINOR = 8;

static uint32_t s_timer1_remainder_us = 0;
static uint32_t s_timer2_remainder_us = 0;
#ifdef MCL_WASM_GUI_TRACE
static uint32_t s_gui_trace_frame = 0;
#endif
static constexpr uint32_t kMaxHostMidiPumpBytesPerPort = 4096;
static constexpr uint8_t kMidiSysexStart = 0xF0;
static constexpr uint8_t kMidiSysexEnd = 0xF7;
static constexpr uint8_t kMaxSysexIngressTraceLines = 32;
static uint8_t s_sysex_ingress_trace_lines = 0;
static constexpr uint16_t kDebugMenuSnapshotMaxBytes = 255;
static char s_debug_menu_snapshot[kDebugMenuSnapshotMaxBytes + 1];
static uint16_t s_debug_menu_snapshot_len = 0;

static MidiUartClass* uart_for_port(int32_t port) {
    switch (port) {
    case MCL_MIDI_UART:  return &MidiUart;
    case MCL_MIDI_UART2: return &MidiUart2;
    case MCL_MIDI_USB:   return (MidiUartClass*)&MidiUartUSB;
    default:             return nullptr;
    }
}

static int32_t port_for_uart(MidiUartClass* uart) {
    if (uart == &MidiUart) return MCL_MIDI_UART;
    if (uart == &MidiUart2) return MCL_MIDI_UART2;
    if (uart == (MidiUartClass*)&MidiUartUSB) return MCL_MIDI_USB;
    return -1;
}

static void debug_menu_copy_snapshot(const char* text) {
    s_debug_menu_snapshot_len = 0;
    s_debug_menu_snapshot[0] = '\0';
    if (!text)
        return;
    while (*text && s_debug_menu_snapshot_len < kDebugMenuSnapshotMaxBytes) {
        s_debug_menu_snapshot[s_debug_menu_snapshot_len++] = *text++;
    }
    s_debug_menu_snapshot[s_debug_menu_snapshot_len] = '\0';
}

static uint16_t debug_menu_build_page_snapshot(PageIndex page,
                                               uint8_t selected_item) {
    const PageIndex previous_page = mcl.currentPage();

    if (page == MCL_CONFIG_PAGE)
        mcl_config_page.select_item(selected_item);
    else if (page == SYSTEM_PAGE)
        system_page.select_item(selected_item);

    mcl.setPage(page);
    oled_display.debugCaptureTextBegin();
    GUI.display();
    debug_menu_copy_snapshot(oled_display.debugCaptureTextEnd());

    if (previous_page < NUM_PAGES && previous_page != page)
        mcl.setPage(previous_page);

    return s_debug_menu_snapshot_len;
}

static uint16_t debug_menu_build_mcl_config_snapshot() {
    return debug_menu_build_page_snapshot(MCL_CONFIG_PAGE, 0);
}

static uint16_t debug_menu_build_system_snapshot() {
    return debug_menu_build_page_snapshot(SYSTEM_PAGE, 0);
}

static uint16_t debug_menu_build_system_no_devices_snapshot() {
    for (uint8_t port = UART1_PORT; port <= MIDI_PORT_COUNT; ++port)
        device_manager.detach_port(port);
    return debug_menu_build_page_snapshot(SYSTEM_PAGE, 5);
}

static uint32_t debug_menu_snapshot_chunk(uint16_t offset) {
    uint32_t out = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        const uint16_t pos = offset + i;
        const uint8_t c = pos < s_debug_menu_snapshot_len
            ? (uint8_t)s_debug_menu_snapshot[pos]
            : 0;
        out |= ((uint32_t)c) << (8u * i);
    }
    return out;
}

static void pump_host_midi_input_for_audio() {
    for (int32_t port = MCL_MIDI_UART; port <= MCL_MIDI_USB; ++port) {
        uint32_t count = 0;
        while (count < kMaxHostMidiPumpBytesPerPort) {
            int32_t byte_val = host_midi_in_pop(port);
            if (byte_val < 0)
                break;
            mcl_midi_in_push(port, (uint8_t)byte_val);
            ++count;
        }
    }
}

static uint32_t drain_uart_output_for_audio(MidiUartClass* uart, int32_t port) {
    if (!uart || port < 0)
        return 0;

    static bool in_sysex[3] = {};
    bool& port_in_sysex = in_sysex[port - MCL_MIDI_UART];
    uint32_t total = 0;
    while (total < kMaxHostMidiPumpBytesPerPort) {
        uint8_t byte_val = 0;
        size_t n = uart->desktop_egress(&byte_val, 1);
        if (!n)
            break;
        host_midi_out_push(port, byte_val);
        ++total;
        if (byte_val == kMidiSysexStart) {
            port_in_sysex = true;
        } else if (byte_val == kMidiSysexEnd && port_in_sysex) {
            port_in_sysex = false;
            break;
        }
    }
    return total;
}

static uint32_t pump_host_midi_output_for_audio() {
    uint32_t total = 0;
    for (int32_t port = MCL_MIDI_UART; port <= MCL_MIDI_USB; ++port) {
        total += drain_uart_output_for_audio(uart_for_port(port), port);
    }
    return total;
}

static uint32_t mount_and_drain_seq_buffer_for_audio(MidiUartClass* output,
                                                     int32_t port,
                                                     MidiUartClass& seq_uart) {
    if (!output || port < 0 || !seq_uart.txRb || seq_uart.txRb->isEmpty())
        return 0;

    uint32_t total = 0;
    if (output->txRb_sidechannel && output->txRb_sidechannel != seq_uart.txRb) {
        total += drain_uart_output_for_audio(output, port);
        if (output->txRb_sidechannel && output->txRb_sidechannel != seq_uart.txRb)
            return total;
    }

    output->txRb_sidechannel = seq_uart.txRb;
    total += drain_uart_output_for_audio(output, port);
    return total;
}

static uint32_t pump_sequencer_midi_output_for_audio() {
    uint32_t total = 0;

    MidiUartClass* primary = mcl_seq.primary_output;
    int32_t primary_port = port_for_uart(primary);
    total += mount_and_drain_seq_buffer_for_audio(primary, primary_port, seq_tx1);
    total += mount_and_drain_seq_buffer_for_audio(primary, primary_port, seq_tx2);

    MidiUartClass* secondary = mcl_seq.secondary_output;
    int32_t secondary_port = port_for_uart(secondary);
    if (secondary != primary || secondary_port != primary_port) {
        total += mount_and_drain_seq_buffer_for_audio(secondary, secondary_port, seq_tx3);
        total += mount_and_drain_seq_buffer_for_audio(secondary, secondary_port, seq_tx4);
    }

    return total;
}

static void run_sequencer_tick_for_audio() {
    MidiClock.inCallback = true;
    uint8_t midi_lock_tmp = MidiUartParent::handle_midi_lock;
    MidiUartParent::handle_midi_lock = 1;
    mcl_seq.seq();
    MidiUartParent::handle_midi_lock = midi_lock_tmp;
    MidiClock.inCallback = false;

    // Hardware UART drains the sequencer side-channel buffers concurrently.
    // Wasm has no UART ISR, so flush after each sequencer tick before the
    // next tick can rotate or clear those ping-pong buffers.
    pump_host_midi_output_for_audio();
    pump_sequencer_midi_output_for_audio();
}

static void trace_sysex_ingress_state(const char* phase,
                                      int32_t port,
                                      MidiUartClass* uart) {
#ifdef DEBUGMODE
    if (s_sysex_ingress_trace_lines >= kMaxSysexIngressTraceLines || uart == nullptr ||
        uart->midi == nullptr || uart->midi->midiSysex == nullptr) {
        return;
    }

    MidiSysexClass* sysex = uart->midi->midiSysex;
    const uint8_t rd = sysex->msg_rd;
    const uint8_t wr = sysex->msg_wr;
    const uint8_t state = sysex->ledger[rd].state;
    const uint16_t len = sysex->ledger[rd].recordLen;
    const uint8_t avail = sysex->avail() ? 1 : 0;
    uint8_t head[8] = {};
    if (len > 0) {
        SysexView view(sysex, rd);
        const uint8_t n = (len < sizeof(head)) ? (uint8_t)len : (uint8_t)sizeof(head);
        for (uint8_t i = 0; i < n; ++i)
            head[i] = view.getByte(i);
    }

    const uint8_t cb0_received =
        (MDSysexListener.onMessageCallbacks.size > 0 &&
         MDSysexListener.onMessageCallbacks.callbacks[0].obj != nullptr &&
         MDSysexListener.onMessageCallbacks.callbacks[0].obj->received) ? 1 : 0;
    const uintptr_t cb0_ptr =
        (MDSysexListener.onMessageCallbacks.size > 0)
            ? (uintptr_t)MDSysexListener.onMessageCallbacks.callbacks[0].obj
            : 0;

    char line[320];
    snprintf(line, sizeof(line),
             "[mcl-wasm-sysex] %s port=%ld live=%u rd=%u wr=%u avail=%u "
             "state=%u len=%u head=%02x %02x %02x %02x %02x %02x %02x %02x "
             "md_msg=%u md_rd=%u cb=%u cb0=%lx cb0_recv=%u scb=%u md_conn=%u\n",
             phase,
             (long)port,
             (unsigned)uart->live_state,
             (unsigned)rd,
             (unsigned)wr,
             (unsigned)avail,
             (unsigned)state,
             (unsigned)len,
             head[0], head[1], head[2], head[3],
             head[4], head[5], head[6], head[7],
             (unsigned)MDSysexListener.msgType,
             (unsigned)MDSysexListener.msg_rd,
             (unsigned)MDSysexListener.onMessageCallbacks.size,
             (unsigned long)cb0_ptr,
             (unsigned)cb0_received,
             (unsigned)MDSysexListener.onStatusResponseCallbacks.size,
             (unsigned)(MD.connected ? 1 : 0));
    host_log(line);
    ++s_sysex_ingress_trace_lines;
#else
    (void)phase;
    (void)port;
    (void)uart;
#endif
}

#ifdef MCL_WASM_GUI_TRACE
static void trace_gui_stage(uint32_t frame, const char* phase) {
    char line[96];
    snprintf(line, sizeof(line), "[mcl-gui-trace] frame=%lu %s\n",
             (unsigned long)frame, phase);
    host_log(line);
}
#endif

// ---- Lifecycle -----------------------------------------------------------

extern "C" void mcl_setup(void) {
    s_timer1_remainder_us = 0;
    s_timer2_remainder_us = 0;
    mcl_desktop_setup();
}

// Audio-thread entry. Catches up MCL's timer-ISR equivalents for the
// elapsed period, then dispatches the softirq-equivalent work
// (sequencer step firing + incoming MIDI handling). Replicates the
// rp2040/irqs.cpp:timer1_handler + timer2_handler + softirq1 + softirq2
// behaviour without an actual ISR.
extern "C" void mcl_tick_audio(uint32_t elapsed_us) {
#ifdef MCL_WASM_DISABLE_SOFTWARE_IRQ
    (void)elapsed_us;
    return;
#else
    if (!mcl_desktop_is_setup_done()) return;
    if (elapsed_us == 0) return;

    // timer2 work (5 kHz on hardware → one tick per 200 µs).
    constexpr uint32_t kTimer2PeriodUs = 200;
    uint64_t fast_total_us = (uint64_t)elapsed_us + s_timer2_remainder_us;
    uint32_t fast_ticks = (uint32_t)(fast_total_us / kTimer2PeriodUs);
    s_timer2_remainder_us = (uint32_t)(fast_total_us % kTimer2PeriodUs);
    while (fast_ticks--) {
        g_clock_fast++;

        // handleInternalTimerTick is PLATFORM_TBD-only on hardware (master
        // clock generation). Skipped here — desktop/wasm follow external
        // clock or run unsynchronised.

        MidiClock.div192th_countdown++;
        if (MidiClock.state == MidiClockClass::STARTED &&
            MidiClock.div192_time > 0 &&
            MidiClock.div192th_countdown >= MidiClock.div192_time &&
            MidiClock.interp_budget > 0) {
            MidiClock.increment192Counter();
            MidiClock.div192th_countdown = 0;
            MidiClock.interp_budget--;
            run_sequencer_tick_for_audio();
        }
    }

    // timer1 work (1 kHz on hardware → one tick per 1000 µs).
    constexpr uint32_t kTimer1PeriodUs = 1000;
    uint64_t ms_total_us = (uint64_t)elapsed_us + s_timer1_remainder_us;
    uint32_t ms_ticks = (uint32_t)(ms_total_us / kTimer1PeriodUs);
    s_timer1_remainder_us = (uint32_t)(ms_total_us % kTimer1PeriodUs);
    while (ms_ticks--) {
        g_clock_ms++;
        g_clock_ticks++;
        if (g_clock_ticks == 60000) {
            g_clock_ticks = 0;
            g_clock_minutes++;
        }
        MidiUart.tickActiveSense();
        MidiUart2.tickActiveSense();
        MidiUartUSB.tickActiveSense();
    }

    // softirq2 — consume host MIDI after advancing the logical timer clocks.
    // MCL derives external tempo from read_clock()/g_clock_fast when 0xF8 is
    // handled, so draining MIDI before this catch-up timestamps clock pulses
    // against stale time and makes the displayed BPM jump high.
    pump_host_midi_input_for_audio();
    handleIncomingMidi();
    pump_host_midi_output_for_audio();
#endif
}

// GUI-rate entry. The first GUI tick enters the Arduino setup() body; later
// ticks run loop(). MCL::loop() itself polls input on wasm so nested modal
// loops receive button/encoder events too.
extern "C" void mcl_tick_gui(void) {
#ifdef MCL_WASM_GUI_TRACE
    const uint32_t gui_frame = s_gui_trace_frame++;
    const bool trace_gui_tick = (gui_frame < 180u) || ((gui_frame % 60u) == 0u);
#endif
    if (!mcl_desktop_is_setup_done()) {
#ifdef MCL_WASM_GUI_TRACE
        if (trace_gui_tick) trace_gui_stage(gui_frame, "setup before desktop");
#endif
        mcl_desktop_tick();
#ifdef MCL_WASM_GUI_TRACE
        if (trace_gui_tick) trace_gui_stage(gui_frame, "setup after desktop");
#endif
#ifdef DEBUGMODE
#ifdef MCL_WASM_GUI_TRACE
        if (trace_gui_tick) trace_gui_stage(gui_frame, "setup before debug-flush");
#endif
        mcl_debug::flush();
#ifdef MCL_WASM_GUI_TRACE
        if (trace_gui_tick) trace_gui_stage(gui_frame, "setup after debug-flush");
#endif
#endif
        return;
    }
#ifdef MCL_WASM_GUI_TRACE
    if (trace_gui_tick) trace_gui_stage(gui_frame, "loop before desktop");
#endif
    mcl_desktop_tick();
#ifdef MCL_WASM_GUI_TRACE
    if (trace_gui_tick) trace_gui_stage(gui_frame, "loop after desktop");
#endif
#ifdef DEBUGMODE
#ifdef MCL_WASM_GUI_TRACE
    if (trace_gui_tick) trace_gui_stage(gui_frame, "loop before debug-flush");
#endif
    mcl_debug::flush();
#ifdef MCL_WASM_GUI_TRACE
    if (trace_gui_tick) trace_gui_stage(gui_frame, "loop after debug-flush");
#endif
#endif
}

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

extern "C" void mcl_set_transport_position(uint32_t tick96) {
    MidiClock.set_transport_position(tick96);
    mcl_seq.set_transport_position(tick96);
    mcl_arrangement.resetPlaybackForTransport();
}

// ---- Input -------------------------------------------------------------
//
// Compatibility push-model exports. The SPS host uses the host_input_*
// imports instead; these remain useful for small harnesses that only call
// mcl_tick_gui() after setting input.

extern "C" void mcl_input_set_button_mask(uint32_t mask) {
    mcl_desktop_button_mask = (uint64_t)mask;
}

extern "C" void mcl_input_set_button_mask64(uint32_t mask_lo, uint32_t mask_hi) {
    mcl_desktop_button_mask = ((uint64_t)mask_hi << 32) | (uint64_t)mask_lo;
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

extern "C" uint32_t mcl_debug_tempo_x100(void) {
    float bpm = MidiClock.get_tempo();
    if (bpm < 0.0f || bpm > 2000.0f)
        return 0;
    return (uint32_t)(bpm * 100.0f + 0.5f);
}

extern "C" uint32_t mcl_debug_state(void) {
    if (!mcl_desktop_is_setup_done())
        return 0;

    PageIndex page = mcl.currentPage();
    uint32_t state = (uint32_t)((page < NUM_PAGES) ? page : NULL_PAGE);
    state |= ((uint32_t)(grid_page.getRow() & 0xffu)) << 8;
    if (grid_page.cur_grid) state |= (1u << 16);
    if (grid_page.show_slot_menu) state |= (1u << 17);
    if (key_interface.state) state |= (1u << 18);
    if (note_interface.note_proceed) state |= (1u << 19);
    if (MD.connected) state |= (1u << 20);

    MidiDevice *primary = device_manager.primary_device();
    MidiDevice *secondary = device_manager.secondary_device();
    if (primary != nullptr && primary->connected) state |= (1u << 21);
    if (secondary != nullptr && secondary->connected) state |= (1u << 22);
    state |= (1u << 23);
    if (note_interface.notes_on) state |= (1u << 24);
    if (note_interface.notes_off) state |= (1u << 25);
    if (GridIOPage::track_select) state |= (1u << 26);
    if (GridIOPage::show_offset) state |= (1u << 27);
    if (GridIOPage::show_track_type) state |= (1u << 28);
#ifdef PLATFORM_TBD
    if (grid_io_overlay.is_active()) state |= (1u << 29);
    if (device_manager.is_ui_active()) state |= (1u << 30);
    if (device_manager.is_ui_collapsed()) state |= (1u << 31);
#endif
    return state;
}

extern "C" uint32_t mcl_debug_value(int32_t id) {
    if (!mcl_desktop_is_setup_done())
        return 0;

    switch (id) {
    case 1:
        return GridIOPage::track_select;
    case 2:
        return note_interface.notes_on;
    case 3:
        return note_interface.notes_off;
    case 4:
        return mcl_debug_state();
    case 5:
        return mcl_cfg.track_type_select;
    case 300:
        return MidiClock.div16th_counter;
    case 301:
        return MidiClock.div96th_counter;
    case 302:
        return MidiClock.div192th_counter;
    case 303:
        return (uint32_t)MidiClock.state |
               ((uint32_t)MidiClock.mod6_counter << 8) |
               ((uint32_t)MidiClock.mod12_counter << 16) |
               ((uint32_t)MidiClock.clock_interpolation << 24);
    case 200:
        return debug_menu_build_mcl_config_snapshot();
    case 265:
        return debug_menu_build_system_snapshot();
    case 266:
        return debug_menu_build_system_no_devices_snapshot();
    default:
        if (id >= 201 && id <= 264) {
            if (s_debug_menu_snapshot_len == 0)
                debug_menu_build_mcl_config_snapshot();
            return debug_menu_snapshot_chunk((uint16_t)(id - 201) * 4u);
        }
        return 0;
    }
}
