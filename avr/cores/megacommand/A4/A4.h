#ifndef A4_H__
#define A4_H__

#include "WProgram.h"

#include "Elektron.h"

#include "A4Messages.h"
#include "A4Params.h"
#include "A4Sysex.h"

/**
 * \addtogroup a4_a4
 *
 * @{
 */

/** Standard elektron sysex header for communicating with the machinedrum. **/
extern uint8_t a4_sysex_hdr[5];
extern uint8_t a4_sysex_proto_version[2];
extern uint8_t a4_sysex_ftr[4];
extern const ElektronSysexProtocol a4_protocol;
/**
 * This is the main class used to communicate with an A4
 * connected to the Minicommand.
 *
 **/
class A4Class : public ElektronDevice {
  /**
   * \addtogroup a4_a4
   *
   * @{
   */

public:
  A4Class();

  virtual bool probe();
  virtual void init_grid_devices();
  virtual uint8_t* icon();

  virtual uint16_t sendKitParams(uint8_t* masks);

  // Overriden for A4 proto version and footer injection
  virtual uint16_t sendRequest(uint8_t, uint8_t, bool send = true);

  virtual ElektronSysexListenerClass* getSysexListener() { return &A4SysexListener; }
  // TODO A4 kit not placed in class
  virtual ElektronSysexObject* getKit() { return nullptr; }
  // TODO A4 pattern not placed in class
  virtual ElektronSysexObject* getPattern() { return nullptr; }
  // TODO A4 global not placed in class
  virtual ElektronSysexObject* getGlobal() { return nullptr; }

  void requestKitX(uint8_t kit);

  void requestSound(uint8_t sound);
  void requestSoundX(uint8_t sound);

  void requestPatternX(uint8_t pattern);

  void requestSongX(uint8_t song);

  void requestSettings(uint8_t setting);
  void requestSettingsX(uint8_t setting);

  void requestGlobalX(uint8_t global);

  bool getBlockingSound(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingSettings(uint8_t global, uint16_t timeout = 3000);

  virtual bool getBlockingPattern(uint8_t pattern, uint16_t timeout) {
    // TODO A4 get pattern is disabled, but reports success.
    return true;
  }

  virtual bool getBlockingKit(uint8_t kit, uint16_t timeout) {
    // TODO A4 get kit is disabled, but reports success.
    return true;
  }

  /*X denotes get from RAM/unsaved  */
  bool getBlockingKitX(uint8_t kit, uint16_t timeout = 3000);
  bool getBlockingPatternX(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingGlobalX(uint8_t global, uint16_t timeout = 3000);
  bool getBlockingSoundX(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingSettingsX(uint8_t global, uint16_t timeout = 3000);

  void muteTrack(uint8_t track, bool mute = true);
  void unmuteTrack(uint8_t track) { muteTrack(track, false); }
  void setLevel(uint8_t track, uint8_t value);
};

/**
 * The standard always present object representing the A4 to which the
 * minicommand is connected.
 **/
extern A4Class Analog4;

/* @} */

#endif /* A4_H__ */
