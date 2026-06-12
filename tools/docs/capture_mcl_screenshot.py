#!/usr/bin/env python3
"""Capture an MCL screen through the SPS headless test runner."""

from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
WORKSPACE_ROOT = REPO_ROOT.parent
DEFAULT_ASSETS_DIR = REPO_ROOT / "docs" / "manual" / "assets" / "images"
DEFAULT_BINARY = (
    WORKSPACE_ROOT
    / "plugin"
    / "build"
    / "SPS_MCL_HeadlessTest_artefacts"
    / "Release"
    / "SPS_MCL_HeadlessTest"
)
DEFAULT_MODULE_DIR = REPO_ROOT / "build_wasm" / "package" / "mcl"
DEFAULT_FIRMWARE = (
    Path.home()
    / "Library"
    / "Application Support"
    / "SPS-X"
    / "firmware"
    / "firmware.syx"
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture a current MCL framebuffer screenshot as PNG."
    )
    parser.add_argument(
        "output",
        help="Output PNG path. Relative paths are written under docs/manual/assets/images.",
    )
    parser.add_argument(
        "--buttons",
        default="",
        help=(
            "Comma-separated input actions before capture. Use + for button chords, "
            "+button/-button for holds, enc2:19 for raw encoder ticks, and "
            "+key:no/-key:no for MD key-interface holds. Use wait:250 for timing."
        ),
    )
    parser.add_argument(
        "--hold-buttons",
        default="",
        help="Comma-separated MCL button names or chords to hold during capture.",
    )
    parser.add_argument(
        "--startup-hold-buttons",
        default="",
        help="Comma-separated MCL button names or chords to hold from module startup.",
    )
    parser.add_argument("--scale", type=int, default=1, help="Framebuffer scale factor.")
    parser.add_argument(
        "--frame-height",
        type=int,
        choices=(32, 64),
        default=32,
        help="Logical LCD rows to save. Use 64 for TBD/full-height captures.",
    )
    parser.add_argument(
        "--full-height",
        action="store_true",
        help="Save the full 64-row LCD frame instead of the default 32 rows.",
    )
    parser.add_argument("--settle-ms", type=int, default=1200)
    parser.add_argument("--pre-input-settle-ms", type=int, default=1200)
    parser.add_argument("--button-hold-ms", type=int, default=80)
    parser.add_argument("--button-settle-ms", type=int, default=250)
    parser.add_argument("--encoder-settle-ms", type=int, default=250)
    parser.add_argument("--timeout-ms", type=int, default=25000)
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    parser.add_argument("--module-dir", type=Path, default=DEFAULT_MODULE_DIR)
    parser.add_argument("--firmware", type=Path, default=DEFAULT_FIRMWARE)
    parser.add_argument(
        "--data-dir",
        type=Path,
        help="Use this SPS data directory instead of creating a temporary one.",
    )
    parser.add_argument(
        "--keep-data-dir",
        action="store_true",
        help="Keep the generated temporary SPS data directory for debugging.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print SPS/MCL runner output on successful captures.",
    )
    args = parser.parse_args()
    if args.full_height:
        args.frame_height = 64
    return args


def resolve_output(path: str) -> Path:
    output = Path(path)
    if output.suffix == "":
        output = output.with_suffix(".png")
    if not output.is_absolute():
        output = DEFAULT_ASSETS_DIR / output
    return output


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise SystemExit(f"{label} not found: {path}")


def button_id_for_name(token: str) -> int:
    name = token.strip().lower().replace("-", "_").replace(" ", "_")
    if not name:
        raise SystemExit("Empty button name")
    if name.isdigit():
        value = int(name)
        if 0 <= value < 64:
            return value
        raise SystemExit(f"Button id out of range: {token}")

    for prefix in ("trig_", "trig"):
        if name.startswith(prefix):
            try:
                value = int(name[len(prefix):])
            except ValueError as exc:
                raise SystemExit(f"Invalid trig button name: {token}") from exc
            if 1 <= value <= 16:
                return 16 + value
            raise SystemExit(f"Trig button out of range: {token}")

    names = {
        "button1": 4,
        "escape": 4,
        "esc": 4,
        "no": 4,
        "button2": 5,
        "page": 5,
        "bank_group": 5,
        "bankgroup": 5,
        "button3": 6,
        "patsong": 6,
        "pat_song": 6,
        "global": 6,
        "button4": 7,
        "enter": 7,
        "yes": 7,
        "record": 8,
        "rec": 8,
        "play": 9,
        "stop": 10,
        "func": 12,
        "function": 12,
        "up": 13,
        "left": 14,
        "down": 15,
        "right": 16,
        "scale": 36,
    }
    try:
        return names[name]
    except KeyError as exc:
        raise SystemExit(f"Unknown button name: {token}") from exc


def startup_button_mask(spec: str) -> int:
    mask = 0
    for token in spec.split(","):
        token = token.strip()
        if not token:
            continue
        for part in token.split("+"):
            part = part.strip()
            if part:
                mask |= 1 << button_id_for_name(part)
    return mask


def prepare_data_dir(data_dir: Path, module_dir: Path, firmware: Path) -> None:
    require_file(module_dir / "module.json", "MCL module.json")
    require_file(module_dir / "mcl.aot", "MCL AOT")
    require_file(firmware, "SPS firmware")

    target_module = data_dir / "modules" / "mcl"
    target_firmware = data_dir / "firmware"
    target_module.mkdir(parents=True, exist_ok=True)
    target_firmware.mkdir(parents=True, exist_ok=True)

    shutil.copy2(module_dir / "module.json", target_module / "module.json")
    shutil.copy2(module_dir / "mcl.aot", target_module / "mcl.aot")
    shutil.copy2(firmware, target_firmware / "firmware.syx")


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    )


def bmp_to_png(bmp_path: Path, png_path: Path, crop_height: int | None = None) -> None:
    data = bmp_path.read_bytes()
    if len(data) < 54 or data[:2] != b"BM":
        raise SystemExit(f"Not a BMP file: {bmp_path}")

    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40:
        raise SystemExit("Unsupported BMP DIB header")

    width = struct.unpack_from("<i", data, 18)[0]
    signed_height = struct.unpack_from("<i", data, 22)[0]
    planes = struct.unpack_from("<H", data, 26)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    if width <= 0 or signed_height == 0 or planes != 1 or bpp != 24 or compression != 0:
        raise SystemExit("Only uncompressed 24-bit BMP captures are supported")

    top_down = signed_height < 0
    height = abs(signed_height)
    row_bytes = ((width * 3 + 3) // 4) * 4
    expected_size = pixel_offset + row_bytes * height
    if expected_size > len(data):
        raise SystemExit("BMP pixel data is truncated")

    output_height = height
    if crop_height is not None and crop_height > 0:
        output_height = min(output_height, crop_height)

    rows: list[bytes] = []
    for y in range(output_height):
        source_y = y if top_down else height - 1 - y
        start = pixel_offset + source_y * row_bytes
        row = bytearray()
        for x in range(width):
            b, g, r = data[start + x * 3 : start + x * 3 + 3]
            row.extend((r, g, b))
        rows.append(b"\x00" + bytes(row))

    raw = b"".join(rows)
    png = (
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(
            b"IHDR",
            struct.pack(">IIBBBBB", width, output_height, 8, 2, 0, 0, 0),
        )
        + png_chunk(b"IDAT", zlib.compress(raw, 9))
        + png_chunk(b"IEND", b"")
    )
    png_path.parent.mkdir(parents=True, exist_ok=True)
    png_path.write_bytes(png)


def run_capture(args: argparse.Namespace, data_dir: Path, bmp_path: Path) -> None:
    require_file(args.binary, "SPS MCL headless binary")
    env = os.environ.copy()
    env.update(
        {
            "SPS_DATA_DIR": str(data_dir),
            "SPS_MCL_UI_MACRO": "screenshot",
            "SPS_MCL_SCREENSHOT_PATH": str(bmp_path),
            "SPS_MCL_SCREENSHOT_SCALE": str(max(1, min(args.scale, 16))),
            "SPS_MCL_SCREENSHOT_SETTLE_MS": str(args.settle_ms),
            "SPS_MCL_SCREENSHOT_PRE_INPUT_SETTLE_MS": str(args.pre_input_settle_ms),
            "SPS_MCL_SCREENSHOT_BUTTON_HOLD_MS": str(args.button_hold_ms),
            "SPS_MCL_SCREENSHOT_BUTTON_SETTLE_MS": str(args.button_settle_ms),
            "SPS_MCL_SCREENSHOT_ENCODER_SETTLE_MS": str(args.encoder_settle_ms),
            "SPS_MCL_SCREENSHOT_TIMEOUT_MS": str(args.timeout_ms),
            "SPS_MCL_HEADLESS_TIMEOUT_MS": str(args.timeout_ms),
        }
    )
    if args.buttons:
        env["SPS_MCL_SCREENSHOT_BUTTONS"] = args.buttons
    if args.hold_buttons:
        env["SPS_MCL_SCREENSHOT_HOLD_BUTTONS"] = args.hold_buttons
    if args.startup_hold_buttons:
        env["SPS_MCL_STARTUP_BUTTON_MASK"] = str(
            startup_button_mask(args.startup_hold_buttons)
        )

    result = subprocess.run(
        [str(args.binary)],
        cwd=REPO_ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        sys.stdout.write(result.stdout)
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)
    if args.verbose and result.stdout:
        sys.stdout.write(result.stdout)
    if args.verbose and result.stderr:
        sys.stderr.write(result.stderr)


def main() -> int:
    args = parse_args()
    output = resolve_output(args.output)
    scale = max(1, min(args.scale, 16))
    crop_height = args.frame_height * scale

    if args.data_dir:
        data_dir = args.data_dir
        data_dir.mkdir(parents=True, exist_ok=True)
        prepare_data_dir(data_dir, args.module_dir, args.firmware)
        bmp_path = data_dir / "capture.bmp"
        run_capture(args, data_dir, bmp_path)
        bmp_to_png(bmp_path, output, crop_height)
    elif args.keep_data_dir:
        data_dir = Path(tempfile.mkdtemp(prefix="sps-mcl-doc-capture."))
        prepare_data_dir(data_dir, args.module_dir, args.firmware)
        bmp_path = data_dir / "capture.bmp"
        run_capture(args, data_dir, bmp_path)
        bmp_to_png(bmp_path, output, crop_height)
        print(f"kept SPS data dir: {data_dir}")
    else:
        with tempfile.TemporaryDirectory(prefix="sps-mcl-doc-capture.") as temp:
            data_dir = Path(temp)
            prepare_data_dir(data_dir, args.module_dir, args.firmware)
            bmp_path = data_dir / "capture.bmp"
            run_capture(args, data_dir, bmp_path)
            bmp_to_png(bmp_path, output, crop_height)

    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
