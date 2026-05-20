# MCL-on-WAMR ABI

Contract between MCL compiled as `mcl.aot` and the JUCE host (SPS plugin)
that loads it via WAMR.

## Threading model

MCL is **single-threaded inside the wasm instance**. The host must serialise
all entry points (`mcl_setup`, `mcl_tick_audio`, `mcl_tick_gui`,
`mcl_midi_*`, `mcl_input_*`) so the instance is never entered concurrently.

The host audio thread drives `mcl_tick_audio()` from audio sample time, not
wall clock. That entry point owns the virtual 1 kHz / 5 kHz timer catch-up,
sequencer work, and MIDI byte dispatch needed by the audio callback.

Host-import callbacks (`host_fs_*`, `host_midi_*`, `host_log`, etc.)
execute on whatever thread called the wasm export that invoked them.
Audio-thread exports must not enter import paths that can block, allocate
heavily, or touch the filesystem.

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
  module name `"env"` because the wasm objects use LLVM's default import
  module unless a symbol explicitly overrides it.
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
| `sps/module.json` | SPS host manifest: artifact path, exports, ports, controls |

`tools/build_mcl_wasm.sh` copies `sps/module.json` and `mcl.aot` into the
runtime package layout expected by SPS:

```text
build_wasm/package/mcl/
  module.json
  mcl.aot
```

## Plugin side

The plugin side lives in `src/host/modules/mcl/` (in the SPS dsp56k_emu repo).
Files there mirror the .h files in this directory:

| Plugin file               | Implements                                       |
|---------------------------|--------------------------------------------------|
| `MCLRuntime.{h,cpp}`      | WAMR init, instantiation, lifetime               |
| `MCLHostImports.{h,cpp}`  | `NativeSymbol[]` + every `host_*` impl           |
| `MCLModule.{h,cpp}`       | Module-system entry (parallel to MdModule)       |
| `LcdModuleDisplayElement` | LCD element that renders the module framebuffer  |

`MCLHostImports.cpp` and `host_imports.h` must stay in sync. If a function
is added in one, it must be added in the other, and the `NativeSymbol[]`
array updated.
