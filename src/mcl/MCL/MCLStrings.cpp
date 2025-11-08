/* Justin Mammarella jmamma@gmail.com 2018 */

#include "MCLStrings.h"

// =============================================================================
// COMMON UI STRINGS
// =============================================================================

MCLSTR_DEFINE(space, " ");
MCLSTR_DEFINE(dash, "--");
MCLSTR_DEFINE(plus, "+");
MCLSTR_DEFINE(arrow_right, ">");
MCLSTR_DEFINE(empty, "");

MCLSTR_DEFINE(hz, "Hz");
MCLSTR_DEFINE(oct, "OCT");
MCLSTR_DEFINE(det, "DET");
MCLSTR_DEFINE(len, "LEN");
MCLSTR_DEFINE(sca, "SCA");
MCLSTR_DEFINE(rec, "REC");
MCLSTR_DEFINE(lck_arrow, "LCK> ");

// =============================================================================
// PAGE TITLES
// =============================================================================

MCLSTR_DEFINE(page_select, "PAGE SELECT");
MCLSTR_DEFINE(arpeggiator, "ARPEGGIATOR: T");
MCLSTR_DEFINE(diagnostic, "DIAGNOSTIC");
MCLSTR_DEFINE(grid, "GRID");
MCLSTR_DEFINE(mixer, "MIXER");
MCLSTR_DEFINE(performance, "PERFORMANCE");

// =============================================================================
// POPUP MESSAGES - Textbox Line 1
// =============================================================================

// Status messages
MCLSTR_DEFINE(please_wait, "PLEASE WAIT");
MCLSTR_DEFINE(mode_na, "MODE N/A");
MCLSTR_DEFINE(os_update, "OS UPDATE");
MCLSTR_DEFINE(dfu_mode, "DFU MODE");
MCLSTR_DEFINE(usb_disk, "USB DISK");
MCLSTR_DEFINE(display_mirror, "DISPLAY ");
MCLSTR_DEFINE(hw_error, "HW ERROR:");
MCLSTR_DEFINE(wrong_len, "WRONG LEN");
MCLSTR_DEFINE(wrong_checksum, "WRONG CHECKSUM");

// Track operations - Clear
MCLSTR_DEFINE(clear_md, "CLEAR MD ");
MCLSTR_DEFINE(clear_ext, "CLEAR EXT ");
MCLSTR_DEFINE(clear, "CLEAR ");
MCLSTR_DEFINE(clear_page, "CLEAR PAGE");
MCLSTR_DEFINE(clear_track_word, "CLEAR TRACK");
MCLSTR_DEFINE(clear_scenes, "CLEAR SCENES");
MCLSTR_DEFINE(fill_scenes, "FILL SCENES");

// Track operations - Copy
MCLSTR_DEFINE(copy_md, "COPY MD ");
MCLSTR_DEFINE(copy_ext, "COPY EXT ");
MCLSTR_DEFINE(copy, "COPY ");
MCLSTR_DEFINE(copy_page, "COPY PAGE");
MCLSTR_DEFINE(copy_track, "COPY TRACK");

// Track operations - Paste/Undo
MCLSTR_DEFINE(paste_md, "PASTE MD ");
MCLSTR_DEFINE(paste_ext, "PASTE EXT ");
MCLSTR_DEFINE(paste, "PASTE");
MCLSTR_DEFINE(paste_page, "PASTE PAGE");
MCLSTR_DEFINE(paste_track, "PASTE TRACK");
MCLSTR_DEFINE(undo, "UNDO");
MCLSTR_DEFINE(undo_track, "UNDO TRACK");
MCLSTR_DEFINE(undo_page, "UNDO PAGE");
MCLSTR_DEFINE(paste_md_tracks, "PASTE MD ");
MCLSTR_DEFINE(paste_ext_tracks, "PASTE EXT TRACKS");
MCLSTR_DEFINE(undo_tracks, "UNDO ");
MCLSTR_DEFINE(undo_ext_tracks, "UNDO EXT TRACKS");
MCLSTR_DEFINE(paste_ext_track, "PASTE EXT TRACK");
MCLSTR_DEFINE(undo_ext_track, "UNDO EXT TRACK");
MCLSTR_DEFINE(copy_step, "COPY STEP");
MCLSTR_DEFINE(undo_step, "UNDO STEP");
MCLSTR_DEFINE(paste_step, "PASTE STEP");
MCLSTR_DEFINE(transpose, "TRANSPOSE");
MCLSTR_DEFINE(clear_track, "CLEAR TRACK");
MCLSTR_DEFINE(clear_poly_tracks, "CLEAR POLY TRACKS");
MCLSTR_DEFINE(clear_step, "CLEAR STEP: ");
MCLSTR_DEFINE(clear_step_word, "CLEAR STEP");
MCLSTR_DEFINE(clear_scenes_word, "CLEAR SCENES");
MCLSTR_DEFINE(clear_ext_tracks, "CLEAR EXT TRACKS");
MCLSTR_DEFINE(clear_ext_track, "CLEAR EXT TRACK");

// Grid operations
MCLSTR_DEFINE(slot, "SLOT ");
MCLSTR_DEFINE(save_slots, "SAVE SLOTS");
MCLSTR_DEFINE(dice, "DICE");
MCLSTR_DEFINE(slice, "SLICE");

// Save/Load operations
MCLSTR_DEFINE(save_tracks, "SAVE TRACKS");
MCLSTR_DEFINE(save_groups, "SAVE GROUPS");
MCLSTR_DEFINE(load_tracks, "LOAD TRACKS");
MCLSTR_DEFINE(load_groups, "LOAD GROUPS");

// Poly/Link messages
MCLSTR_DEFINE(poly, "POLY-");
MCLSTR_DEFINE(lock_params, "LOCK PARAMS ");

// Upgrade
MCLSTR_DEFINE(upgrade, "UPGRADE ");

// WavDesigner
MCLSTR_DEFINE(render, "Render");
MCLSTR_DEFINE(sending, "Sending..");

// =============================================================================
// POPUP MESSAGES - Textbox Line 2
// =============================================================================

// Common words
MCLSTR_DEFINE(tracks, "TRACKS");
MCLSTR_DEFINE(track, "TRACK");
MCLSTR_DEFINE(locks, "LOCKS");
MCLSTR_DEFINE(lock, "LOCK");
MCLSTR_DEFINE(slots, "SLOTS");
MCLSTR_DEFINE(slot_word, "SLOT");
MCLSTR_DEFINE(link, "LINK");
MCLSTR_DEFINE(update, "UPDATE");
MCLSTR_DEFINE(full, "FULL");
MCLSTR_DEFINE(machinedrum, "MACHINEDRUM");
MCLSTR_DEFINE(monomachine, "MONOMACHINE");
MCLSTR_DEFINE(mirror, "MIRROR");
MCLSTR_DEFINE(page, "PAGE");
MCLSTR_DEFINE(groups, "GROUPS");

// Mask types
MCLSTR_DEFINE(trig, "TRIG");
MCLSTR_DEFINE(slide, "SLIDE");
MCLSTR_DEFINE(mute, "MUTE");

// =============================================================================
// LOAD/SAVE MODES
// =============================================================================

MCLSTR_DEFINE(manual, "MANUAL");
MCLSTR_DEFINE(queue, "QUEUE");
MCLSTR_DEFINE(auto, "AUTO");
MCLSTR_DEFINE(manual_short, "MAN");
MCLSTR_DEFINE(queue_short, "QUE");
MCLSTR_DEFINE(auto_short, "AUT");

// =============================================================================
// FILE BROWSER
// =============================================================================

MCLSTR_DEFINE(path_wav_root, "/Samples/WAV");
MCLSTR_DEFINE(path_syx_root, "/Samples/SYX");
MCLSTR_DEFINE(path_snd_root, "/Sounds");
MCLSTR_DEFINE(suffix_wav, ".wav");
MCLSTR_DEFINE(suffix_syx, ".syx");
MCLSTR_DEFINE(suffix_snd, ".snd");
MCLSTR_DEFINE(name_wav, "WAV");
MCLSTR_DEFINE(name_sysex, "SYSEX");

// =============================================================================
// WAVEFORM NAMES (4 chars each)
// =============================================================================

const wave_name_t wave_names[] PROGMEM = {
  "--", "SIN", "TRI", "PUL", "SAW", "USR"
};

// =============================================================================
// ARPEGGIATOR MODE NAMES (4 chars each)
// =============================================================================

const arp_name_t arp_names[] PROGMEM = {
  "UP",  "DWN", "UD",  "DU",  "UND", "DNU", "CNV", "DIV", "CND",
  "PU",  "PD",  "TU",  "TD",  "UPP", "DP",  "U2",  "D2",  "RND", "RN2"
};

// =============================================================================
// SCALE NAMES (4 chars each)
// =============================================================================

const scale_name_t scale_names[] PROGMEM = {
  "---", "MAJ", "DOR", "PHR", "LYD", "MIX", "MIN", "LOC",
  "mHA", "mME", "MPE", "mPE", "sPE", "ISS", "BLU", "MBP",
  "DBP", "mBP", "MA",  "MIA", "MM7", "Mm7", "mm7", "M79"
};
