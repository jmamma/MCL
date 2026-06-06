/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLSTRINGS_H__
#define MCLSTRINGS_H__

#include <avr/pgmspace.h>
#include <stddef.h> // For size_t
#include <string.h> // For strncpy

/**
 * Central string repository for MCL UI text
 * All strings stored in PROGMEM to save RAM
 */

// Helper macros for PROGMEM strings
#define MCLSTR_DECLARE(name) extern const char mclstr_##name[] PROGMEM
#define MCLSTR_DEFINE(name, str) const char mclstr_##name[] PROGMEM = str

// =============================================================================
// COMMON UI STRINGS
// =============================================================================

// Common single characters / symbols
MCLSTR_DECLARE(space);
MCLSTR_DECLARE(dash);
MCLSTR_DECLARE(dash_dash_space);
MCLSTR_DECLARE(dash_space);
MCLSTR_DECLARE(plus);
MCLSTR_DECLARE(arrow_right);
MCLSTR_DECLARE(empty);
MCLSTR_DECLARE(k_space);
MCLSTR_DECLARE(colon);
MCLSTR_DECLARE(kb);

// Common labels
MCLSTR_DECLARE(hz);
MCLSTR_DECLARE(oct);
MCLSTR_DECLARE(det);
MCLSTR_DECLARE(len);
MCLSTR_DECLARE(sca);
MCLSTR_DECLARE(rec);
MCLSTR_DECLARE(lck_arrow);
MCLSTR_DECLARE(load);
MCLSTR_DECLARE(mode);
MCLSTR_DECLARE(save);
MCLSTR_DECLARE(channel_select);
MCLSTR_DECLARE(cond);
MCLSTR_DECLARE(cond_caret);
MCLSTR_DECLARE(hex_prefix); // For "0x"
MCLSTR_DECLARE(plen);
MCLSTR_DECLARE(quant);
MCLSTR_DECLARE(arp);
MCLSTR_DECLARE(rate);
MCLSTR_DECLARE(range);
MCLSTR_DECLARE(spd);
MCLSTR_DECLARE(mult);
MCLSTR_DECLARE(dep1);
MCLSTR_DECLARE(dep2);
MCLSTR_DECLARE(ofs1);
MCLSTR_DECLARE(ofs2);
MCLSTR_DECLARE(src);
MCLSTR_DECLARE(sli);
MCLSTR_DECLARE(ptc);
MCLSTR_DECLARE(seq);
MCLSTR_DECLARE(dest);
MCLSTR_DECLARE(destination);
MCLSTR_DECLARE(snd);
MCLSTR_DECLARE(no_option);
MCLSTR_DECLARE(yes_option);
MCLSTR_DECLARE(queue_option);
MCLSTR_DECLARE(record_option);
MCLSTR_DECLARE(play_option);
MCLSTR_DECLARE(ram_prefix);
MCLSTR_DECLARE(mono_option);
MCLSTR_DECLARE(s_colon);
MCLSTR_DECLARE(l_colon);
MCLSTR_DECLARE(gui_loop);
MCLSTR_DECLARE(wav_label);
MCLSTR_DECLARE(sd_card_error);
MCLSTR_DECLARE(osc_mixer);
MCLSTR_DECLARE(ply_label);
MCLSTR_DECLARE(utiming_label);
MCLSTR_DECLARE(v_label);
MCLSTR_DECLARE(select_label);
MCLSTR_DECLARE(par);
MCLSTR_DECLARE(thr);
MCLSTR_DECLARE(utim);
MCLSTR_DECLARE(route);
MCLSTR_DECLARE(parameter);
MCLSTR_DECLARE(parameter);
MCLSTR_DECLARE(lck);
MCLSTR_DECLARE(off);

// Common two-character display strings
MCLSTR_DECLARE(on);
MCLSTR_DECLARE(lat);
MCLSTR_DECLARE(ech);
MCLSTR_DECLARE(rev);
MCLSTR_DECLARE(eq);
MCLSTR_DECLARE(dyn);
MCLSTR_DECLARE(ext);
MCLSTR_DECLARE(ler);
MCLSTR_DECLARE(l1);

// Common display labels
MCLSTR_DECLARE(note);
MCLSTR_DECLARE(chromat);
MCLSTR_DECLARE(zero);
MCLSTR_DECLARE(minus);
MCLSTR_DECLARE(lock_space);
MCLSTR_DECLARE(lock_page);

// Browser page titles
MCLSTR_DECLARE(title_md_rom);
MCLSTR_DECLARE(title_sample);
MCLSTR_DECLARE(title_sound);
MCLSTR_DECLARE(title_files);
MCLSTR_DECLARE(title_project);

// Browser actions
MCLSTR_DECLARE(action_recv_bracket);
MCLSTR_DECLARE(action_save_bracket);

// File browser
MCLSTR_DECLARE(root_path);
MCLSTR_DECLARE(empty_str);

// Kit/Sound
MCLSTR_DECLARE(new_kit);
MCLSTR_DECLARE(new_kit_underscore);

// Step display
MCLSTR_DECLARE(step_label);
MCLSTR_DECLARE(step_prefix);
MCLSTR_DECLARE(zero_dash_space);
MCLSTR_DECLARE(four_dashes);
MCLSTR_DECLARE(delete_space);
MCLSTR_DECLARE(step_space);

// Page names (for grid page)
MCLSTR_DECLARE(empty_page);

// =============================================================================
// PAGE TITLES
// =============================================================================

MCLSTR_DECLARE(page_select);
MCLSTR_DECLARE(arpeggiator);
MCLSTR_DECLARE(diagnostic);
MCLSTR_DECLARE(grid);
MCLSTR_DECLARE(mixer);
MCLSTR_DECLARE(performance);

// =============================================================================
// POPUP MESSAGES - Textbox Line 1
// =============================================================================

// Status messages
MCLSTR_DECLARE(please_wait);
MCLSTR_DECLARE(mode_na);
MCLSTR_DECLARE(os_update);
MCLSTR_DECLARE(dfu_mode);
MCLSTR_DECLARE(usb_disk);
MCLSTR_DECLARE(display_mirror);
MCLSTR_DECLARE(hw_error);
MCLSTR_DECLARE(wrong_len);
MCLSTR_DECLARE(wrong_checksum);
MCLSTR_DECLARE(this_piece_of);
MCLSTR_DECLARE(machine_is_ok);
MCLSTR_DECLARE(peering);

// Action verbs (for building two-part messages)
MCLSTR_DECLARE(clear);
MCLSTR_DECLARE(copy);
MCLSTR_DECLARE(paste);
MCLSTR_DECLARE(undo);
MCLSTR_DECLARE(fill);

// Combined action verbs and common object words
MCLSTR_DECLARE(clear_step);
MCLSTR_DECLARE(copy_step);
MCLSTR_DECLARE(paste_step);
MCLSTR_DECLARE(undo_step);

// Device prefixes
MCLSTR_DECLARE(md_prefix);
MCLSTR_DECLARE(ext_prefix);

// Common object words (also declared in Line 2 section)
MCLSTR_DECLARE(step);
MCLSTR_DECLARE(scene);
MCLSTR_DECLARE(poly);
MCLSTR_DECLARE(slot);

// Track operations - Clear
MCLSTR_DECLARE(clear_md);
MCLSTR_DECLARE(clear_ext);

// Track operations - Copy
MCLSTR_DECLARE(copy_md);
MCLSTR_DECLARE(copy_ext);

// Track operations - Paste/Undo
MCLSTR_DECLARE(paste_md);
MCLSTR_DECLARE(paste_ext);
MCLSTR_DECLARE(transpose);

// Grid operations
MCLSTR_DECLARE(dice);
MCLSTR_DECLARE(slice);
MCLSTR_DECLARE(save_slots);

// Special strings
MCLSTR_DECLARE(allowed_chars);

// Save/Load operations
MCLSTR_DECLARE(save_tracks);
MCLSTR_DECLARE(save_groups);
MCLSTR_DECLARE(load_tracks);
MCLSTR_DECLARE(load_groups);

// Poly/Link messages
MCLSTR_DECLARE(lock_params);

// Upgrade
MCLSTR_DECLARE(upgrade);

// WavDesigner
MCLSTR_DECLARE(render);
MCLSTR_DECLARE(sending);

// =============================================================================
// POPUP MESSAGES - Textbox Line 2
// =============================================================================

// Common words
MCLSTR_DECLARE(tracks);
MCLSTR_DECLARE(track);
MCLSTR_DECLARE(poly_tracks);
MCLSTR_DECLARE(locks);
MCLSTR_DECLARE(lock);
MCLSTR_DECLARE(slots);
MCLSTR_DECLARE(scenes);
MCLSTR_DECLARE(ext_track);
MCLSTR_DECLARE(ext_tracks);
MCLSTR_DECLARE(link);
MCLSTR_DECLARE(update);
MCLSTR_DECLARE(full);
MCLSTR_DECLARE(machinedrum);
MCLSTR_DECLARE(monomachine);
MCLSTR_DECLARE(mirror);
MCLSTR_DECLARE(page);
MCLSTR_DECLARE(groups);

// Mask types
MCLSTR_DECLARE(trig);
MCLSTR_DECLARE(slide);
MCLSTR_DECLARE(mute);
MCLSTR_DECLARE(swing);

// =============================================================================
// LOAD/SAVE MODES
// =============================================================================

MCLSTR_DECLARE(manual);
MCLSTR_DECLARE(queue);
MCLSTR_DECLARE(auto);
MCLSTR_DECLARE(manual_short);  // "MAN"
MCLSTR_DECLARE(queue_short);   // "QUE"
MCLSTR_DECLARE(auto_short);    // "AUT"

// =============================================================================
// FILE BROWSER
// =============================================================================

MCLSTR_DECLARE(path_wav_root);
MCLSTR_DECLARE(path_syx_root);
MCLSTR_DECLARE(path_snd_root);
MCLSTR_DECLARE(suffix_wav);
MCLSTR_DECLARE(suffix_syx);
MCLSTR_DECLARE(suffix_snd);
MCLSTR_DECLARE(name_wav);
MCLSTR_DECLARE(name_sysex);
MCLSTR_DECLARE(name_snd);

// =============================================================================
// WAVEFORM NAMES (4 chars each)
// =============================================================================

typedef char wave_name_t[4];
extern const wave_name_t wave_names[] PROGMEM;
#define NUM_WAVE_NAMES 6

// =============================================================================
// ARPEGGIATOR MODE NAMES (4 chars each)
// =============================================================================

typedef char arp_name_t[4];
extern const arp_name_t arp_names[] PROGMEM;
#define NUM_ARP_NAMES 19

// =============================================================================
// SCALE NAMES (4 chars each)
// =============================================================================

typedef char scale_name_t[4];
extern const scale_name_t scale_names[] PROGMEM;
#define NUM_SCALE_NAMES 24

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * Copy a PROGMEM string to a RAM buffer
 *
 * @param dest Destination buffer
 * @param src Source PROGMEM string
 * @param max_len Maximum length to copy (including null terminator)
 */
inline void mclstr_copy_progmem(char *dest, const char *src_P, size_t max_len) {
  strncpy_P(dest, src_P, max_len - 1);
  dest[max_len - 1] = '\0';
}

/**
 * Copy a PROGMEM string from a string array to RAM buffer
 *
 * @param dest Destination buffer
 * @param src_array Source PROGMEM string array
 * @param index Index in the array
 * @param str_len Length of each string in the array
 */
inline void mclstr_copy_idx_progmem(char *dest, const char *src_array, uint8_t index, size_t str_len) {
  const char *src_P = (const char *)pgm_read_ptr(&src_array) + (index * str_len);
  strncpy_P(dest, src_P, str_len - 1);
  dest[str_len - 1] = '\0';
}

#endif /* MCLSTRINGS_H__ */
