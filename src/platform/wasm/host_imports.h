// host_imports.h — declarations for the wasm host-import ABI.
//
// MCL compiled to wasm32 calls these to reach the JUCE host for things the
// desktop build does inline with std:: facilities (time, file I/O, midi
// byte-streams, framebuffer/encoder access). The plugin side (loading the
// .aot via WAMR) registers a native-symbol table that resolves each name.
//
// Convention:
//   - All names start with `host_`.
//   - C linkage (so wamrc's mangling matches the plugin-side
//     NativeSymbol[] table).
//   - Pass-by-value primitives + opaque int handles, never C++ types.
//   - File descriptors are int32. Negative return = error.
//   - Strings are passed as wasm-linear-memory `(const char*, int len)`
//     when the length isn't implicit; null-terminated otherwise.
//
// Keep this header in sync with the plugin-side NativeSymbol table.
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Time ----------------------------------------------------------------
uint32_t host_millis(void);
uint32_t host_micros(void);

// ---- File I/O (back the SdFat shim) --------------------------------------
//
// File handles are positive int32. Mode bits match SdFat.h O_* constants
// (O_READ/O_WRITE/O_RDWR/O_CREAT/O_EXCL/O_APPEND/O_TRUNC).
int32_t host_fs_open  (const char* path, int32_t mode);
int32_t host_fs_close (int32_t fd);
int32_t host_fs_read  (int32_t fd, void* buf, int32_t len);
int32_t host_fs_write (int32_t fd, const void* buf, int32_t len);
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

// Directory iteration. host_fs_dir_open returns a directory handle; subsequent
// host_fs_dir_next writes the next entry's filename into `name` (max `cap`
// bytes incl NUL) and returns 1 for "got an entry" or 0 for end-of-stream.
int32_t host_fs_dir_open (const char* path);
int32_t host_fs_dir_next (int32_t handle, char* name, int32_t cap);
int32_t host_fs_dir_close(int32_t handle);
int32_t host_fs_dir_rewind(int32_t handle);

// ---- MIDI ----------------------------------------------------------------
//
// MCL has multiple ports (UART/UART2/USB/...). The port index is just a tag
// the plugin side knows how to map onto its juce::MidiBuffer.
//
// host_midi_in_pop drains one byte (returns -1 if empty, 0..255 otherwise).
// host_midi_out_push pushes one byte for the host to send.
int32_t host_midi_in_pop  (int32_t port);
void    host_midi_out_push(int32_t port, uint8_t byte);

// ---- Input ---------------------------------------------------------------
//
// Encoder accumulated delta since last call (consumes & resets), and button
// pressed-state snapshot (1 bit per slot, packed into a uint32 covering up
// to 32 buttons — MCL's desktop GUI_hardware is sized for 8).
int8_t   host_encoder_delta (int32_t idx);
uint8_t  host_encoder_button(int32_t idx);
uint32_t host_button_mask   (void);

// ---- Display -------------------------------------------------------------
//
// The Oled framebuffer lives in wasm linear memory. The plugin calls back
// into wasm via the AOT export `mcl_framebuffer_offset` to learn where it
// sits, then reads pixels directly. host_display_dirty is called by MCL's
// oled.cpp after a `display()` flush to tell the host the bytes changed.
void host_display_dirty(void);

// ---- Debug ---------------------------------------------------------------
//
// Stderr surrogate. The plugin routes this into juce::Logger or similar.
void host_log(const char* text);

#ifdef __cplusplus
}
#endif
