# MCL-on-WAMR ABI

Contract between MCL compiled as `mcl.aot` and the JUCE host (SPS plugin)
that loads it via WAMR.

## Threading model

MCL is **single-threaded inside the wasm instance**. The host must serialise
all entry points (`mcl_setup`, `mcl_tick`, `mcl_midi_*`, `mcl_input_*`) onto
a single thread — in practice the JUCE message thread, driven by a
`juce::Timer` at ~60 Hz.

The host's audio thread **never** calls into wasm. MIDI bytes received on
the audio thread must be queued in a lock-free SPSC ring on the host side
and drained from the message thread before the next `mcl_tick`.

Host-import callbacks (`host_fs_*`, `host_midi_*`, `host_log`, etc.)
execute on whatever thread called the wasm export that invoked them — by
the above, that's always the message thread. Implementations therefore
don't need internal locking.

## Lifetime

- One WAMR runtime per host process, refcounted across plugin instances
  (mirrors `usr_runtime.cpp`'s pattern).
- One `wasm_module_inst_t` per engine. MCL's globals (Midi, GUI, MD, …)
  live in that instance's linear memory and are naturally per-engine.
- Host calls `mcl_setup()` exactly once after instantiation; subsequent
  calls are no-ops.
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

## Naming convention

- Host → wasm (imports MCL calls): `host_*`. Registered with WAMR under
  module name `"mcl"`.
- Wasm → host (exports the host calls): `mcl_*`. Looked up by name on the
  `wasm_module_inst_t`.

## Files in this directory

| File              | Owns                                                |
|-------------------|-----------------------------------------------------|
| `host_imports.h`  | Declarations of every `host_*` the wasm calls       |
| `wasm_exports.h`  | Declarations of every `mcl_*` the host calls        |
| `platform.cpp`    | `millis`/`micros`/`delay` impl                      |
| `SdFat.cpp`       | Wasm-side SdFat backend (routes to `host_fs_*`)     |
| `exports.cpp`     | Wasm-side impl of `mcl_*` exports (TODO)            |
| `libc/`           | Freestanding libc stubs + a tiny C++ wrapper layer  |

## Plugin side

The plugin side lives in `src/host/Mcl/` (in the SPS dsp56k_emu repo).
Files there mirror the .h files in this directory:

| Plugin file               | Implements                                       |
|---------------------------|--------------------------------------------------|
| `McuRuntime.{h,cpp}`      | WAMR init, instantiation, lifetime               |
| `McuHostImports.{h,cpp}`  | `NativeSymbol[]` + every `host_*` impl           |
| `McuModule.{h,cpp}`       | Module-system entry (parallel to MdModule)       |
| `McuPage.{h,cpp}`         | LCD page that renders the framebuffer            |

`McuHostImports.cpp` and `host_imports.h` must stay in sync. If a function
is added in one, it must be added in the other, and the `NativeSymbol[]`
array updated.
