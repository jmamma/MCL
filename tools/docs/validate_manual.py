#!/usr/bin/env python3
"""Validate the Markdown manual before building or publishing."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlparse


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANUAL_ROOT = REPO_ROOT / "docs" / "manual"


MARKDOWN_LINK_RE = re.compile(r"(?<!!)\[([^\]]+)\]\(([^)]+)\)")
MARKDOWN_IMAGE_RE = re.compile(r"!\[([^\]]*)\]\(([^)]+)\)")
SUMMARY_RE = re.compile(r"^\s*-\s+\[([^\]]+)\]\(([^)]+)\)")


def is_external_link(target: str) -> bool:
    parsed = urlparse(target)
    return parsed.scheme in {"http", "https", "mailto"}


def split_target(target: str) -> tuple[str, str]:
    path, _, anchor = target.partition("#")
    return unquote(path), anchor


def heading_anchors(markdown: str) -> set[str]:
    anchors: set[str] = set()
    for line in markdown.splitlines():
        match = re.match(r"^(#{1,6})\s+(.+)$", line)
        if not match:
            continue
        title = re.sub(r"`([^`]+)`", r"\1", match.group(2))
        title = re.sub(r"<[^>]+>", "", title)
        anchor = re.sub(r"[^a-z0-9 -]", "", title.lower())
        anchor = re.sub(r"\s+", "-", anchor).strip("-")
        if anchor:
            anchors.add(anchor)
    return anchors


def validate_manual(root: Path) -> list[str]:
    errors: list[str] = []
    if not root.exists():
        return [f"manual root does not exist: {root}"]

    summary = root / "SUMMARY.md"
    if not summary.exists():
        errors.append("SUMMARY.md is missing")
    else:
        for lineno, line in enumerate(summary.read_text(encoding="utf-8").splitlines(), start=1):
            match = SUMMARY_RE.match(line)
            if not match:
                continue
            target, _ = split_target(match.group(2))
            if target and not (root / target).exists():
                errors.append(f"SUMMARY.md:{lineno}: missing target {target}")

    markdown_files = sorted(root.rglob("*.md"))
    anchors_by_file = {path: heading_anchors(path.read_text(encoding="utf-8")) for path in markdown_files}

    for path in markdown_files:
        rel = path.relative_to(root)
        text = path.read_text(encoding="utf-8")
        for regex, kind in ((MARKDOWN_IMAGE_RE, "image"), (MARKDOWN_LINK_RE, "link")):
            for match in regex.finditer(text):
                raw_target = match.group(2).strip()
                if is_external_link(raw_target):
                    continue
                target, anchor = split_target(raw_target)
                if not target:
                    target_path = path
                else:
                    target_path = (path.parent / target).resolve()
                try:
                    target_path.relative_to(root.resolve())
                except ValueError:
                    errors.append(f"{rel}: {kind} escapes manual root: {raw_target}")
                    continue
                if target and not target_path.exists():
                    errors.append(f"{rel}: missing {kind} target: {raw_target}")
                    continue
                if anchor and target_path.suffix == ".md":
                    anchors = anchors_by_file.get(target_path)
                    if anchors is not None and anchor not in anchors:
                        errors.append(f"{rel}: missing anchor #{anchor} in {target_path.relative_to(root)}")

    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manual-root", type=Path, default=DEFAULT_MANUAL_ROOT)
    args = parser.parse_args()
    errors = validate_manual(args.manual_root.resolve())
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    print(f"Manual validation passed: {args.manual_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
