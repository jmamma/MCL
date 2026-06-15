# MCL Sequencer

The MCL sequencer stores musical events inside grid tracks. A track can be a classic Machinedrum step track, a TBD step track, or an external MIDI-style track, depending on the devices assigned to Grid X and Grid Y.

## Track Families

| Track family | Typical use | Main editor |
| --- | --- | --- |
| Primary step tracks | Machinedrum or TBD-style per-step sequencing. | Step Editor |
| External MIDI tracks | Polyphonic notes and automation for MIDI, A4, MNM or generic devices. | PianoRoll Editor |
| Chromatic/voice tracks | Live pitch input, arpeggiation and polyphonic Machinedrum voice groups. | Chromatic and Polyphony pages |
| State tracks | Performance, route, FX, tempo and other page state. | The related page |

Classic MegaCommand projects commonly use 16 primary tracks and six external tracks. The configured grid device decides what the primary and secondary tracks control.

## Primary Step Tracks

Primary step tracks are edited from the Step Editor and are designed for 16-step page editing.

| Feature | Details |
| --- | --- |
| Track count | 16 primary step tracks where the active device supports them. |
| Length | Up to 64 steps per track. |
| Speeds | `1x`, `2x`, `3/2x`, `3/4x`, `1/2x`, `1/4x`, `1/8x`. |
| Step data | Trigs, mutes, swing mask, slides, pitch, conditions and microtiming. |
| Locks | Parameter locks per step, with device-specific lock capacity. |
| Swing | Per-track swing amount, stored with the track. |
| Arp and LFO | Each track has its own arpeggiator and LFO settings. |

TBD step tracks use the current Step Editor behavior, including microtiming, retrigs, fill conditions and parameter locks for the controls exposed by the active TBD sound.

## External MIDI Tracks

External MIDI-style tracks are edited from the PianoRoll and automation views.

| Feature | Details |
| --- | --- |
| Track count | Six classic external tracks where enabled. |
| Length | Up to 128 steps per track. |
| Notes | Polyphonic note events, velocity, note length and microtiming. |
| Automation | CC, NRPN, RPN, pitch bend, channel pressure, poly pressure and program change locks where supported. |
| Per-step behavior | Conditions, mute state, slide/glide and live recording. |
| Route channels | Normal MIDI channels `1..16`, plus MD route targets for Machinedrum polyphonic voice groups. |
| Arp and LFO | Per-track arpeggiator and LFO data is stored with the track. |

TBD and desktop/browser versions use full MIDI sequencer tracks for Generic MIDI, A4/MNM-style secondary devices and TBD as a secondary device. MegaCommand and MegaCMD use the classic external track path.

When Machinedrum is the primary grid device, an external MIDI track can route to a Machinedrum polyphonic voice group instead of a MIDI output channel. In the external track's `CHANNEL` setting, scroll past MIDI channel `16` to choose `MD1` through `MD16`. The selected `MD` target uses the Polyphony Page voice allocator for notes, and CC automation or parameter locks can control the addressed Machinedrum track parameters.

## Conditional Trigs

Trig conditions use the same condition model across supported step sequencer engines.

| Label | Meaning |
| --- | --- |
| `---` | Always play. |
| `10%`, `25%`, `33%`, `50%`, `66%`, `75%`, `90%` | Probability that the trig plays. |
| `1SH` | One-shot; plays once, then must be rearmed. |
| `1ST` | First cycle/pass only. |
| `!1S` | Not first cycle/pass. |
| `FIL` | Plays when fill is active for the track. |
| `!FL` | Plays when fill is not active for the track. |
| `PRE` | Plays when the previous trig condition fired. |
| `!PR` | Plays when the previous trig condition did not fire. |
| `NEI` | Plays when the neighbouring previous track fired. |
| `!NE` | Plays when the neighbouring previous track did not fire. |
| `x:y` | Iterator condition; for example `2:4` plays on the second pass of a four-pass cycle. |

When a condition is shown with a `^` or `+` marker, the condition also gates related lock/slide behavior instead of only gating the trig.

## Mutes And Fill

MCL stores track mutes and fill states as sequencer state.

| State | Use |
| --- | --- |
| Track mute | Silences a track and is stored with sequencer tracks. |
| Step mute | Disables a specific step without deleting the step data. |
| Fill state | Enables steps using `FIL` conditions and disables steps using `!FL` conditions. |

Track fill states can be edited from the Mixer Page, and enhanced Machinedrum mode can toggle the Mute Menu between mute and fill editing.

## Swing

Each primary step track has its own swing percentage. The Swing edit mask chooses which steps are delayed by the swing amount.

The default swing mask affects off-beat steps. When a Machinedrum pattern's global swing is set to affect all steps, the per-step swing mask is hidden because editing it would not change playback.

## Microtiming

Microtiming moves a step, note or automation event slightly earlier or later relative to the grid.

Negative values play earlier, positive values play later. The timing offset is saved with the step or event and is preserved through save/load, copy/paste and supported project migration.

## Live Record

Live record can capture trigs, notes, pitch input and parameter changes depending on the active page and track type. Start live record with the Machinedrum-style **[Rec]** + **[Play]** gesture.

Recorded MIDI and device control data is quantized according to the current sequencer quantization setting where applicable.

## Stored Sequencer Data

Saving a grid slot stores the sequencer data for that slot.

| Stored with the track | Notes |
| --- | --- |
| Sequence events | Trigs, notes, locks, automation and timing. |
| Track settings | Length, speed, swing, mute mask and conditions. |
| Arpeggiator settings | Stored per track. |
| LFO settings | Stored per track. |
| Sound/device state | Stored where supported, unless the slot's `SOUND` option is off during load. |

Older projects are migrated on first load where supported so legacy conditions, mutes, microtiming and LFO data map into the current project format.
