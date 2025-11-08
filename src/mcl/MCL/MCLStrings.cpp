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

// Action verbs (for building two-part messages)
MCLSTR_DEFINE(clear_word, "CLEAR");
MCLSTR_DEFINE(copy_word, "COPY");
MCLSTR_DEFINE(paste_word, "PASTE");
MCLSTR_DEFINE(undo_word, "UNDO");
MCLSTR_DEFINE(save_word, "SAVE");
MCLSTR_DEFINE(load_word, "LOAD");
MCLSTR_DEFINE(fill_word, "FILL");

// Device prefixes
MCLSTR_DEFINE(md_prefix, "MD");
MCLSTR_DEFINE(ext_prefix, "EXT");

// Common object words (also declared in Line 2 section)
MCLSTR_DEFINE(step_word, "STEP");
MCLSTR_DEFINE(scene_word, "SCENE");
MCLSTR_DEFINE(poly_word, "POLY");

// Track operations - Clear
MCLSTR_DEFINE(clear_md, "CLEAR MD ");
MCLSTR_DEFINE(clear_ext, "CLEAR EXT ");

// Track operations - Copy
MCLSTR_DEFINE(copy_md, "COPY MD ");
MCLSTR_DEFINE(copy_ext, "COPY EXT ");

// Track operations - Paste/Undo
MCLSTR_DEFINE(paste_md, "PASTE MD ");
MCLSTR_DEFINE(transpose, "TRANSPOSE");

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
MCLSTR_DEFINE(scenes, "SCENES");
MCLSTR_DEFINE(ext_track, "EXT TRACK");
MCLSTR_DEFINE(ext_tracks, "EXT TRACKS");
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
