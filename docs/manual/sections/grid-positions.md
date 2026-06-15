# Grid Positions

Grid positions describe where a slot lives in a project and how MCL maps physical controls to rows and columns.

## Address Format

A complete grid position has three parts:

```text
Grid : Column BankRow
```

For example, `X:05 A01` means:

| Part | Meaning |
| --- | --- |
| `X` | Grid X. |
| `05` | Column/slot 5 in the visible grid. |
| `A01` | Bank A, row 1. |

The same position can still be described conceptually as grid, row and column; the screen shows the column before the bank/row label.

## Rows And Banks

Each project has 128 rows.

| Bank | Rows |
| --- | --- |
| A | 1-16 |
| B | 17-32 |
| C | 33-48 |
| D | 49-64 |
| E | 65-80 |
| F | 81-96 |
| G | 97-112 |
| H | 113-128 |

The Machinedrum bank and trig keys can be used as row shortcuts for pattern-style loading.

## Columns And Slots

Each grid has 16 visible columns. A physical trig key maps to the matching visible column when selecting slots from the Grid, Save or Load pages.

Internally, MCL tracks both grids as a 32-slot logical row:

| Logical range | Visible range |
| --- | --- |
| Slots 1-16 | Grid X columns 1-16. |
| Slots 17-32 | Grid Y columns 1-16. |

Most users do not need the internal slot numbers, but they explain why load destinations and group selections can span both grids.

## Moving Around

On the Grid Page:

| Control | Action |
| --- | --- |
| Encoder 1 | Move horizontally through columns. |
| Encoder 2 | Move vertically through rows. |
| **[Left]** / **[Right]** | Move across columns. |
| **[Up]** / **[Down]** | Move across rows. |
| **[Function]** + arrow | Move faster where supported. |
| **[Scale]** | Toggle the active grid between X and Y. |

When the cursor reaches the edge of the visible area, the page scrolls.

If `CONFIG > SYSTEM > GRID ENCOD` is set to `PERF`, the Grid Page encoders act as performance controllers instead. Use the arrow keys for grid navigation in that mode.

## Active Row

The active row is the row currently being played or targeted by loading. The grid can show queued or active slots differently depending on load mode.

With bank/trig row loading, the selected row becomes part of the load workflow. In queue mode, multiple selected rows can form a repeating queue.

## Row Names

Rows can be renamed from the Slot Menu with `RENAME`.

Row names are shared across Grid X and Grid Y. Clearing every slot in a row can clear the row name when the row becomes inactive.

## Desktop Mouse Support

Desktop-capable builds can also use mouse actions:

| Action | Result |
| --- | --- |
| Click a cell | Move the grid cursor to that slot. |
| Double-click a cell | Open the Load workflow for that slot. |
| Right-click a cell | Open the Slot Menu. |
| Mouse wheel | Scroll rows. |
| Mouse wheel in Slot Menu | Adjust the selected range. |
