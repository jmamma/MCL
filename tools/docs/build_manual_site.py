#!/usr/bin/env python3
"""Build the Markdown manual into a dependency-free static HTML site."""

from __future__ import annotations

import argparse
import html
import json
import posixpath
import re
import shutil
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANUAL_ROOT = REPO_ROOT / "docs" / "manual"
DEFAULT_SITE_ROOT = REPO_ROOT / "docs" / "site"


SUMMARY_RE = re.compile(r"^\s*-\s+\[([^\]]+)\]\(([^)]+)\)")


@dataclass(frozen=True)
class NavItem:
    title: str
    source: Path
    href: str


def slugify(value: str) -> str:
    value = re.sub(r"`([^`]+)`", r"\1", value)
    value = re.sub(r"<[^>]+>", "", value)
    value = re.sub(r"[^a-z0-9 -]", "", value.lower())
    return re.sub(r"\s+", "-", value).strip("-")


def parse_nav(manual_root: Path) -> list[NavItem]:
    summary = manual_root / "SUMMARY.md"
    items: list[NavItem] = []
    for line in summary.read_text(encoding="utf-8").splitlines():
        match = SUMMARY_RE.match(line)
        if not match:
            continue
        title = match.group(1)
        target = match.group(2)
        source = manual_root / target
        href = Path(target).with_suffix(".html").as_posix()
        items.append(NavItem(title=title, source=source, href=href))
    return items


def extract_title(markdown: str, fallback: str) -> str:
    for line in markdown.splitlines():
        match = re.match(r"^#\s+(.+)$", line)
        if match:
            return strip_inline_markup(match.group(1))
    return fallback


def strip_inline_markup(value: str) -> str:
    value = re.sub(r"!\[([^\]]*)\]\([^)]+\)", r"\1", value)
    value = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", value)
    value = value.replace("`", "").replace("*", "").replace("_", "")
    value = re.sub(r"<[^>]+>", "", value)
    return value.strip()


def parse_inline(value: str) -> str:
    code_values: list[str] = []

    def store_code(match: re.Match[str]) -> str:
        code_values.append(html.escape(match.group(1)))
        return f"@@CODE{len(code_values) - 1}@@"

    value = re.sub(r"`([^`]+)`", store_code, value)
    value = html.escape(value)
    value = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", value)
    value = re.sub(r"(?<!\w)_([^_]+)_(?!\w)", r"<em>\1</em>", value)
    value = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", lambda m: f'<a href="{html.escape(link_target(m.group(2)))}">{parse_inline(m.group(1))}</a>', value)
    for index, code in enumerate(code_values):
        value = value.replace(f"@@CODE{index}@@", f"<code>{code}</code>")
    return value


def link_target(target: str) -> str:
    if target.endswith(".md"):
        return target[:-3] + ".html"
    if ".md#" in target:
        return target.replace(".md#", ".html#")
    return target


def image_block(line: str) -> str | None:
    match = re.match(r"!\[([^\]]*)\]\(([^)]+)\)", line.strip())
    if not match:
        return None
    alt = html.escape(match.group(1))
    src = html.escape(link_target(match.group(2)))
    return f'<figure><img src="{src}" alt="{alt}" loading="lazy"><figcaption>{alt}</figcaption></figure>'


def table_html(lines: list[str]) -> str:
    rows = []
    for line in lines:
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if all(re.fullmatch(r":?-{3,}:?", cell) for cell in cells):
            continue
        rows.append(cells)
    if not rows:
        return ""
    header = rows[0]
    body = rows[1:]
    out = ["<div class=\"table-wrap\"><table>", "<thead><tr>"]
    out.extend(f"<th>{parse_inline(cell)}</th>" for cell in header)
    out.append("</tr></thead>")
    if body:
        out.append("<tbody>")
        for row in body:
            out.append("<tr>")
            out.extend(f"<td>{parse_inline(cell)}</td>" for cell in row)
            out.append("</tr>")
        out.append("</tbody>")
    out.append("</table></div>")
    return "\n".join(out)


def render_markdown(markdown: str) -> tuple[str, list[tuple[int, str, str]]]:
    lines = markdown.splitlines()
    out: list[str] = []
    headings: list[tuple[int, str, str]] = []
    i = 0
    in_code = False
    code_lang = ""
    code_lines: list[str] = []

    def flush_code() -> None:
        escaped = html.escape("\n".join(code_lines))
        lang_class = f" language-{html.escape(code_lang)}" if code_lang else ""
        out.append(f'<pre><code class="{lang_class}">{escaped}</code></pre>')

    while i < len(lines):
        line = lines[i]
        if line.startswith("```"):
            if in_code:
                flush_code()
                code_lines = []
                in_code = False
                code_lang = ""
            else:
                in_code = True
                code_lang = line.strip("`").strip()
            i += 1
            continue
        if in_code:
            code_lines.append(line)
            i += 1
            continue
        if not line.strip():
            i += 1
            continue
        if line.strip().startswith("<!--"):
            while i < len(lines):
                done = "-->" in lines[i]
                i += 1
                if done:
                    break
            continue

        image = image_block(line)
        if image:
            out.append(image)
            i += 1
            continue

        heading = re.match(r"^(#{1,6})\s+(.+)$", line)
        if heading:
            level = len(heading.group(1))
            title = strip_inline_markup(heading.group(2))
            anchor = slugify(title)
            headings.append((level, title, anchor))
            out.append(f'<h{level} id="{anchor}">{parse_inline(heading.group(2))}</h{level}>')
            i += 1
            continue

        if line.startswith("> "):
            quote_lines = []
            while i < len(lines) and lines[i].startswith("> "):
                quote_lines.append(lines[i][2:])
                i += 1
            out.append("<blockquote>" + "\n".join(f"<p>{parse_inline(q)}</p>" for q in quote_lines) + "</blockquote>")
            continue

        if line.startswith("|") and i + 1 < len(lines) and lines[i + 1].startswith("|"):
            table_lines = []
            while i < len(lines) and lines[i].startswith("|"):
                table_lines.append(lines[i])
                i += 1
            out.append(table_html(table_lines))
            continue

        if re.match(r"^\s*[-*]\s+", line):
            items = []
            while i < len(lines) and re.match(r"^\s*[-*]\s+", lines[i]):
                items.append(re.sub(r"^\s*[-*]\s+", "", lines[i]))
                i += 1
            out.append("<ul>" + "".join(f"<li>{parse_inline(item)}</li>" for item in items) + "</ul>")
            continue

        if re.match(r"^\s*\d+\.\s+", line):
            items = []
            while i < len(lines) and re.match(r"^\s*\d+\.\s+", lines[i]):
                items.append(re.sub(r"^\s*\d+\.\s+", "", lines[i]))
                i += 1
            out.append("<ol>" + "".join(f"<li>{parse_inline(item)}</li>" for item in items) + "</ol>")
            continue

        paragraph = [line.strip()]
        i += 1
        while i < len(lines) and lines[i].strip() and not re.match(r"^(#{1,6})\s+", lines[i]) and not lines[i].startswith(("```", "|", "> ", "![", "- ")):
            paragraph.append(lines[i].strip())
            i += 1
        out.append(f"<p>{parse_inline(' '.join(paragraph))}</p>")

    if in_code:
        flush_code()
    return "\n".join(out), headings


def site_css() -> str:
    return """\
:root {
  --bg: #f7f7f4;
  --surface: #ffffff;
  --text: #202421;
  --muted: #666d68;
  --line: #d8ddd7;
  --accent: #126e5d;
  --accent-soft: #e3f0ec;
  --code: #eef1ec;
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 16px/1.55 system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
}

a {
  color: var(--accent);
}

.layout {
  display: grid;
  grid-template-columns: minmax(220px, 300px) minmax(0, 1fr);
  min-height: 100vh;
}

.sidebar {
  position: sticky;
  top: 0;
  height: 100vh;
  overflow: auto;
  border-right: 1px solid var(--line);
  background: #fbfbf8;
  padding: 20px 16px;
}

.brand {
  font-weight: 700;
  font-size: 1rem;
  margin-bottom: 14px;
}

.nav-filter {
  width: 100%;
  min-height: 36px;
  border: 1px solid var(--line);
  border-radius: 6px;
  padding: 7px 9px;
  color: var(--text);
  background: var(--surface);
}

.nav {
  display: flex;
  flex-direction: column;
  gap: 2px;
  margin-top: 14px;
}

.nav a {
  display: block;
  border-radius: 6px;
  padding: 7px 8px;
  color: var(--text);
  text-decoration: none;
}

.nav a:hover,
.nav a.active {
  background: var(--accent-soft);
  color: #0b4f42;
}

.content {
  width: min(100%, 980px);
  padding: 42px 34px 80px;
}

.doc-meta {
  color: var(--muted);
  font-size: 0.9rem;
  margin-bottom: 28px;
}

h1,
h2,
h3,
h4 {
  line-height: 1.2;
  letter-spacing: 0;
}

h1 {
  font-size: 2.1rem;
  margin: 0 0 18px;
}

h2 {
  font-size: 1.45rem;
  margin-top: 38px;
  padding-top: 10px;
  border-top: 1px solid var(--line);
}

h3 {
  font-size: 1.15rem;
  margin-top: 28px;
}

p,
ul,
ol,
blockquote,
table,
figure,
pre {
  margin: 16px 0;
}

blockquote {
  border-left: 4px solid var(--accent);
  background: var(--accent-soft);
  padding: 10px 14px;
  color: #24453d;
}

code {
  border-radius: 4px;
  background: var(--code);
  padding: 0.1em 0.3em;
  font-size: 0.92em;
}

pre {
  overflow: auto;
  border: 1px solid var(--line);
  border-radius: 6px;
  background: #f0f2ee;
  padding: 14px;
}

pre code {
  background: transparent;
  padding: 0;
}

figure {
  margin: 24px 0;
}

figure img {
  display: block;
  max-width: 100%;
  height: auto;
  border: 1px solid var(--line);
  border-radius: 6px;
  background: var(--surface);
}

figcaption {
  color: var(--muted);
  font-size: 0.88rem;
  margin-top: 6px;
}

.table-wrap {
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  background: var(--surface);
}

th,
td {
  border: 1px solid var(--line);
  padding: 8px 10px;
  vertical-align: top;
}

th {
  background: #edf1ec;
  text-align: left;
}

.pager {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  margin-top: 54px;
  padding-top: 22px;
  border-top: 1px solid var(--line);
}

.pager a {
  max-width: 46%;
  text-decoration: none;
}

@media (max-width: 840px) {
  .layout {
    grid-template-columns: 1fr;
  }

  .sidebar {
    position: static;
    height: auto;
    border-right: 0;
    border-bottom: 1px solid var(--line);
  }

  .content {
    padding: 28px 18px 56px;
  }

  h1 {
    font-size: 1.75rem;
  }
}
"""


def site_js() -> str:
    return """\
const filter = document.querySelector('[data-nav-filter]');
const links = Array.from(document.querySelectorAll('[data-nav-link]'));

if (filter) {
  filter.addEventListener('input', () => {
    const query = filter.value.trim().toLowerCase();
    for (const link of links) {
      const text = link.textContent.toLowerCase();
      link.hidden = query && !text.includes(query);
    }
  });
}
"""


def render_page(item: NavItem, items: list[NavItem], index: int, manual_root: Path) -> tuple[str, str]:
    markdown = item.source.read_text(encoding="utf-8")
    title = extract_title(markdown, item.title)
    body, headings = render_markdown(markdown)
    page_dir = Path(item.href).parent.as_posix()
    if page_dir == ".":
        page_dir = ""

    def page_href(target: str) -> str:
        start = page_dir or "."
        return posixpath.relpath(target, start=start)

    css_href = page_href("assets/manual.css")
    js_href = page_href("assets/manual.js")
    nav_html = "\n".join(
        f'<a data-nav-link class="{"active" if nav.href == item.href else ""}" href="{html.escape(page_href(nav.href))}">{html.escape(nav.title)}</a>'
        for nav in items
    )
    prev_link = items[index - 1] if index > 0 else None
    next_link = items[index + 1] if index + 1 < len(items) else None
    pager = '<nav class="pager" aria-label="Page navigation">'
    pager += (
        f'<a href="{html.escape(page_href(prev_link.href))}">Previous<br><strong>{html.escape(prev_link.title)}</strong></a>'
        if prev_link
        else "<span></span>"
    )
    pager += (
        f'<a href="{html.escape(page_href(next_link.href))}">Next<br><strong>{html.escape(next_link.title)}</strong></a>'
        if next_link
        else "<span></span>"
    )
    pager += "</nav>"
    rel_source = item.source.relative_to(manual_root).as_posix()
    return title, f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{html.escape(title)} - MCL Manual</title>
  <link rel="stylesheet" href="{html.escape(css_href)}">
</head>
<body>
  <div class="layout">
    <aside class="sidebar">
      <div class="brand">MegaCommand Live Manual</div>
      <input class="nav-filter" data-nav-filter type="search" placeholder="Filter sections" aria-label="Filter sections">
      <nav class="nav" aria-label="Manual sections">
        {nav_html}
      </nav>
    </aside>
    <main class="content">
      <div class="doc-meta">Source: {html.escape(rel_source)}</div>
      {body}
      {pager}
    </main>
  </div>
  <script src="{html.escape(js_href)}"></script>
</body>
</html>
"""


def build(manual_root: Path, site_root: Path) -> None:
    items = parse_nav(manual_root)
    if site_root.exists():
        shutil.rmtree(site_root)
    (site_root / "assets").mkdir(parents=True)

    assets_source = manual_root / "assets"
    if assets_source.exists():
        shutil.copytree(assets_source, site_root / "assets", dirs_exist_ok=True)
    (site_root / "assets" / "manual.css").write_text(site_css(), encoding="utf-8")
    (site_root / "assets" / "manual.js").write_text(site_js(), encoding="utf-8")

    search_index = []
    for index, item in enumerate(items):
        title, html_text = render_page(item, items, index, manual_root)
        out_path = site_root / item.href
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(html_text, encoding="utf-8")
        plain = strip_inline_markup(item.source.read_text(encoding="utf-8"))
        search_index.append({"title": title, "href": item.href, "text": plain[:4000]})

    root_index = site_root / "index.html"
    manual_index = site_root / "index.html"
    if not manual_index.exists() and items:
        shutil.copy2(site_root / items[0].href, root_index)
    (site_root / "search-index.json").write_text(json.dumps(search_index, indent=2), encoding="utf-8")
    (site_root / ".nojekyll").write_text("", encoding="utf-8")
    print(f"Built manual site: {site_root}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manual-root", type=Path, default=DEFAULT_MANUAL_ROOT)
    parser.add_argument("--site-root", type=Path, default=DEFAULT_SITE_ROOT)
    args = parser.parse_args()
    build(args.manual_root.resolve(), args.site_root.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
