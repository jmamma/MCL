# MCL Manual Source

The editable manual now lives in Markdown under this directory. The generated GitHub Pages site is built from these files by `tools/docs/build_manual_site.py`.

Useful commands from the `MCL` repository root:

```sh
python3 tools/docs/validate_manual.py
python3 tools/docs/build_manual_site.py
```

The legacy LaTeX repository is no longer the source of truth. Use `tools/docs/convert_latex_manual.py` only for migration archaeology or for recreating the initial Markdown import.
