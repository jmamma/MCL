#ifndef A4MESSAGES_H__
#define A4MESSAGES_H__

#include "A4Pattern.h"
#include "helpers.h"
#include "Elektron.h"

extern uint8_t a4_sysex_hdr[5];

/**
 * \addtogroup A4 Elektron MachineDrum
 *
 * @{
 *
 * \addtogroup md_sysex MachineDrum Sysex Messages
 *
 * @{
 **/

/**
 * \addtogroup md_sysex_global MachineDrum Global Message
 * @{
 **/

/**
 * This class stores the global settings of the machinedrum, which comprises:
 *
 * - MIDI channel and clock and trigger settings
 * - MachineDrum clock configuration
 * - routing of the individual tracks to the audio outputs
 * - gate, sensitivity and levels of the audio inputs
 **/
class A4Global {
  /**
   * \addtogroup md_sysex_global
   * @{
   **/

public:
  /* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES. */

  /** Original position of the global inside the A4 (0 to 7). **/
  uint8_t origPosition;
  /** Stores the audio output for each track. **/
  uint8_t drumRouting[16];
  /** Stores the MIDI pitch that triggers each track. **/
  int8_t drumMapping[16];
  /** Stores the MIDI pitch that triggers each pattern. **/
  uint8_t keyMap[128];

  /** The MIDI base channel of the MachineDrum. **/
  uint8_t baseChannel;
  uint8_t unused;

  uint16_t tempo;
  bool extendedMode;
  bool clockIn;
  bool clockOut;
  bool transportIn;
  bool transportOut;
  bool localOn;

  uint8_t drumLeft;
  uint8_t drumRight;
  uint8_t gateLeft;
  uint8_t gateRight;
  uint8_t senseLeft;
  uint8_t senseRight;
  uint8_t minLevelLeft;
  uint8_t minLevelRight;
  uint8_t maxLevelLeft;
  uint8_t maxLevelRight;

  uint8_t programChange;
  uint8_t trigMode;

  A4Global() {}

  /** Read in a global message from a sysex buffer. **/
  bool fromSysex(MidiClass *midi);
  uint16_t toSysex(ElektronDataToSysexEncoder &encoder);

  /* @} */
};

class
__attribute__((packed))
a4time_t {
  uint8_t data;
public:
  a4time_t() {};
  a4time_t(uint8_t dat) : data(dat) {}
  float decode() { return .0f; }
};

class
__attribute__((packed))
a4notelen_t {
  uint8_t data;
public:
  a4notelen_t() {};
  a4notelen_t(uint8_t dat) : data(dat) {}
  float decode() { return .0f; }
};

/**
 * Two-byte unsigned floating numbers
 **/
class 
__attribute__((packed))
a4ufloat_t {
  uint8_t data[2];
public:
  a4ufloat_t() {};
  a4ufloat_t(uint8_t dat[2]) { memcpy(&data,&dat,2); }
  float decode() { return .0f; }
};

/**
 * Two-byte signed floating numbers
 **/
class 
__attribute__((packed))
a4sfloat_t {
  uint8_t data[2];
public:
  a4sfloat_t() {};
  a4sfloat_t(uint8_t dat[2]) {  memcpy(&data,&dat,2); }
  float decode() { return .0f; }
};

struct 
__attribute__((packed))
a4flt_t{
  a4ufloat_t freq;
  uint8_t    res;
  uint8_t    res_pad;
  int8_t     overdrive;
  int8_t     overdrive_pad;
  int8_t     keytrack;
  int8_t     keytrack_pad;
  int8_t     env_depth;
  int8_t     env_depth_pad;
};

// XXX to be studied further
struct 
__attribute__((packed))
a4mod_t {
  uint8_t dest1[2];
  int8_t  depth1[2];
  uint8_t dest2[2];
  int8_t  depth2[2];
  uint8_t dest3[2];
  int8_t  depth3[2];
  uint8_t dest4[2];
  int8_t  depth4[2];
  uint8_t dest5[2];
  int8_t  depth5[2];
};

struct 
__attribute__((packed))
a4sound_t {
  // 0x2b
  int8_t      osc1_tuning;
  int8_t      osc1_fine;
  int8_t      osc2_tuning;
  int8_t      osc2_fine;
  int8_t      osc1_detune;
  int8_t      osc1_detune_pad;
  int8_t      osc2_detune;
  int8_t      osc2_detune_pad;
  bool        osc1_keytrack;
  bool        osc1_keytrack_pad;
  bool        osc2_keytrack;
  bool        osc2_keytrack_pad;
  uint8_t     osc1_level;
  uint8_t     osc1_level_pad;
  uint8_t     osc2_level;
  uint8_t     osc2_level_pad;
  // 0x3d
  uint8_t     osc1_waveform;
  uint8_t     osc1_waveform_pad;
  uint8_t     osc2_waveform;
  uint8_t     osc2_waveform_pad;
  uint8_t     osc1_sub;
  uint8_t     osc1_sub_pad;
  uint8_t     osc2_sub;
  uint8_t     osc2_sub_pad;
  // 0x46
  int8_t      osc1_pulse_width;
  int8_t      osc1_pulse_width_pad;
  int8_t      osc2_pulse_width;
  int8_t      osc2_pulse_width_pad;
  uint8_t     osc1_pwm_speed;
  uint8_t     osc1_pwm_speed_pad;
  uint8_t     osc2_pwm_speed;
  uint8_t     osc2_pwm_speed_pad;
  uint8_t     osc1_pwm_depth;
  uint8_t     osc1_pwm_depth_pad;
  uint8_t     osc2_pwm_depth;
  uint8_t     osc2_pwm_depth_pad;
  uint8_t     osc_pad[6];
  // 0x5b
  uint8_t     noise_samplehold;
  uint8_t     noise_samplehold_pad;
  int8_t      noise_fade;
  int8_t      noise_fade_pad;
  uint8_t     noise_level;
  uint8_t     noise_level_pad;
  bool        osc1_am;
  bool        osc1_am_pad;
  bool        osc2_am;
  bool        osc2_am_pad;
  uint8_t     sync_mode;
  uint8_t     sync_mode_pad;
  uint8_t     sync_amount;
  uint8_t     sync_amount_pad;
  int8_t      bend_depth;
  int8_t      bend_depth_pad;
  uint8_t     note_slidetime;
  uint8_t     note_slidetime_pad;
  bool        osc_retrig;
  bool        osc_retrig_pad;
  // 0x71
  int8_t      vibrato_fade;
  int8_t      vibrato_fade_pad;
  uint8_t     vibrato_speed;
  uint8_t     vibrato_speed_pad;
  uint8_t     vibrato_depth;
  uint8_t     vibrato_depth_pad;
  // 0x78
  a4flt_t     filter1;
  a4flt_t     filter2;
  uint8_t     filters_pad[2];
  // 0x91
  uint8_t     amp_chorus;
  uint8_t     amp_chorus_pad;
  uint8_t     amp_delay;
  uint8_t     amp_delay_pad;
  uint8_t     amp_reverb;
  uint8_t     amp_reverb_pad;
  int8_t      amp_panning;
  int8_t      amp_panning_pad;
  uint8_t     amp_level;
  uint8_t     amp_level_pad;
  uint8_t     amp_accent;
  uint8_t     amp_accent_pad;
  // 0x9f
  uint8_t     envF_attack;
  uint8_t     envF_attack_pad;
  uint8_t     env2_attack;
  uint8_t     env2_attack_pad;
  uint8_t     amp_attack;
  uint8_t     amp_attack_pad;
  uint8_t     envF_decay;
  uint8_t     envF_decay_pad;
  uint8_t     env2_decay;
  uint8_t     env2_decay_pad;
  uint8_t     amp_decay;
  uint8_t     amp_decay_pad;
  uint8_t     envF_sustain;
  uint8_t     envF_sustain_pad;
  uint8_t     env2_sustain;
  uint8_t     env2_sustain_pad;
  uint8_t     amp_sustain;
  uint8_t     amp_sustain_pad;
  uint8_t     envF_release;
  uint8_t     envF_release_pad;
  uint8_t     env2_release;
  uint8_t     env2_release_pad;
  uint8_t     amp_release;
  uint8_t     amp_release_pad;
  // 0xbb
  uint8_t     envF_shape;
  uint8_t     envF_shape_pad;
  uint8_t     env2_shape;
  uint8_t     env2_shape_pad;
  uint8_t     amp_shape;
  uint8_t     amp_shape_pad;
  a4time_t    envF_gatelen;
  a4time_t    envF_gatelen_pad;
  a4time_t    env2_gatelen;
  a4time_t    env2_gatelen_pad;
  uint8_t     envF_destA;
  uint8_t     envF_destA_pad;
  uint8_t     envF_destB;
  uint8_t     envF_destB_pad;
  uint8_t     env2_destA;
  uint8_t     env2_destA_pad;
  uint8_t     env2_destB;
  uint8_t     env2_destB_pad;
  a4sfloat_t  envF_depthB;
  a4sfloat_t  envF_depthA;
  a4sfloat_t  env2_depthA;
  a4sfloat_t  env2_depthB;

  int8_t      lfo1_speed;
  int8_t      lfo1_speed_pad;
  int8_t      lfo2_speed;
  int8_t      lfo2_speed_pad;
  uint8_t     lfo1_multiplier;
  uint8_t     lfo1_multiplier_pad;
  uint8_t     lfo2_multiplier;
  uint8_t     lfo2_multiplier_pad;
  int8_t      lfo1_fade;
  int8_t      lfo1_fade_pad;
  int8_t      lfo2_fade;
  int8_t      lfo2_fade_pad;
  uint8_t     lfo1_phase;
  uint8_t     lfo1_phase_pad;
  uint8_t     lfo2_phase;
  uint8_t     lfo2_phase_pad;
  uint8_t     lfo1_mode;
  uint8_t     lfo1_mode_pad;
  uint8_t     lfo2_mode;
  uint8_t     lfo2_mode_pad;
  uint8_t     lfo1_wave;
  uint8_t     lfo1_wave_pad;
  uint8_t     lfo2_wave;
  uint8_t     lfo2_wave_pad;
  uint8_t     lfo1_destA;
  uint8_t     lfo1_destA_pad;
  uint8_t     lfo1_destB;
  uint8_t     lfo1_destB_pad;
  uint8_t     lfo2_destA;
  uint8_t     lfo2_destA_pad;
  uint8_t     lfo2_destB;
  uint8_t     lfo2_destB_pad;
  a4sfloat_t  lfo1_depthA;
  a4sfloat_t  lfo1_depthB;
  a4sfloat_t  lfo2_depthA;
  a4sfloat_t  lfo2_depthB;

  uint8_t     lfo_pad[8];

  // 0x10f
  int8_t      noise_color;
  uint8_t     noise_color_pad[7];
  bool        osc_drift;
  uint8_t     note_portamode;
  bool        note_legato;
  uint8_t     note_velcurve;
  bool        f1res_boost;
  uint8_t     f1res_boost_pad[3];

  // 0x121
  a4mod_t          mod_velocity;
  bool             mod_velocity_pad;
  // 0x139
  bool             mod_velocity_bipolar;
  // 0x13B
  a4mod_t          mod_aftertouch;
  // 0x151
  a4mod_t          mod_modwheel;
  // 0x168
  a4mod_t          mod_pitchbend;
  // 0x17F
  a4mod_t          mod_breadth;
};

class A4Sound_270 {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:
  bool workSpace; // When transferring sounds, we must decide if we are going to
                  // send them to the RAM workspace, or permanent memory.
  uint8_t origPosition;
  // old a4 sound patch layout: uint8_t[415 - 10 - 2 - 4 - 1], 398 bytes
  // -10 : prologue and 0x78 0x3E, part of a4 sound header.
  // -2: sysex frame
  // -4: len & checksum
  // -1: origposition
  uint8_t payload[415 - 10 - 2 - 4 - 1];
};

class 
__attribute__((packed))
A4Sound {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:
  A4Sound() { };
  bool soundpool; // When transferring sounds, we must decide if we are going to
                  // send them to the pool (RAM workspace), or permanent memory.
                  // The pooled patches can be P-Locked, while permanent memory
                  // patches should be loaded to the track and then pooled first

  uint8_t          origPosition; // 0-127


  // === Begin new a4 sound patch layout
  uint8_t          tags[4];      // 32 tags
  char             name[16];     // null-terminated
  // sizeof(a4sound_t) is 318, which encodes 363 bytes in 7-bit enc, + 42 bytes header metadata + 8B prologue + 2B SYSEX frame = 415B
  a4sound_t        sound;
  // === End new a4 sound patch layout, 338B

  void convert(A4Sound_270* old);

  bool fromSysex_impl(ElektronSysexDecoder *decoder);
  /** Convert the sound object into a sysex buffer to be sent to the
   * AnalogFour. **/
  void toSysex_impl(ElektronDataToSysexEncoder *encoder);
  bool fromSysex(MidiClass *midi);
  /** Convert the global object into a sysex buffer to be sent to the
   * machinedrum. **/
  uint16_t toSysex(ElektronDataToSysexEncoder *encoder);
  uint16_t toSysex();
};

/**
 * This class is a short version of the full kit class to store just
 * the models, names and position of a kit for studio firmwares.
 **/
/**
 * This class stores the settings for a complete kit on the
 * machinedrum, including effect and machine settings.
 **/

// 2679 - 10 /*header/  - 4 /len check/ - 1 /F7 = 2664
// 398 * 4
class A4Kit : public ElektronSysexObject {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:
  bool workSpace;
  uint8_t origPosition;
  // Unknown data structure, probably includes levels kit name etc.
  uint8_t payload_start[38];
  A4Sound sounds[4];
  // Unknown data strucutre, probably includes CV and FX settings.
  uint8_t payload_end[1034]; // 2664-398*4-38

  uint16_t toSysex();
  /**
   * Convert the global object and encode it into a sysex encoder,
   * for example to send directly to the UAR.
   **/
  virtual uint16_t toSysex(ElektronDataToSysexEncoder *encoder);

  /**
   * Swap two machines.
   **/
  void swapTracks(uint8_t srcTrack, uint8_t dstTrack);

  /* @} */
};

/* @} */

#endif /* A4MESSAGES_H__ */
