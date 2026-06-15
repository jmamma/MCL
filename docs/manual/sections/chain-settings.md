# Chains And Queues

Chains and queues let MCL move between grid rows or slots automatically while the project plays.

## Manual, Auto And Queue

MCL has three performance load modes:

| Mode | Best for | Behavior |
| --- | --- | --- |
| Manual (`MAN`) | Direct performance changes | Loads the selected slots at the next transition interval and clears existing queues for those slots. |
| Auto (`AUT`) | Pre-programmed arrangements | Loads a slot, then follows its `LOOP` and `JUMP` values. |
| Queue (`QUE`) | Improvised row chains | Adds selected rows or slots to a repeating queue. |

Use **[Bank A]**, **[Bank B]** and **[Bank C]** from Load or the Slot Menu to switch modes quickly.

## Auto Mode

Auto mode uses each slot's link settings:

| Slot setting | Meaning |
| --- | --- |
| `LOOP` | How many times the slot should repeat. |
| `JUMP` | Which row should load after the loop count is reached. |

If `LOOP` is `0`, the slot does not auto-advance.

Auto mode is useful for planned song structures where each column can follow its own row path.

## Queue Mode

Queue mode creates a repeating queue per slot/column.

Rows added in Queue mode behave as follows:

- each selected slot/column gets its own queue
- queued rows repeat in the order they were added
- up to eight queue links can be active for a column
- queued slots are shown differently on the Grid Page

Queue mode is useful for live arrangement because rows can be chained without editing the saved `LOOP` and `JUMP` data.

## Queue Length

Queue length controls how long a queued item plays before advancing.

| Queue length | Behavior |
| --- | --- |
| `-` / 1 | Use the slot's saved length. |
| 2-64 | Override the queued duration. |

Use queue length when a saved track is short but should stay active for a longer section.

## Quantization

Quantization controls when the next load occurs.

| Quantization | Result |
| --- | --- |
| `-` / 1 | Use the destination slot's own timing. |
| 2-64 | Wait for that step interval before loading. Normal encoder turns step through powers of two; holding the encoder button allows intermediate values. |

Higher quantization values are safer for full-row changes. Lower values are useful for fast slot-level changes.

## Bank/Trig Row Chains

The fastest way to create a row chain is from the Grid Page:

1. Hold a bank key.
2. Press a trig for the first row.
3. Press additional trigs to add rows to the chain.
4. Release to return to the Grid Page.

MCL temporarily uses Queue mode for the chained rows, then restores the previous load mode.

## Slot Menu Range Loading

The Slot Menu can also load a rectangular range.

If the range covers more than one row, MCL temporarily uses Queue mode so the selected rows advance as a queue.
