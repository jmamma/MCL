/* Justin Mammarella jmamma@gmail.com 2018 */

#include "MCLStrings.h"

// Common labels
MCLSTR_DEFINE(space, " ");
MCLSTR_DEFINE(dash, "-- ");
MCLSTR_DEFINE(dash_dash_space, "--  ");
MCLSTR_DEFINE(dash_space, "- ");
MCLSTR_DEFINE(plus, "+");
MCLSTR_DEFINE(arrow_right, ">");
MCLSTR_DEFINE(empty, "");
MCLSTR_DEFINE(k_space, "k ");
MCLSTR_DEFINE(colon, ":");
MCLSTR_DEFINE(kb, "kB");

MCLSTR_DEFINE(hz, "Hz");
MCLSTR_DEFINE(oct, "OCT");
MCLSTR_DEFINE(det, "DET");
MCLSTR_DEFINE(len, "LEN");
MCLSTR_DEFINE(sca, "SCA");
MCLSTR_DEFINE(rec, "REC");
MCLSTR_DEFINE(lck_arrow, "LCK> ");
MCLSTR_DEFINE(lck, "LCK");
MCLSTR_DEFINE(load, "LOAD");
MCLSTR_DEFINE(mode, "MODE");
MCLSTR_DEFINE(save, "SAVE");
MCLSTR_DEFINE(voice_select, "VOICE SELECT");
MCLSTR_DEFINE(cond, "COND");
MCLSTR_DEFINE(hex_prefix, "0x");
MCLSTR_DEFINE(plen, "PLEN");
MCLSTR_DEFINE(quant, "QUANT");
MCLSTR_DEFINE(arp, "ARP");
MCLSTR_DEFINE(rate, "RATE");
MCLSTR_DEFINE(range, "RANGE");
MCLSTR_DEFINE(spd, "SPD");
MCLSTR_DEFINE(dep1, "DEP1");
MCLSTR_DEFINE(dep2, "DEP2");
MCLSTR_DEFINE(ofs1, "OFS1");
MCLSTR_DEFINE(ofs2, "OFS2");
MCLSTR_DEFINE(src, "SRC");
MCLSTR_DEFINE(dest, "DEST");
MCLSTR_DEFINE(sli, "SLI");
MCLSTR_DEFINE(ptc, "PTC");
MCLSTR_DEFINE(seq, "SEQ");

// Common two-character display strings
MCLSTR_DEFINE(on, "ON");
MCLSTR_DEFINE(off, "OFF");
MCLSTR_DEFINE(lat, "LAT");
MCLSTR_DEFINE(ech, "ECH");
MCLSTR_DEFINE(rev, "REV");
MCLSTR_DEFINE(eq, "EQ");
MCLSTR_DEFINE(dyn, "DYN");
MCLSTR_DEFINE(ext, "EXT");
MCLSTR_DEFINE(ler, "LER");
MCLSTR_DEFINE(l1, "L1");

// Common display labels
MCLSTR_DEFINE(note, "NOTE");
MCLSTR_DEFINE(chromat, "CHROMAT");
MCLSTR_DEFINE(zero, "0");
MCLSTR_DEFINE(minus, "-");
MCLSTR_DEFINE(lock_space, "LOCK  ");
MCLSTR_DEFINE(step_prefix, "S");

// Common labels
MCLSTR_DEFINE(zero_dash_space, "00 - ");
MCLSTR_DEFINE(four_dashes, "----");
MCLSTR_DEFINE(delete_space, "Delete");
MCLSTR_DEFINE(step_space, "STEP");

// Common labels
MCLSTR_DEFINE(destination, "DESTINATION");
MCLSTR_DEFINE(snd, "SND");

// Common labels
MCLSTR_DEFINE(no_option, "NO");
MCLSTR_DEFINE(yes_option, "YES");

// Common labels
MCLSTR_DEFINE(queue_option, " [Queue]");
MCLSTR_DEFINE(record_option, " [Record]");
MCLSTR_DEFINE(play_option, " [Play]");
MCLSTR_DEFINE(ram_prefix, "RAM ");
MCLSTR_DEFINE(mono_option, "MONO");
MCLSTR_DEFINE(s_colon, " S:");
MCLSTR_DEFINE(l_colon, " L:");

// Common labels
MCLSTR_DEFINE(gui_loop, "GUI loop");

// Common labels
MCLSTR_DEFINE(wav_label, "WAV");

// Common labels
MCLSTR_DEFINE(sd_card_error, "SD CARD ERROR :-(");

// Common labels
MCLSTR_DEFINE(osc_mixer, "OSC MIXER");

// Common labels
MCLSTR_DEFINE(ply_label, "PLY");

// Common labels
MCLSTR_DEFINE(utiming_label, "uTIMING: ");

// Common labels
MCLSTR_DEFINE(v_label, "V");

// Common labels
MCLSTR_DEFINE(select_label, "SELECT");
MCLSTR_DEFINE(par, "PAR");
MCLSTR_DEFINE(thr, "THR");
MCLSTR_DEFINE(utim, "UTIM");
MCLSTR_DEFINE(route, "ROUTE");
MCLSTR_DEFINE(parameter, "PARAMETER");

// =============================================================================
// Browser page titles
MCLSTR_DEFINE(title_md_rom, "MD ROM");
MCLSTR_DEFINE(title_sample, "SAMPLE");
MCLSTR_DEFINE(title_sound, "SOUND");
MCLSTR_DEFINE(title_files, "Files");

// Browser actions
MCLSTR_DEFINE(action_recv_bracket, "[ RECV ]");
MCLSTR_DEFINE(action_save_bracket, "[ SAVE ]");

// File browser
MCLSTR_DEFINE(root_path, "/");
MCLSTR_DEFINE(empty_str, "");

// Kit/Sound
MCLSTR_DEFINE(new_kit, "NEW KIT");
MCLSTR_DEFINE(new_kit_underscore, "NEW_KIT");
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
MCLSTR_DEFINE(this_piece_of, "THIS PIECE OF");
MCLSTR_DEFINE(machine_is_ok, "MACHINE IS O.K.");
MCLSTR_DEFINE(peering, "Peering...");

// Action verbs (for building two-part messages)
MCLSTR_DEFINE(clear, "CLEAR");
MCLSTR_DEFINE(copy, "COPY");
MCLSTR_DEFINE(paste, "PASTE");
MCLSTR_DEFINE(undo, "UNDO");
MCLSTR_DEFINE(fill, "FILL");

// Combined action verbs and common object words
MCLSTR_DEFINE(clear_step, "CLEAR STEP");
MCLSTR_DEFINE(copy_step, "COPY STEP");
MCLSTR_DEFINE(paste_step, "PASTE STEP");
MCLSTR_DEFINE(undo_step, "UNDO STEP");

// Device prefixes
MCLSTR_DEFINE(md_prefix, "MD");
MCLSTR_DEFINE(ext_prefix, "EXT");

// Common object words (also declared in Line 2 section)
MCLSTR_DEFINE(step, "STEP");
MCLSTR_DEFINE(scene, "SCENE");
MCLSTR_DEFINE(poly, "POLY");
MCLSTR_DEFINE(slot, "SLOT");

// Track operations - Clear
MCLSTR_DEFINE(clear_md, "CLEAR MD");
MCLSTR_DEFINE(clear_ext, "CLEAR EXT");

// Track operations - Copy
MCLSTR_DEFINE(copy_md, "COPY MD");
MCLSTR_DEFINE(copy_ext, "COPY EXT");

// Track operations - Paste/Undo
MCLSTR_DEFINE(paste_md, "PASTE MD");
MCLSTR_DEFINE(paste_ext, "PASTE EXT");
MCLSTR_DEFINE(transpose, "TRANSPOSE");

// Grid operations
MCLSTR_DEFINE(dice, "DICE");
MCLSTR_DEFINE(slice, "SLICE");
MCLSTR_DEFINE(save_slots, "SAVE SLOTS");

// Special strings
MCLSTR_DEFINE(allowed_chars, "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_&@-=!");

// Save/Load operations
MCLSTR_DEFINE(save_tracks, "SAVE TRACKS");
MCLSTR_DEFINE(save_groups, "SAVE GROUPS");
MCLSTR_DEFINE(load_tracks, "LOAD TRACKS");
MCLSTR_DEFINE(load_groups, "LOAD GROUPS");

// Poly/Link messages
MCLSTR_DEFINE(lock_params, "LOCK PARAMS");

// Upgrade
MCLSTR_DEFINE(upgrade, "UPGRADE");

// WavDesigner
MCLSTR_DEFINE(render, "Render");
MCLSTR_DEFINE(sending, "Sending..");

// =============================================================================
// POPUP MESSAGES - Textbox Line 2
// =============================================================================

// Common words
MCLSTR_DEFINE(tracks, "TRACKS");
MCLSTR_DEFINE(track, "TRACK");
MCLSTR_DEFINE(poly_tracks, "POLY TRACKS");
MCLSTR_DEFINE(locks, "LOCKS");
MCLSTR_DEFINE(lock, "LOCK");
MCLSTR_DEFINE(slots, "SLOTS");
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
MCLSTR_DEFINE(name_snd, "SND");

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
