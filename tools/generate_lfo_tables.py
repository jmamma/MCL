#!/usr/bin/env python3
"""Generate and preview the firmware LFO tables.

Only sine and exponential curves are stored in PROGMEM. The other shape IDs are
rendered by the firmware runtime from the 15-bit SPS-style phase accumulator.
"""

import argparse
import math
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

WAV_LENGTH = 128
PHASE_LENGTH = 0x8000
PHASE_MASK = PHASE_LENGTH - 1

SHAPES = (
    "TRI_WAV",
    "SAW_WAV",
    "SQU_WAV",
    "LIN_WAV",
    "EXP_WAV",
    "RND_WAV",
    "REV_LIN_WAV",
    "REV_EXP_WAV",
    "SIN_WAV",
    "STEP_WAV",
    "LINLIN_WAV",
)

TABLE_SHAPES = ("SIN_WAV", "EXP_WAV")

CENTERED_SHAPES = {"SIN_WAV", "TRI_WAV", "SQU_WAV", "SAW_WAV", "RND_WAV"}


def clamp_u8(value):
    return max(0, min(255, int(value)))


def phase_for_sample(sample):
    return (sample * PHASE_LENGTH // WAV_LENGTH) & PHASE_MASK


def phase_index(phase):
    return (phase & PHASE_MASK) >> 8


def sin_table():
    return [
        clamp_u8(round(127.5 + 127.5 * math.sin(2 * math.pi * n / WAV_LENGTH)))
        for n in range(WAV_LENGTH)
    ]


def exp_table():
    return [clamp_u8(round(128 * math.exp(-n / 40.0))) for n in range(WAV_LENGTH)]


def generate_tables():
    return [sin_table(), exp_table()]


def table_value(tables, table, phase):
    return tables[table][phase_index(phase)]


def signed_tri(phase):
    p = phase & PHASE_MASK
    if p < 0x2000:
        return p >> 6
    if p < 0x6000:
        return 256 - (p >> 6)
    return (p >> 6) - 512


def signed_saw(phase):
    return 128 - ((phase & PHASE_MASK) >> 7)


def unipolar_decay(phase):
    return 128 - phase_index(phase)


def unipolar_rise(phase):
    return phase_index(phase)


def unipolar_linlin(phase):
    idx = phase_index(phase)
    raw = idx * 2 if idx < 64 else (128 - idx) * 2
    return raw


def seed_to_random_value(seed):
    return ((seed & 0x3FFF) >> 7) - 64


def preview_random(phase):
    seed0 = 0x1234
    seed1 = 0x5678
    next_seed = seed0
    coarse = (phase & PHASE_MASK) >> 12
    for _ in range(coarse + 1):
        next_seed = (seed0 + seed1) & 0xFFFF
        seed1 = seed0
        seed0 = next_seed
    return seed_to_random_value(next_seed)


def preview_sample(tables, shape, phase):
    name = SHAPES[shape]
    if name == "TRI_WAV":
        return signed_tri(phase)
    if name == "SAW_WAV":
        return signed_saw(phase)
    if name == "SQU_WAV":
        return -128 if phase & 0x4000 else 128
    if name == "LIN_WAV":
        return unipolar_decay(phase)
    if name == "EXP_WAV":
        return table_value(tables, 1, phase)
    if name == "RND_WAV":
        return preview_random(phase)
    if name == "REV_LIN_WAV":
        return unipolar_rise(phase)
    if name == "REV_EXP_WAV":
        return 128 - table_value(tables, 1, phase)
    if name == "SIN_WAV":
        return table_value(tables, 0, phase) - 128
    if name == "STEP_WAV":
        return 128 if phase & 0x4000 else 0
    if name == "LINLIN_WAV":
        return unipolar_linlin(phase)
    raise ValueError(name)


def preview_value(tables, shape, phase):
    sample = preview_sample(tables, shape, phase)
    if SHAPES[shape] in CENTERED_SHAPES:
        return max(0, min(128, (sample + 128) >> 1))
    return max(0, min(128, sample))


def format_table(tables, initializer_only):
    lines = []
    if not initializer_only:
        lines.append(
            "const uint8_t LFOSeqTrack::wav_tables[LFO_TABLE_COUNT][WAV_LENGTH] "
            "PROGMEM = {"
        )
    else:
        lines.append("{")

    for row in tables:
        lines.append("  {")
        for start in range(0, WAV_LENGTH, 16):
            chunk = row[start : start + 16]
            suffix = "," if start + 16 < WAV_LENGTH else ""
            lines.append("    " + ", ".join(f"{value:3d}" for value in chunk) + suffix)
        lines.append("  },")

    lines.append("};" if not initializer_only else "}")
    return "\n".join(lines)


def shape_filter(shape):
    if shape is None:
        return range(len(SHAPES))
    if shape.isdigit():
        idx = int(shape)
        if idx < 0 or idx >= len(SHAPES):
            raise SystemExit(f"shape index must be 0..{len(SHAPES) - 1}")
        return (idx,)
    try:
        return (SHAPES.index(shape),)
    except ValueError:
        raise SystemExit(f"unknown shape {shape!r}; expected one of {', '.join(SHAPES)}")


def resample(values, width):
    if width == len(values):
        return values
    return [values[(idx * len(values)) // width] for idx in range(width)]


def render_ascii(tables, width, height, shape):
    selected = shape_filter(shape)
    lines = []
    for idx in selected:
        name = SHAPES[idx]
        values = [
            preview_value(tables, idx, phase_for_sample(n)) for n in range(WAV_LENGTH)
        ]
        values = resample(values, width)
        grid = [[" " for _ in range(width)] for _ in range(height)]

        center_row = round((128 - 64) * (height - 1) / 128)
        if 0 <= center_row < height:
            for x in range(width):
                grid[center_row][x] = "-"

        for x, value in enumerate(values):
            row = round((128 - value) * (height - 1) / 128)
            row = max(0, min(height - 1, row))
            grid[row][x] = "*"

        mode = "centered" if name in CENTERED_SHAPES else "offset-max"
        storage = "table" if name in TABLE_SHAPES else "runtime"
        lines.append(f"{idx:02d} {name} ({mode}, {storage})")
        for row_idx, row in enumerate(grid):
            label = (
                "128"
                if row_idx == 0
                else "064"
                if row_idx == center_row
                else "000"
                if row_idx == height - 1
                else "   "
            )
            lines.append(f"{label} |{''.join(row)}|")
        lines.append("")
    return "\n".join(lines).rstrip()


def write_gnuplot_png(tables, path, shape):
    if shutil.which("gnuplot") is None:
        raise SystemExit("gnuplot not found in PATH")

    selected = tuple(shape_filter(shape))
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmpdir:
        data_path = Path(tmpdir) / "lfo_tables.dat"
        script_path = Path(tmpdir) / "plot.gp"

        with data_path.open("w") as data:
            data.write("# sample " + " ".join(SHAPES[idx] for idx in selected) + "\n")
            for n in range(WAV_LENGTH):
                data.write(str(n))
                phase = phase_for_sample(n)
                for idx in selected:
                    data.write(f" {preview_value(tables, idx, phase)}")
                data.write("\n")

        plot_terms = []
        for col, idx in enumerate(selected, start=2):
            plot_terms.append(
                f"'{data_path}' using 1:{col} with lines title '{idx:02d} {SHAPES[idx]}'"
            )

        script_path.write_text(
            "\n".join(
                [
                    "set terminal pngcairo size 1400,900",
                    f"set output '{path}'",
                    "set title 'MCL LFO previews'",
                    "set xlabel 'sample'",
                    "set ylabel 'preview value'",
                    "set xrange [0:127]",
                    "set yrange [0:128]",
                    "set grid",
                    "set key outside",
                    "plot " + ", ".join(plot_terms),
                    "",
                ]
            )
        )
        subprocess.run(["gnuplot", str(script_path)], check=True)
    print(path)


def extract_current_table(source):
    marker = "const uint8_t LFOSeqTrack::wav_tables"
    start = source.find(marker)
    if start < 0:
        raise ValueError("could not find LFOSeqTrack::wav_tables definition")

    brace_start = source.find("{", start)
    if brace_start < 0:
        raise ValueError("could not find wavetable opening brace")

    depth = 0
    for idx in range(brace_start, len(source)):
        char = source[idx]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[brace_start : idx + 1]
    raise ValueError("could not find wavetable closing brace")


def parse_rows(table_text):
    rows = []
    depth = 0
    row_start = None
    for idx, char in enumerate(table_text):
        if char == "{":
            depth += 1
            if depth == 2:
                row_start = idx + 1
        elif char == "}":
            if depth == 2 and row_start is not None:
                row_text = table_text[row_start:idx]
                rows.append([int(value) for value in re.findall(r"\b\d+\b", row_text)])
                row_start = None
            depth -= 1
    return rows


def verify(path):
    expected = generate_tables()
    current = parse_rows(extract_current_table(path.read_text()))
    if current != expected:
        raise SystemExit(f"{path}: generated LFO tables do not match")
    print(f"{path}: generated LFO tables match")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--initializer-only",
        action="store_true",
        help="print only the braced initializer",
    )
    parser.add_argument(
        "--verify",
        type=Path,
        metavar="LFOSEQTRACK_CPP",
        help="compare generated values against an existing LFOSeqTrack.cpp",
    )
    parser.add_argument(
        "--ascii",
        action="store_true",
        help="render the generated shapes as terminal ASCII plots",
    )
    parser.add_argument(
        "--gnuplot",
        type=Path,
        metavar="PNG",
        help="render the generated shapes to a PNG using gnuplot",
    )
    parser.add_argument(
        "--shape",
        help="limit render output to one shape by index or name",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=128,
        help="ASCII plot width, default: 128",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=17,
        help="ASCII plot height, default: 17",
    )
    args = parser.parse_args()

    tables = generate_tables()

    if args.verify:
        verify(args.verify)
        return

    if args.ascii:
        print(render_ascii(tables, args.width, args.height, args.shape))
        return

    if args.gnuplot:
        write_gnuplot_png(tables, args.gnuplot, args.shape)
        return

    print(format_table(tables, args.initializer_only))


if __name__ == "__main__":
    main()
