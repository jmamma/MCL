#ifndef A4MESSAGES_H__
#define A4MESSAGES_H__

#include "A4Pattern.hh"
#include "helpers.h"
#include "Elektron.hh"

#include <algorithm>

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
  bool fromSysex(uint8_t *sysex, uint16_t len);
  /** Convert the global object into a sysex buffer to be sent to the
   * machinedrum. **/
  uint16_t toSysex(uint8_t *sysex, uint16_t len);
  /**
   * Convert the global object and encode it into a sysex encoder,
   * for example to send directly to the UAR.
   **/
  uint16_t toSysex(ElektronDataToSysexEncoder &encoder);

  /* @} */
};

class a4time_t {
  uint8_t data;
public:
  a4time_t(uint8_t dat) : data(dat) {}
  float decode() { return .0f; }
};

class a4notelen_t {
  uint8_t data;
public:
  a4notelen_t(uint8_t dat) : data(dat) {}
  float decode() { return .0f; }
};

/**
 * Two-byte unsigned floating numbers
 **/
class a4ufloat_t {
  uint8_t data[2];
public:
  a4ufloat_t(uint8_t dat[2]) { std::copy(dat, dat + 2, data); }
  float decode() { return .0f; }
};

/**
 * Two-byte signed floating numbers
 **/
class a4sfloat_t {
  uint8_t data[2];
public:
  a4sfloat_t(uint8_t dat[2]) { std::copy(dat, dat + 2, data); }
  float decode() { return .0f; }
};

__attribute__((packed))
struct a4osc_t {
  int8_t  tuning;
  int8_t  fine;
  bool    keytrack;
  uint8_t level;
  int8_t  detune;
  uint8_t waveform;
  int8_t  pulse_width;
  uint8_t pwm_speed;
  uint8_t pwm_depth;
  uint8_t sub;
  bool    am;
};

__attribute__((packed))
struct a4flt_t{
  a4ufloat_t freq;
  uint8_t    res;
  int8_t     overdrive;
  int8_t     keytrack;
  int8_t     env_depth;
};

__attribute__((packed))
struct a4env_t {
  uint8_t    attack;
  uint8_t    decay;
  uint8_t    sustain;
  uint8_t    release;
  uint8_t    shape;
  a4time_t   gatelen;
  uint8_t    destA;
  a4sfloat_t depthA;
  uint8_t    destB;
  a4sfloat_t depthB;
};

__attribute__((packed))
struct a4lfo_t {
  int8_t  speed;
  uint8_t multiplier;
  int8_t  fade;
  uint8_t phase;
  uint8_t mode;
  uint8_t wave;
  uint8_t destA;
  a4sfloat_t depthA;
  uint8_t destB;
  a4sfloat_t depthB;
};

__attribute__((packed))
struct a4mod_t {
  uint8_t dest[5];
  int8_t  depth[5];
};

__attribute__((packed))
struct a4sound_common_t {
  uint8_t noise_samplehold;
  int8_t  noise_color;
  int8_t  noise_fade;
  uint8_t noise_level;
  uint8_t sync_mode;
  uint8_t sync_amount;
  uint8_t note_slidetime;
  bool    note_legato;
  uint8_t note_portamode;
  uint8_t note_velcurve;
  bool    osc_retrig;
  bool    osc_drift;
  int8_t  vibrato_fade;
  uint8_t vibrato_speed;
  uint8_t vibrato_depth;
  uint8_t amp_attack;
  uint8_t amp_decay;
  uint8_t amp_sustain;
  uint8_t amp_release;
  uint8_t amp_shape;
  uint8_t amp_chorus;
  uint8_t amp_delay;
  uint8_t amp_reverb;
  uint8_t amp_level;
  int8_t  amp_panning;
  uint8_t amp_accent;
};

__attribute__((packed))
class A4Sound {
  /**
   * \addtogroup md_sysex_kit
   * @{
   **/

public:
  bool soundpool; // When transferring sounds, we must decide if we are going to
                  // send them to the pool (RAM workspace), or permanent memory.
                  // The pooled patches can be P-Locked, while permanent memory
                  // patches should be loaded to the track and then pooled first

  uint8_t          origPosition; // 0-127
  uint8_t          tags[4];      // 32 tags
  char             name[16];     // null-terminated
  a4osc_t          osc[2];
  a4flt_t          filter[2];
  a4env_t          envF;
  a4env_t          env2;
  a4sound_common_t common;
  bool             mod_velocity_bipolar;
  a4mod_t          mod_velocity;
  a4mod_t          mod_aftertouch;
  a4mod_t          mod_modwheel;
  a4mod_t          mod_pitchbend;
  a4mod_t          mod_breadth;
  
  bool fromSysex(uint8_t *sysex, uint16_t len);
  /** Convert the sound object into a sysex buffer to be sent to the
   * AnalogFour. **/
  uint16_t toSysex(uint8_t *sysex, uint16_t len);
  uint16_t toSysex();
  uint16_t toSysex(ElektronDataToSysexEncoder &encoder);
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
class A4Kit {
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

  /** Read in a kit message from a sysex buffer. **/
  bool fromSysex(uint8_t *sysex, uint16_t len);
  /** Convert a kit object to a sysex buffer ready to be sent to the A4. **/
  uint16_t toSysex(uint8_t *sysex, uint16_t len);
  uint16_t toSysex();
  /**
   * Convert the global object and encode it into a sysex encoder,
   * for example to send directly to the UAR.
   **/
  uint16_t toSysex(ElektronDataToSysexEncoder &encoder);

  /**
   * Swap two machines.
   **/
  void swapTracks(uint8_t srcTrack, uint8_t dstTrack);

  /* @} */
};

/* @} */

#endif /* A4MESSAGES_H__ */
