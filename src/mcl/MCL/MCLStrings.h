/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLSTRINGS_H__
#define MCLSTRINGS_H__

#include <avr/pgmspace.h>

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
MCLSTR_DECLARE(plus);
MCLSTR_DECLARE(arrow_right);
MCLSTR_DECLARE(empty);

// Common labels
MCLSTR_DECLARE(hz);
MCLSTR_DECLARE(oct);
MCLSTR_DECLARE(det);
MCLSTR_DECLARE(len);
MCLSTR_DECLARE(sca);
MCLSTR_DECLARE(rec);
MCLSTR_DECLARE(lck_arrow);

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

// Record
MCLSTR_DECLARE(rec);

// Track operations - Clear
MCLSTR_DECLARE(clear_md);
MCLSTR_DECLARE(clear_ext);
MCLSTR_DECLARE(clear);
MCLSTR_DECLARE(clear_page);
MCLSTR_DECLARE(clear_track);
MCLSTR_DECLARE(clear_scenes);
MCLSTR_DECLARE(fill_scenes);

// Track operations - Copy
MCLSTR_DECLARE(copy_md);
MCLSTR_DECLARE(copy_ext);
MCLSTR_DECLARE(copy);
MCLSTR_DECLARE(copy_page);
MCLSTR_DECLARE(copy_track);

// Track operations - Paste/Undo
MCLSTR_DECLARE(paste_md);
MCLSTR_DECLARE(paste_ext);
MCLSTR_DECLARE(paste);
MCLSTR_DECLARE(paste_page);
MCLSTR_DECLARE(paste_track);
MCLSTR_DECLARE(undo);
MCLSTR_DECLARE(undo_track);
MCLSTR_DECLARE(undo_page);
MCLSTR_DECLARE(paste_md_tracks);
MCLSTR_DECLARE(paste_ext_tracks);
MCLSTR_DECLARE(undo_tracks);
MCLSTR_DECLARE(undo_ext_tracks);
MCLSTR_DECLARE(paste_ext_track);
MCLSTR_DECLARE(undo_ext_track);
MCLSTR_DECLARE(copy_step);
MCLSTR_DECLARE(undo_step);
MCLSTR_DECLARE(paste_step);
MCLSTR_DECLARE(transpose);
MCLSTR_DECLARE(clear_poly_tracks);
MCLSTR_DECLARE(clear_step);
MCLSTR_DECLARE(clear_step_word);
MCLSTR_DECLARE(clear_scenes_word);
MCLSTR_DECLARE(clear_ext_tracks);
MCLSTR_DECLARE(clear_ext_track);

// Grid operations
MCLSTR_DECLARE(slot);
MCLSTR_DECLARE(dice);
MCLSTR_DECLARE(slice);

// Save/Load operations
MCLSTR_DECLARE(save_tracks);
MCLSTR_DECLARE(save_groups);
MCLSTR_DECLARE(load_tracks);
MCLSTR_DECLARE(load_groups);

// Poly/Link messages
MCLSTR_DECLARE(poly);
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
MCLSTR_DECLARE(locks);
MCLSTR_DECLARE(lock);
MCLSTR_DECLARE(slots);
MCLSTR_DECLARE(slot_word);
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
