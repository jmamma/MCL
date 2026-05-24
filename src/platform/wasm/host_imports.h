// host_imports.h — declarations for the wasm host-import ABI.
//
// MCL compiled to wasm32 calls these to reach the host for things the desktop
// build does inline with std:: facilities. The host registers a native-symbol
// table that resolves each name. See ABI.md for full semantics + threading
// rules.
//
// Conventions:
//   - All names start with `host_`.
//   - C linkage (so wamrc's mangling matches the host NativeSymbol[] table).
//   - Pass-by-value primitives + opaque int handles, never C++ types.
//   - File descriptors are int32. Negative return = error.
//   - Strings are wasm-linear-memory pointers; host translates via
//     wasm_runtime_addr_app_to_native() and must not retain.
//
// Keep this header in sync with:
//   - src/host/modules/wasm_panel/PanelDeviceHostImports.cpp on the host side
//     (NativeSymbol[]).
//   - tools/build_mcl_wasm.sh (-Wl,--allow-undefined flag).
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Time ----------------------------------------------------------------
// Monotonic; resets only at host process restart. Wraps after ~49 days.
uint32_t host_millis(void);
uint32_t host_micros(void);

// Cooperative yield point for MCL's normal blocking UI loops. The host
// sleeps/yields a little so a modal page does not spin a CPU core.
void host_yield(void);

// ---- File I/O (back the SdFat shim) --------------------------------------
//
// File handles are positive int32. Negative = error. Mode bits match
// SdFat.h O_* (O_READ/O_WRITE/O_RDWR/O_CREAT/O_EXCL/O_APPEND/O_TRUNC).
// Paths are wasm-linear-memory cstring pointers. Path semantics: absolute
// paths starting with '/' resolve under the host-configured module storage
// root (normally <app-data>/modules/<module-id>/data); relative paths resolve
// under the host-tracked virtual cwd modified by host_fs_chdir.
int32_t host_fs_open  (const char* path, int32_t mode);
int32_t host_fs_close (int32_t fd);
int32_t host_fs_read  (int32_t fd, void* buf, int32_t len);
int32_t host_fs_write (int32_t fd, const void* buf, int32_t len);
int32_t host_fs_sync  (int32_t fd);
int32_t host_fs_truncate(int32_t fd, int32_t len);
int32_t host_fs_seek  (int32_t fd, int32_t pos);
int32_t host_fs_tell  (int32_t fd);
int32_t host_fs_size  (int32_t fd);
int32_t host_fs_exists(const char* path);
int32_t host_fs_mkdir (const char* path, int32_t recursive);
int32_t host_fs_rmdir (const char* path);
int32_t host_fs_remove(const char* path);
int32_t host_fs_rename(const char* old_path, const char* new_path);
int32_t host_fs_chdir (const char* path);
int32_t host_fs_is_dir(const char* path);

// Directory iteration. host_fs_dir_open returns a directory handle (>=0);
// host_fs_dir_next writes the next entry's filename into `name` (max
// `cap` bytes incl NUL) and returns 1 for "got entry", 0 for end-of-stream,
// -1 for error.
int32_t host_fs_dir_open (const char* path);
int32_t host_fs_dir_next (int32_t handle, char* name, int32_t cap);
int32_t host_fs_dir_close(int32_t handle);
int32_t host_fs_dir_rewind(int32_t handle);

// ---- MIDI ----------------------------------------------------------------
//
// `port` tags MCL's logical UARTs: 0 = MidiUart (DIN A), 1 = MidiUart2
// (DIN B), 2 = MidiUartUSB. Defined in mcl_midi_port_t in this header.
// The host's bridge moves bytes between these ports and juce::MidiBuffer
// around mcl_tick_audio().
//
// host_midi_in_pop returns -1 if empty, 0..255 otherwise.
// host_midi_out_push always accepts; ring overflow drops oldest.
enum mcl_midi_port_t {
    MCL_MIDI_UART  = 0,
    MCL_MIDI_UART2 = 1,
    MCL_MIDI_USB   = 2,
};
int32_t host_midi_in_pop  (int32_t port);
void    host_midi_out_push(int32_t port, uint8_t byte_val);

// ---- Input ---------------------------------------------------------------
//
// Pull-model: GUI_hardware.poll() calls these from inside MCL's own loop.
// That lets normal blocking modal pages receive input while wasm is already
// executing and the host cannot enter a setter export.
uint64_t host_input_button_mask(void);
int32_t  host_input_encoder_delta(int32_t encoder_id);
uint32_t host_input_encoder_button_mask(void);

// Retained for ABI compatibility. SPS returns 0 by default; timer/audio
// advancement is normally driven by the host audio side via mcl_tick_audio(),
// not from the GUI/service-thread poll path.
uint32_t host_audio_pending_us(void);

// ---- Display -------------------------------------------------------------
//
// The Oled framebuffer lives in wasm linear memory at a stable offset
// (mcl_framebuffer_offset() — see wasm_exports.h). host_display_dirty
// is called by MCL after each `display()` flush so the host knows when
// to repaint.
void host_display_dirty(void);

// ---- Debug ---------------------------------------------------------------
// Stderr surrogate. Plugin routes to juce::Logger or DBG.
void host_log(const char* text);

// ---- Math ----------------------------------------------------------------
//
// Forwarded from libc.c's sin/cos/etc. to the host's libm so we don't have
// to bundle approximations. All taking doubles for max precision; libc.c
// downcasts to float for sinf/cosf/sqrtf.
double host_math_sin (double x);
double host_math_cos (double x);
double host_math_tan (double x);
double host_math_sqrt(double x);
double host_math_pow (double x, double y);
double host_math_exp (double x);
double host_math_log (double x);

// ---- Memory --------------------------------------------------------------
//
// Wasm-side malloc/free are routed through these so MCL's heap (C++ new,
// std::string copies under PLATFORM_DESKTOP not WASM, etc.) can be backed
// by a per-instance arena that lives outside wasm linear memory. On the
// host side this should call wasm_runtime_module_malloc/_free against
// the active module_inst.
//
// IMPORTANT: the returned pointer is a *wasm-linear-memory* address from
// MCL's perspective (i.e., a uint32 offset into linear memory) — but the
// declaration here uses `void*` to match C ABI expectations. WAMR's
// runtime APIs handle the translation transparently when the import is
// registered with the matching signature.
void* host_malloc (size_t n);
void  host_free   (void* p);
void* host_realloc(void* p, size_t n);

#ifdef __cplusplus
}
#endif
