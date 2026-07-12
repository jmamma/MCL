#!/usr/bin/env python3
"""One-time migration helper for the legacy MCL LaTeX manual.

The generated Markdown is intended to become the editable source of truth under
docs/manual. This script is deliberately conservative: it handles the small
LaTeX subset used by the old manual and records missing legacy includes instead
of hiding them.
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LEGACY_ROOT = REPO_ROOT.parent / "MCL_Documentation" / "MCL Documentation"
DEFAULT_OUTPUT_ROOT = REPO_ROOT / "docs" / "manual"


@dataclass
class Section:
    include_name: str
    title: str
    source_path: Path | None
    output_path: Path
    missing: bool = False


def remove_comments(text: str) -> str:
    lines: list[str] = []
    for line in text.splitlines():
        escaped = False
        out: list[str] = []
        for ch in line:
            if ch == "%" and not escaped:
                break
            out.append(ch)
            escaped = ch == "\\" and not escaped
            if ch != "\\":
                escaped = False
        lines.append("".join(out))
    return "\n".join(lines)


def parse_group(text: str, start: int, opener: str = "{", closer: str = "}") -> tuple[str, int]:
    if start >= len(text) or text[start] != opener:
        raise ValueError(f"expected {opener!r} at {start}")
    depth = 0
    escaped = False
    out: list[str] = []
    i = start
    while i < len(text):
        ch = text[i]
        if escaped:
            out.append(ch)
            escaped = False
            i += 1
            continue
        if ch == "\\":
            out.append(ch)
            escaped = True
            i += 1
            continue
        if ch == opener:
            if depth > 0:
                out.append(ch)
            depth += 1
            i += 1
            continue
        if ch == closer:
            depth -= 1
            if depth == 0:
                return "".join(out), i + 1
            out.append(ch)
            i += 1
            continue
        out.append(ch)
        i += 1
    raise ValueError(f"unterminated group at {start}")


def replace_command_args(text: str, command: str, argc: int, formatter) -> str:
    marker = "\\" + command
    out: list[str] = []
    i = 0
    while i < len(text):
        idx = text.find(marker, i)
        if idx < 0:
            out.append(text[i:])
            break
        out.append(text[i:idx])
        j = idx + len(marker)
        args: list[str] = []
        ok = True
        for _ in range(argc):
            while j < len(text) and text[j].isspace():
                j += 1
            if j >= len(text) or text[j] != "{":
                ok = False
                break
            try:
                arg, j = parse_group(text, j)
            except ValueError:
                ok = False
                break
            args.append(arg)
        if ok:
            out.append(formatter(*args))
            i = j
        else:
            out.append(marker)
            i = idx + len(marker)
    return "".join(out)


def clean_inline(text: str) -> str:
    replacements = {
        r"\&": "&",
        r"\%": "%",
        r"\_": "_",
        r"\#": "#",
        r"\$": "$",
        r"\{": "{",
        r"\}": "}",
        "~": " ",
    }
    for old, new in replacements.items():
        text = text.replace(old, new)

    text = strip_color_commands(text)
    text = replace_command_args(text, "textbf", 1, lambda value: f"**{clean_inline(value)}**")
    text = replace_command_args(text, "textit", 1, lambda value: f"_{clean_inline(value)}_")
    text = replace_command_args(text, "emph", 1, lambda value: f"_{clean_inline(value)}_")
    text = replace_command_args(text, "underline", 1, lambda value: f"<u>{clean_inline(value)}</u>")
    text = replace_command_args(text, "texttt", 1, lambda value: f"`{clean_inline(value)}`")
    text = replace_command_args(text, "footnote", 1, lambda value: f" _(Note: {clean_inline(value)})_")

    text = re.sub(r"\\[a-zA-Z]+\*?(?:\[[^\]]*\])?", "", text)
    text = text.replace("{", "").replace("}", "")
    text = re.sub(r"(?<!`) `<([^>\n]{1,40})>`", lambda m: f" `{m.group(1)}`", text)
    text = re.sub(r"(?<![<`])<([A-Za-z][A-Za-z0-9 +/|._-]{0,38})>(?![`>])", lambda m: f"`{m.group(1)}`", text)
    text = re.sub(r"[ \t]+", " ", text)
    return text.strip()


def strip_color_commands(text: str) -> str:
    """Drop LaTeX color commands while preserving visible cell text."""
    for command in ("rowcolor", "cellcolor", "color"):
        marker = "\\" + command
        out: list[str] = []
        i = 0
        while i < len(text):
            idx = text.find(marker, i)
            if idx < 0:
                out.append(text[i:])
                break
            out.append(text[i:idx])
            j = idx + len(marker)
            while j < len(text) and text[j].isspace():
                j += 1
            if j < len(text) and text[j] == "[":
                try:
                    _, j = parse_group(text, j, "[", "]")
                except ValueError:
                    pass
            while j < len(text) and text[j].isspace():
                j += 1
            if j < len(text) and text[j] == "{":
                try:
                    _, j = parse_group(text, j)
                except ValueError:
                    pass
            i = j
        text = "".join(out)
    return text


def resolve_image(name: str, image_names: set[str]) -> str:
    name = clean_inline(name).strip()
    if Path(name).suffix:
        return name
    for suffix in (".png", ".jpg", ".jpeg", ".gif"):
        candidate = name + suffix
        if candidate in image_names:
            return candidate
    return name


def image_markdown(name: str, image_names: set[str]) -> str:
    filename = resolve_image(name, image_names)
    if filename not in image_names:
        return f"\n\n<!-- missing-image: {filename} -->\n> Missing legacy screenshot: `{filename}`.\n\n"
    alt = Path(filename).stem.replace("_", " ").replace("-", " ")
    return f"\n\n![{alt}](../assets/images/{filename})\n\n"


def controls_table(rows: list[tuple[str, str]]) -> str:
    body = ["", "| Control | Assignment |", "| --- | --- |"]
    body.extend(f"| {clean_inline(control)} | {clean_inline(value)} |" for control, value in rows)
    body.append("")
    return "\n".join(body)


def convert_nested_tabular_cells(text: str) -> str:
    pattern = re.compile(r"\\begin\{tabular\}\[c\]\{@\{\}l@\{\}\}(.*?)\\end\{tabular\}", re.DOTALL)

    def repl(match: re.Match[str]) -> str:
        value = match.group(1)
        value = re.sub(r"\\\\(?:\[[^\]]+\])?", "<br>", value)
        return clean_inline(value)

    return pattern.sub(repl, text)


def split_latex_rows(body: str) -> list[str]:
    rows: list[str] = []
    current: list[str] = []
    i = 0
    while i < len(body):
        if body.startswith("\\\\", i):
            rows.append("".join(current))
            current = []
            i += 2
            if i < len(body) and body[i] == "[":
                try:
                    _, i = parse_group(body, i, "[", "]")
                except ValueError:
                    pass
            continue
        current.append(body[i])
        i += 1
    tail = "".join(current)
    if tail.strip():
        rows.append(tail)
    return rows


def split_latex_cells(row: str) -> list[str]:
    cells: list[str] = []
    current: list[str] = []
    depth = 0
    escaped = False
    for ch in row:
        if escaped:
            current.append(ch)
            escaped = False
            continue
        if ch == "\\":
            current.append(ch)
            escaped = True
            continue
        if ch == "{":
            depth += 1
        elif ch == "}" and depth:
            depth -= 1
        if ch == "&" and depth == 0:
            cells.append("".join(current))
            current = []
        else:
            current.append(ch)
    cells.append("".join(current))
    return cells


def convert_tabular_block(body: str) -> str:
    body = strip_color_commands(body)
    body = replace_command_args(body, "multicolumn", 3, lambda _cols, _fmt, value: value)
    body = body.replace("\\hline", "\n")
    rows: list[list[str]] = []
    for row in split_latex_rows(body):
        row = row.strip()
        if not row:
            continue
        raw_cells = split_latex_cells(row)
        cells = [clean_inline(cell) for cell in raw_cells]
        if cells and not cells[0] and rows:
            continuation = " ".join(cell for cell in cells[1:] if cell)
            if continuation:
                target_index = min(1, len(rows[-1]) - 1)
                rows[-1][target_index] = f"{rows[-1][target_index]} {continuation}".strip()
            continue
        cells = [cell for cell in cells if cell]
        if cells:
            rows.append(cells)
    if not rows:
        return ""

    width = max(len(row) for row in rows)
    normalized = [row + [""] * (width - len(row)) for row in rows]
    lines = ["", "| " + " | ".join(normalized[0]) + " |", "| " + " | ".join("---" for _ in range(width)) + " |"]
    for row in normalized[1:]:
        lines.append("| " + " | ".join(row) + " |")
    lines.append("")
    return "\n".join(lines)


def convert_tabulars(text: str) -> str:
    text = convert_nested_tabular_cells(text)
    pattern = re.compile(r"\\begin\{tabular\}(?:\[[^\]]+\])?\{[^}]*\}(.*?)\\end\{tabular\}", re.DOTALL)
    previous = None
    while previous != text:
        previous = text
        text = pattern.sub(lambda match: convert_tabular_block(match.group(1)), text)
    return text


def convert_images(text: str, image_names: set[str]) -> str:
    text = replace_command_args(text, "screenshot", 1, lambda name: image_markdown(name, image_names))

    pattern = re.compile(r"\\includegraphics(?:\[[^\]]+\])?\{([^}]+)\}")
    return pattern.sub(lambda match: image_markdown(match.group(1), image_names), text)


def convert_headings(text: str) -> str:
    text = replace_command_args(text, "chapter", 1, lambda title: f"\n# {clean_inline(title)}\n")
    text = replace_command_args(text, "section", 1, lambda title: f"\n## {clean_inline(title)}\n")
    text = replace_command_args(text, "subsection", 1, lambda title: f"\n### {clean_inline(title)}\n")
    text = replace_command_args(text, "subsubsection", 1, lambda title: f"\n#### {clean_inline(title)}\n")
    return text


def convert_links(text: str) -> str:
    text = replace_command_args(text, "href", 2, lambda url, label: f"[{clean_inline(label)}]({clean_inline(url)})")
    text = replace_command_args(text, "url", 1, lambda url: f"<{clean_inline(url)}>")
    return text


def extract_verbatim(text: str) -> tuple[str, dict[str, str]]:
    blocks: dict[str, str] = {}

    def repl(match: re.Match[str]) -> str:
        key = f"@@VERBATIM_{len(blocks)}@@"
        blocks[key] = "\n```text\n" + match.group(1).strip("\n") + "\n```\n"
        return key

    text = re.sub(r"\\begin\{verbatim\}(.*?)\\end\{verbatim\}", repl, text, flags=re.DOTALL)
    return text, blocks


def restore_verbatim(text: str, blocks: dict[str, str]) -> str:
    for key, value in blocks.items():
        text = text.replace(key, value)
    return text


def convert_environments_and_lines(text: str) -> str:
    text = re.sub(r"\\begin\{(?:figure|center|small)\}(?:\[[^\]]+\])?", "\n", text)
    text = re.sub(r"\\end\{(?:figure|center|small)\}", "\n", text)
    text = re.sub(r"\\begin\{itemize\}", "\n", text)
    text = re.sub(r"\\end\{itemize\}", "\n", text)
    text = re.sub(r"\\begin\{enumerate\}", "\n", text)
    text = re.sub(r"\\end\{enumerate\}", "\n", text)
    text = text.replace("\\newpage", "\n")
    text = text.replace("\\clearpage", "\n")
    text = re.sub(r"\\\\(?:\[[^\]]+\])?", "\n", text)

    converted: list[str] = []
    enumerate_mode = False
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            converted.append("")
            continue
        item_match = re.match(r"\\item(?:\s+|$)(.*)", line)
        if item_match:
            converted.append(f"- {clean_inline(item_match.group(1))}")
            continue
        if re.match(r"^\d+\.\s+", line):
            enumerate_mode = True
        elif enumerate_mode and line.startswith("- "):
            line = "1. " + line[2:]
        converted.append(clean_inline(line))
    return "\n".join(converted)


def convert_latex_to_markdown(text: str, image_names: set[str]) -> str:
    text = remove_comments(text)
    text, verbatim = extract_verbatim(text)
    text = convert_links(text)
    text = convert_headings(text)
    text = convert_images(text, image_names)
    text = replace_command_args(
        text,
        "encodersbuttons",
        8,
        lambda e1, e2, e3, e4, b1, b2, b3, b4: controls_table(
            [
                ("Encoder 1", e1),
                ("Encoder 2", e2),
                ("Encoder 3", e3),
                ("Encoder 4", e4),
                ("Save / No", b1),
                ("Page", b2),
                ("Load / Yes", b3),
                ("Shift", b4),
            ]
        ),
    )
    text = replace_command_args(
        text,
        "encoders",
        4,
        lambda e1, e2, e3, e4: controls_table(
            [("Encoder 1", e1), ("Encoder 2", e2), ("Encoder 3", e3), ("Encoder 4", e4)]
        ),
    )
    text = replace_command_args(
        text,
        "buttons",
        4,
        lambda b1, b2, b3, b4: controls_table(
            [("Save / No", b1), ("Page", b2), ("Load / Yes", b3), ("Shift", b4)]
        ),
    )
    text = replace_command_args(text, "fbox", 1, lambda value: value)
    text = convert_tabulars(text)
    text = convert_environments_and_lines(text)
    text = restore_verbatim(text, verbatim)
    text = normalize_markdown(text)
    return text


def normalize_markdown(text: str) -> str:
    lines = [line.rstrip() for line in text.splitlines()]
    out: list[str] = []
    blank_count = 0
    for line in lines:
        if line:
            out.append(line)
            blank_count = 0
        else:
            blank_count += 1
            if blank_count <= 2:
                out.append("")
    return "\n".join(out).strip() + "\n"


def slugify(value: str) -> str:
    value = re.sub(r"([a-z0-9])([A-Z])", r"\1-\2", value)
    value = value.lower().replace("_", "-")
    value = re.sub(r"[^a-z0-9]+", "-", value)
    return value.strip("-") or "section"


def title_from_source(stem: str, markdown: str) -> str:
    for line in markdown.splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return stem.replace("_", " ").replace("-", " ").title()


def parse_includes(main_tex: Path) -> list[str]:
    includes: list[str] = []
    for raw_line in main_tex.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if line.startswith("%"):
            continue
        match = re.search(r"\\include\{([^}]+)\}", line)
        if match:
            includes.append(match.group(1))
    return includes


def write_if_allowed(path: Path, content: str, force: bool) -> None:
    if path.exists() and not force:
        raise FileExistsError(f"{path} exists; use --force to overwrite generated migration files")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def build_index() -> str:
    return """# MCL

This Markdown manual was migrated from the legacy LaTeX/PDF documentation so it can be maintained with the MCL source code and published through GitHub Pages.

> Migration status: the first pass preserves the old manual content. The content still needs the MCL 5.00 audit and screenshot refresh described in [Manual Audit](manual_audit.md).

![MCL logo](assets/images/mcl_logo_black_short.png)
"""


def build_readme() -> str:
    return """# MCL Source

The editable manual now lives in Markdown under this directory. The generated GitHub Pages site is built from these files by `tools/docs/build_manual_site.py`.

Useful commands from the `MCL` repository root:

```sh
python3 tools/docs/validate_manual.py
python3 tools/docs/build_manual_site.py
```

The legacy LaTeX repository is no longer the source of truth. Use `tools/docs/convert_latex_manual.py` only for migration archaeology or for recreating the initial Markdown import.
"""


def build_audit(output_root: Path, missing_sections: list[Section], missing_images: list[str]) -> str:
    lines = [
        "# Manual Audit",
        "",
        "This file tracks the documentation migration and the MCL 5.00 content refresh.",
        "",
        "## Migration Findings",
        "",
    ]
    if missing_sections:
        lines.append("Missing legacy include files from `main.tex`:")
        lines.append("")
        for section in missing_sections:
            lines.append(f"- `{section.include_name}.tex` -> `{section.output_path.relative_to(output_root)}`")
    else:
        lines.append("No missing legacy include files were found.")
    lines.append("")
    if missing_images:
        lines.append("Referenced legacy images not present during migration:")
        lines.append("")
        for image in missing_images:
            lines.append(f"- `{image}`")
    else:
        lines.append("No missing referenced images were found during migration.")
    lines.extend(
        [
            "",
            "## MCL 5.00 Refresh Checklist",
            "",
            "- [ ] Resolve canonical MCL 5.00 release date between top-level and source changelogs.",
            "- [ ] Rewrite key concepts around Grid X/Y devices, TBD, platform support, and project format changes.",
            "- [ ] Update MIDI, controller, sync, route, MD, and system configuration labels from `resource/menu_layouts.cpp` and `resource/menu_options.cpp`.",
            "- [ ] Update project load/save documentation for folders, cloning, moving, versions, and project config.",
            "- [ ] Update grid and slot menu documentation, including the `SOUND` slot option.",
            "- [ ] Update sequencer documentation for swing, mute masks, fill conditions, current condition labels, and signed microtiming behavior.",
            "- [ ] Update LFO and arp documentation for per-track storage and new destinations/options.",
            "- [ ] Update polyphony/chromatic documentation for `POLY MODE` and multi-timbral behavior.",
            "- [ ] Update mixer/performance documentation for mute/fill modes and performance-state fill storage.",
            "- [ ] Update piano roll documentation for selection, zoom, copy/paste, and expanded MIDI locks.",
            "- [ ] Update sample browser, sound browser, WAV designer, RAM, and MD samplebank-linking sections.",
            "- [ ] Replace stale screenshots with SPS/MCL headless captures where possible.",
            "",
            "## Code Truth Sources",
            "",
            "- `Changelog`",
            "- `src/mcl/Changelog`",
            "- `resource/menu_layouts.cpp`",
            "- `resource/menu_options.cpp`",
            "- `src/mcl/MCL/PageIndex.h`",
            "- `src/mcl/MCL/MCLDefines.h`",
            "- `src/mcl/MCL/MCLSysConfig.h`",
            "- `src/mcl/MCL/Sequencer`",
        ]
    )
    return "\n".join(lines) + "\n"


def migrate(args: argparse.Namespace) -> int:
    legacy_root = args.latex_root.resolve()
    output_root = args.output.resolve()
    main_tex = legacy_root / "main.tex"
    tex_root = legacy_root / "TeX_files"
    image_root = legacy_root / "images"

    if not main_tex.exists():
        print(f"missing legacy main.tex: {main_tex}", file=sys.stderr)
        return 2
    if not tex_root.exists():
        print(f"missing legacy TeX_files directory: {tex_root}", file=sys.stderr)
        return 2
    if not image_root.exists():
        print(f"missing legacy images directory: {image_root}", file=sys.stderr)
        return 2

    image_names = {path.name for path in image_root.iterdir() if path.is_file()}
    sections_dir = output_root / "sections"
    assets_dir = output_root / "assets" / "images"
    sections_dir.mkdir(parents=True, exist_ok=True)
    assets_dir.mkdir(parents=True, exist_ok=True)

    for image in sorted(image_root.iterdir()):
        if image.is_file():
            target = assets_dir / image.name
            if target.exists() and not args.force:
                raise FileExistsError(f"{target} exists; use --force to overwrite copied assets")
            shutil.copy2(image, target)

    sections: list[Section] = []
    missing_sections: list[Section] = []
    missing_images: set[str] = set()

    for include_name in parse_includes(main_tex):
        stem = Path(include_name).name
        source_path = legacy_root / f"{include_name}.tex"
        slug = slugify(stem)
        output_path = sections_dir / f"{slug}.md"
        if not source_path.exists():
            title = stem.replace("_", " ").replace("-", " ").title()
            section = Section(include_name, title, None, output_path, missing=True)
            missing_sections.append(section)
            sections.append(section)
            markdown = (
                f"# {title}\n\n"
                f"> Migration note: legacy `main.tex` included `{include_name}.tex`, "
                "but that file was not present when the manual was migrated. "
                "Reconstruct this section during the MCL 5.00 documentation audit.\n"
            )
            write_if_allowed(output_path, markdown, args.force)
            continue

        markdown = convert_latex_to_markdown(source_path.read_text(encoding="utf-8"), image_names)
        for match in re.finditer(r"\]\(\.\./assets/images/([^)]+)\)", markdown):
            image_name = match.group(1)
            if image_name not in image_names:
                missing_images.add(image_name)
        for match in re.finditer(r"missing-image:\s*([^\s>]+)", markdown):
            missing_images.add(match.group(1))
        title = title_from_source(stem, markdown)
        section = Section(include_name, title, source_path, output_path)
        sections.append(section)
        write_if_allowed(output_path, markdown, args.force)

    summary_lines = ["# Summary", "", "- [MCL](index.md)", "- [Manual Audit](manual_audit.md)", ""]
    for section in sections:
        rel = section.output_path.relative_to(output_root).as_posix()
        summary_lines.append(f"- [{section.title}]({rel})")

    write_if_allowed(output_root / "index.md", build_index(), args.force)
    write_if_allowed(output_root / "README.md", build_readme(), args.force)
    write_if_allowed(output_root / "SUMMARY.md", "\n".join(summary_lines) + "\n", args.force)
    write_if_allowed(output_root / "manual_audit.md", build_audit(output_root, missing_sections, sorted(missing_images)), args.force)

    print(f"Converted {len([s for s in sections if not s.missing])} sections to {output_root}")
    if missing_sections:
        print(f"Recorded {len(missing_sections)} missing legacy includes in manual_audit.md")
    if missing_images:
        print(f"Recorded {len(missing_images)} missing referenced images in manual_audit.md")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--latex-root", type=Path, default=DEFAULT_LEGACY_ROOT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--force", action="store_true", help="overwrite generated migration files")
    args = parser.parse_args()
    return migrate(args)


if __name__ == "__main__":
    raise SystemExit(main())
