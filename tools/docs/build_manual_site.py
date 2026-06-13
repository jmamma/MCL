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
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


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


def png_dimensions(path: Path) -> tuple[int, int] | None:
    try:
        data = path.read_bytes()[:24]
    except OSError:
        return None
    if len(data) < 24 or data[:8] != PNG_SIGNATURE:
        return None
    return int.from_bytes(data[16:20], "big"), int.from_bytes(data[20:24], "big")


def image_block(line: str, source_dir: Path | None = None) -> str | None:
    match = re.match(r"!\[([^\]]*)\]\(([^)]+)\)", line.strip())
    if not match:
        return None
    alt = html.escape(match.group(1))
    target = match.group(2)
    classes: list[str] = []
    figure_classes: list[str] = []
    image_target = target.split("#", 1)[0]
    is_logo = Path(image_target).name.startswith("mcl_logo")
    if is_logo:
        figure_classes.append("logo-figure")
        classes.append("logo-image")
    if source_dir is not None and not re.match(r"^[a-z]+:", target):
        dims = png_dimensions((source_dir / image_target).resolve())
        if dims in ((128, 32), (128, 64)):
            classes.append("screen-image")
    figure_class_attr = (
        f' class="{" ".join(figure_classes)}"' if figure_classes else ""
    )
    class_attr = f' class="{" ".join(classes)}"' if classes else ""
    src = html.escape(link_target(target))
    caption = "" if is_logo else f"<figcaption>{alt}</figcaption>"
    return (
        f"<figure{figure_class_attr}>"
        f'<img{class_attr} src="{src}" alt="{alt}" loading="lazy">'
        f"{caption}</figure>"
    )


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
    out = [
        f"<div class=\"table-wrap\"><table class=\"cols-{len(header)}\">",
        "<thead><tr>",
    ]
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


def render_markdown(
    markdown: str, source_dir: Path | None = None
) -> tuple[str, list[tuple[int, str, str]]]:
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

        image = image_block(line, source_dir)
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
  --bg: #eef1f5;
  --paper: #fbfbfc;
  --text: #242932;
  --muted: #66707a;
  --line: #c8ced6;
  --rule: #9aa7b4;
  --soft: #f4f6f8;
  --accent: #7b3541;
  --accent-soft: #f3e5e8;
  --link: #315f8c;
  --table-head: #e7edf4;
  --table-line: #b9c3cf;
  --code: #edf1f4;
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font: 15px/1.48 Arial, Helvetica, sans-serif;
}

a {
  color: var(--link);
  text-decoration-thickness: 1px;
  text-underline-offset: 0.16em;
}

.layout {
  display: grid;
  grid-template-columns: minmax(230px, 292px) minmax(0, 1fr);
  min-height: 100vh;
}

.sidebar {
  position: sticky;
  top: 0;
  height: 100vh;
  overflow: auto;
  border-right: 2px solid var(--rule);
  background: #e4e8ee;
  padding: 18px 14px;
}

.brand {
  border-bottom: 2px solid var(--rule);
  color: var(--text);
  font-family: Arial, Helvetica, sans-serif;
  font-weight: 700;
  font-size: 1.02rem;
  line-height: 1.1;
  margin-bottom: 13px;
  padding-bottom: 10px;
  text-transform: uppercase;
}

.nav-filter {
  width: 100%;
  min-height: 36px;
  border: 1px solid var(--line);
  border-radius: 0;
  padding: 7px 8px;
  color: var(--text);
  background: var(--paper);
  font: inherit;
}

.nav {
  display: flex;
  flex-direction: column;
  gap: 1px;
  margin-top: 14px;
}

.nav a {
  display: block;
  border-left: 3px solid transparent;
  padding: 6px 8px 6px 9px;
  color: var(--text);
  font-size: 0.88rem;
  line-height: 1.2;
  text-decoration: none;
}

.nav a:hover,
.nav a.active {
  border-left-color: var(--accent);
  background: var(--accent-soft);
  color: #4a222a;
}

.content {
  width: min(100%, 930px);
  min-height: 100vh;
  background: var(--paper);
  padding: 38px 50px 84px;
}

.doc-meta {
  color: var(--muted);
  border-top: 2px solid var(--rule);
  font-size: 0.72rem;
  font-weight: 700;
  line-height: 1;
  margin: 0 0 30px;
  padding-top: 9px;
  text-transform: uppercase;
}

h1,
h2,
h3,
h4 {
  color: var(--text);
  font-family: Arial, Helvetica, sans-serif;
  font-weight: 700;
  line-height: 1.2;
  letter-spacing: 0;
  text-transform: uppercase;
}

h1 {
  border-bottom: 2px solid var(--rule);
  font-size: 2rem;
  margin: 0 0 22px;
  padding-bottom: 12px;
}

h2 {
  border-top: 1px solid var(--rule);
  font-size: 1.22rem;
  margin: 38px 0 10px;
  padding-top: 10px;
}

h3 {
  font-size: 1.02rem;
  margin: 26px 0 8px;
}

h4 {
  font-size: 0.92rem;
  margin: 20px 0 8px;
}

p,
ul,
ol,
blockquote,
table,
figure,
pre {
  margin: 13px 0;
}

ul,
ol {
  padding-left: 1.45rem;
}

li + li {
  margin-top: 4px;
}

blockquote {
  border: 1px solid #e0c7cc;
  background: #fbf4f5;
  color: var(--text);
  padding: 11px 14px;
}

code {
  border: 1px solid var(--line);
  border-radius: 0;
  background: var(--code);
  padding: 0.08em 0.28em;
  font-size: 0.92em;
}

pre {
  overflow: auto;
  border: 1px solid var(--line);
  border-radius: 0;
  background: var(--soft);
  padding: 14px;
}

pre code {
  background: transparent;
  padding: 0;
}

figure {
  margin: 22px 0;
}

figure.logo-figure {
  margin: 0 0 24px;
}

figure img {
  display: block;
  max-width: 100%;
  height: auto;
  border: 1px solid var(--line);
  border-radius: 0;
  background: var(--paper);
}

figure img.logo-image {
  border: 0;
  width: min(100%, 300px);
}

figure img.screen-image {
  width: min(100%, 512px);
  image-rendering: crisp-edges;
  image-rendering: pixelated;
}

figcaption {
  color: var(--muted);
  font-size: 0.72rem;
  font-weight: 700;
  margin-top: 6px;
  text-transform: uppercase;
}

.table-wrap {
  border-bottom: 2px solid var(--rule);
  border-top: 2px solid var(--rule);
  margin: 18px 0 22px;
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  background: var(--paper);
  font-size: 0.9rem;
  line-height: 1.34;
}

th,
td {
  border: 0;
  border-bottom: 1px solid var(--table-line);
  padding: 7px 9px;
  vertical-align: top;
}

th {
  background: var(--table-head);
  color: var(--text);
  font-size: 0.75rem;
  font-weight: 700;
  text-align: left;
  text-transform: uppercase;
}

tbody tr:last-child td {
  border-bottom: 0;
}

td:first-child {
  color: var(--text);
  font-weight: 700;
}

table.cols-2 th:first-child,
table.cols-2 td:first-child {
  min-width: 12rem;
  width: 30%;
}

table.cols-3 th:first-child,
table.cols-3 td:first-child {
  min-width: 9rem;
  width: 22%;
}

table.cols-4 th,
table.cols-4 td {
  padding: 6px 8px;
}

table code {
  background: var(--paper);
  white-space: nowrap;
}

.pager {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  margin-top: 54px;
  padding-top: 22px;
  border-top: 2px solid var(--rule);
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
    border-bottom: 2px solid var(--rule);
  }

  .content {
    padding: 28px 18px 56px;
  }

  h1 {
    font-size: 1.55rem;
  }

  table.cols-2 th:first-child,
  table.cols-2 td:first-child,
  table.cols-3 th:first-child,
  table.cols-3 td:first-child {
    min-width: 10rem;
  }
}
"""


def site_js() -> str:
    return """\
const sidebar = document.querySelector('.sidebar');
const filter = document.querySelector('[data-nav-filter]');
const links = Array.from(document.querySelectorAll('[data-nav-link]'));
const sidebarScrollKey = 'mcl-manual-sidebar-scroll';

if (sidebar) {
  const savedScroll = sessionStorage.getItem(sidebarScrollKey);
  if (savedScroll !== null) {
    sidebar.scrollTop = Number(savedScroll) || 0;
  }

  const saveSidebarScroll = () => {
    sessionStorage.setItem(sidebarScrollKey, String(sidebar.scrollTop));
  };

  sidebar.addEventListener('scroll', saveSidebarScroll, { passive: true });
  window.addEventListener('beforeunload', saveSidebarScroll);
  for (const link of links) {
    link.addEventListener('click', saveSidebarScroll);
  }
}

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
    body, headings = render_markdown(markdown, item.source.parent)
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
    return title, f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{html.escape(title)} - MCL</title>
  <link rel="stylesheet" href="{html.escape(css_href)}">
</head>
<body>
  <div class="layout">
    <aside class="sidebar">
      <div class="brand">MCL</div>
      <input class="nav-filter" data-nav-filter type="search" placeholder="Filter sections" aria-label="Filter sections">
      <nav class="nav" aria-label="Manual sections">
        {nav_html}
      </nav>
    </aside>
    <main class="content">
      <div class="doc-meta">MCL / {html.escape(title)}</div>
      {body}
      {pager}
    </main>
  </div>
  <script src="{html.escape(js_href)}"></script>
</body>
</html>
"""


def redirect_page(target: str) -> str:
    escaped = html.escape(target)
    return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="0; url={escaped}">
  <title>MCL</title>
  <link rel="canonical" href="{escaped}">
</head>
<body>
  <p><a href="{escaped}">Open MCL</a></p>
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
    if not root_index.exists() and items:
        root_index.write_text(redirect_page(items[0].href), encoding="utf-8")
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
