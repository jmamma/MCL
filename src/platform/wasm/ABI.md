# MCL-on-WAMR ABI

Contract between MCL compiled as `mcl.aot` and a host that loads it via WAMR.

## Threading model

MCL is **single-threaded inside the wasm instance**. The host must serialise
all entry points (`mcl_setup`, `mcl_tick_audio`, `mcl_tick_gui`,
`mcl_midi_*`, `mcl_input_*`) so the instance is never entered concurrently.

The host audio side drives `mcl_tick_audio()` from audio sample time, not
wall clock. The host GUI/service side drives `mcl_tick_gui()` from a
module-owned non-realtime thread. The host must never enter both exports
concurrently.

Host-import callbacks (`host_fs_*`, `host_midi_*`, `host_log`, etc.)
execute on whatever thread called the wasm export that invoked them.
Audio-thread exports must not enter import paths that can block, allocate
heavily, or touch the filesystem.

`host_yield()` and `host_input_*()` are intentionally GUI/service-thread
imports. MCL calls them from `MCL::loop()` / `GUI_hardware.poll()` on wasm
so normal blocking UI loops remain cooperative and can receive panel and
desktop pointer input without changing the hardware event path. Audio-thread
exports must not call them. The same hook pumps `host_midi_in_pop()` /
`host_midi_out_push()`, so modal pages do not strand host-side MIDI queues
while the service thread owns the wasm runtime.

`host_audio_pending_us()` lets the platform layer catch up a bounded amount of
sample-clock time while MCL is already inside a GUI/service-thread call. This
models the hardware timer IRQs that keep MIDI clock phase and blocking sysex
waits moving while foreground UI code spins. The catch-up call stays inside the
same wasm entry; the host must not re-enter WAMR concurrently.

## Lifetime

- One WAMR runtime per host process, refcounted across host instances.
- One `wasm_module_inst_t` per engine. MCL's globals (Midi, GUI, MD, …)
  live in that instance's linear memory and are naturally per-engine.
- Host calls `mcl_setup()` exactly once after instantiation. On wasm this is
  a fast prepare step; the Arduino `setup()` body runs from the first
  `mcl_tick_gui()` call, before normal `loop()` ticks.
- Teardown: host calls `wasm_runtime_deinstantiate` on engine shutdown.
  Destructors inside wasm run only if the .aot was built with whole-module
  finalisation enabled (it isn't for the first cut — engine shutdown is
  effectively `process exit` for wasm state).

## Memory ownership

- Strings passed to host-imports are pointers into wasm linear memory.
  The host **must** translate via `wasm_runtime_addr_app_to_native()` and
  must not retain the pointer past the call (wasm side may reuse the
  buffer on the next entry).
- Buffers passed to `host_fs_read` / `host_fs_write` are wasm-linear-memory
  pointers, same rule.
- `host_fs_sync` and `host_fs_truncate` back SdFat `sync()`,
  `truncate()`, and `preAllocate()`. MCL project/grid files depend on
  those calls having real SD-card semantics.
- The framebuffer (returned by `mcl_framebuffer_offset()`) is **stable**
  — it lives in wasm-side BSS, never moves. The host gets the offset
  once and may keep the native pointer for the runtime's lifetime.

## ABI version

`mcl_abi_version()` returns `(major << 16) | minor`. Host rejects load if
its compiled-in major doesn't match. Bump major on:

- Removing or renaming a host-import.
- Changing a host-import signature.
- Changing the `mcl_*` export surface.

Adding new host-imports / exports is a minor bump.

Minor 7 adds `host_input_pointer_pop(void* out, int32_t len)`. The host writes
a 12-byte desktop pointer event (`type/buttons/modifiers/reserved/x/y/dx/dy`)
into `out` and returns the number of bytes written, or `0` when no event is
queued.

Minor 8 adds `mcl_set_transport_position(uint32_t tick96)`, letting the host
apply absolute SPS tick96 transport positions after DAW seek/loop/start
discontinuities. It also adds diagnostic clock IDs `300..303` for headless
sync probes.

Minor 9 adds `host_headless_boot()`. Product composition now selects whether
MCL auto-loads persistent storage or exposes its normal startup/project UI, so
the same module artifact can serve embedded and interactive hosts.

Minor 10 adds `mcl_panel_leds_offset()` and `mcl_panel_led_count()`. The
returned stable array contains logical panel outputs as packed `0xRRGGBB`
values; hosts copy it after a GUI tick just as they copy the OLED framebuffer.

Minor 11 adds optional `mcl_panel_encoder_values()`. It packs the four active
page encoder values into four bytes in the 0..127 display domain; `0xff` marks
an unassigned slot. Host encoder deltas are logical steps and MCL applies the
active encoder's resolution before updating it.

## Naming convention

- Host → wasm (imports MCL calls): `host_*`. Registered with WAMR under
  module name `"env"` because the wasm objects use LLVM's default import
  module unless a symbol explicitly overrides it.
- Wasm → host (exports the host calls): `mcl_*`. Looked up by name on the
  `wasm_module_inst_t`.
- `mcl_debug_value(id)` is an optional read-only diagnostic export for SPS
  integration tests. Current IDs are: `1=GridIOPage::track_select`,
  `2=note_interface.notes_on`, `3=note_interface.notes_off`,
  `4=mcl_debug_state()`, `5=mcl_cfg.track_type_select`,
  `300=MidiClock.div16th_counter`, `301=MidiClock.div96th_counter`,
  `302=MidiClock.div192th_counter`, `303=packed clock state/phase`.

## Files in this directory

| File              | Owns                                                |
|-------------------|-----------------------------------------------------|
| `host_imports.h`  | Declarations of every `host_*` the wasm calls       |
| `wasm_exports.h`  | Declarations of every `mcl_*` the host calls        |
| `platform.cpp`    | `millis`/`micros`/`delay` impl                      |
| `SdFat.cpp`       | Wasm-side SdFat backend (routes to `host_fs_*`)     |
| `exports.cpp`     | Wasm-side impl of `mcl_*` exports (TODO)            |
| `libc/`           | Freestanding libc stubs + a tiny C++ wrapper layer  |
| `sps/module.json` | Host manifest: artifact path, exports, ports, controls     |

`tools/build_mcl_wasm.sh` copies `sps/module.json` and `mcl.aot` into the
runtime package layout:

```text
build_wasm/package/mcl/
  module.json
  mcl.aot
```

## Host side

The reference host side lives in `src/host/modules/wasm_panel/`.
Files there mirror the .h files in this directory:

| Host file                 | Implements                                       |
|---------------------------|--------------------------------------------------|
| `WamrAotRuntime.{h,cpp}`  | WAMR init, instantiation, lifetime               |
| `PanelDeviceHostImports.{h,cpp}` | `NativeSymbol[]` + every `host_*` impl    |
| `WasmPanelModule.{h,cpp}` | Module-system entry                              |
| `LcdModuleDisplayElement` | LCD element that renders the module framebuffer  |

`PanelDeviceHostImports.cpp` and `host_imports.h` must stay in sync. If a
function is added in one, it must be added in the other, and the
`NativeSymbol[]` array updated.
