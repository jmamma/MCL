# RAM Page

The RAM pages automate Machinedrum RAM record and RAM playback machines. They set up the MD machines, track links and transition timing so RAM capture/playback can be performed from MCL.

Open them with:

![ram1](../assets/images/ram1.png)

| Page | Shortcut | Tracks used |
| --- | --- | --- |
| RAM 1 | **[Bank Group] + [Trig 15]** | Track 15 in mono mode. |
| RAM 2 | **[Bank Group] + [Trig 16]** | Track 16 in mono mode. |

In stereo/link mode, RAM 1 controls the linked track pair 15 and 16.

## RAM Link Mode

RAM mode is configured from the Machinedrum configuration page:

`CONFIG > MD > RAM LINK`

| Value | Behavior |
| --- | --- |
| `MONO` | RAM 1 and RAM 2 act independently on tracks 15 and 16. Source can be `INT`, `A` or `B`. |
| `STEREO` / `LINK` | Tracks 15 and 16 are linked as a stereo pair. Main output records as left/right, and external input records as A/B. |

This option moved from the old Page Setup area into the Machinedrum configuration page.

## Controls

| Control | Function |
| --- | --- |
| `Encoder 1` | Recording source. |
| `Encoder 2` | Dice mode. |
| `Encoder 3` | Number of slices. |
| `Encoder 4` | Record/playback length in sequencer steps. |
| MD **[Yes]** or MCL **[Save/No]** | Queue RAM recording. |
| MD **[No]** or MCL **[Load/Yes]** | Queue RAM playback or apply normal slicing. |
| **[Global]** | Apply dice slicing. |

Pressing an encoder button returns to the Grid Page.

## Recording

1. Start the Machinedrum pattern or make sure transport is ready.
2. Leave tracks 15 and 16 free; RAM pages overwrite their machine and sequence data.
3. Choose the source with `Encoder 1`.
4. Set the length with `Encoder 4`.
5. Press MD **[Yes]** or MCL **[Save/No]** to queue recording.

The display shows `QUEUE` until the quantized transition point, then `RECORD`. Recording continues until playback is queued.

![ram1 action](../assets/images/ram1_action.png)

## Playback

Press MD **[No]** or MCL **[Load/Yes]** to queue RAM playback. If the current RAM playback track has not been set up yet, MCL creates the RAM playback machine, sequence step and track link first.

The display shows `QUEUE` until the transition point, then `PLAY`.

## Mono Mode

In `MONO`, the pages are independent:

| Page | RAM record/play machine |
| --- | --- |
| RAM 1 | Track 15, RAM-R1/RAM-P1. |
| RAM 2 | Track 16, RAM-R2/RAM-P2. |

Source `INT` samples the Machinedrum mix. Sources `A` and `B` sample the external inputs.

## Stereo / Link Mode

In `STEREO` / `LINK`, track 15 and 16 work as a pair. MCL creates linked RAM machines, pans them left/right and keeps compatible playback parameter changes synchronized between the pair.

For main-output recording, track 15 records the left side and track 16 records the right side. For external-input recording, track 15 records input A and track 16 records input B.

## Slice And Dice

Slicing divides the RAM playback into multiple slices by writing start/end locks for the slice trigs.

| Action | Result |
| --- | --- |
| Set slices with `Encoder 3`, then press MD **[No]** or MCL **[Load/Yes]** | Apply normal slice playback. |
| Set dice mode with `Encoder 2`, then press **[Global]** | Apply dice slice playback. |

Dice modes alter slice order or direction:

| Mode | Behavior |
| --- | --- |
| `0` | Reverse slices. |
| `1..4` | Reverse every nth slice. |
| `5` | Play slices in reverse order. |
| `6` | Play slices in random order. |

The RAM page also keeps the RAM sequencer track unmuted for RAM transitions, avoiding the older failure mode where RAM playback or recording was set up but could not be heard.
