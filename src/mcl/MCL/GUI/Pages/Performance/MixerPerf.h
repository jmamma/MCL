/* Justin Mammarella jmamma@gmail.com 2026 */

#pragma once

#include <stdint.h>

class Encoder;
class MidiDevice;

namespace MixerPerf {

bool available(MidiDevice *device);
uint8_t mixer_param_for_encoder(uint8_t encoder_idx);
void load_locks(Encoder *const *encoders, uint8_t locks[4][4],
                uint8_t state);
bool handle_preview_lock_edits(Encoder *const *encoders,
                               uint8_t locks[4][4],
                               uint8_t preview_mute_set, bool notes_on);
void func_enc_check();
void encoder_send();
bool should_show_encoder(Encoder *encoder, uint16_t &used_clock,
                         bool activity, uint16_t timeout);
uint8_t display_value(Encoder *encoder, uint8_t locks[4][4],
                      uint8_t preview_mute_set, uint8_t encoder_idx,
                      bool &highlight);
void clear_scenes(Encoder *encoder);
void scene_autofill(Encoder *encoder);
bool handle_preview_lock_button(Encoder *encoder, uint8_t locks[4][4],
                                uint8_t preview_mute_set,
                                uint8_t encoder_idx, bool pressed);

} // namespace MixerPerf
