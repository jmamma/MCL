#ifndef A4_H__
#define A4_H__

#include "WProgram.h"

#include "../Elektron/Elektron.hh"


/**
 * \addtogroup a4_callbacks
 *
 * @{
 * A4 Callback class, inherit from this class if you want to use callbacks on A4 events.
 **/
class A4Callback {
};

/**
 * Standard method prototype for argument-less A4 callbacks.
 **/
typedef void(A4Callback::*A4_callback_ptr_t)();

class A4BlockCurrentStatusCallback : public A4Callback {
    /** 
     * \addtogroup md_callbacks
     * @{
     **/
    
public:
  uint8_t type;
  uint8_t value;
  bool received;

   A4BlockCurrentStatusCallback(uint8_t _type = 0) {
    type = _type;
    received = false;
    value = 255;
  }

    void onStatusResponseCallback(uint8_t _type, uint8_t param) {
    
     // GUI.printf_fill("eHHHH C%h N%h ",value, param);
      if (type == _type) {
      value = param;
      received = true;
    }   
  }

    void onSysexReceived() {
        received = true;
    }   

    /* @} */
};

#include "A4Sysex.hh"
#include "A4Params.hh"
#include "A4Messages.hh"

/**
 * \addtogroup a4_a4
 *
 * @{
 */

/** Standard elektron sysex header for communicating with the machinedrum. **/
extern uint8_t a4_sysex_hdr[5];
extern uint8_t a4_sysex_proto_version[2];
extern uint8_t a4_sysex_ftr[4];
/**
 * This is the main class used to communicate with an A4
 * connected to the Minicommand.
 *
**/
class A4Class {
	/**
	 * \addtogroup a4_a4
	 *
	 * @{
	 */
	
 public:
  A4Class();
	/** Send the given sysex buffer to the A4. **/
  void sendSysex(uint8_t *bytes, uint8_t cnt);
	/**
	 * Send a sysex request to the MachineDrum. All the request calls
	 * are wrapped in appropriate methods like requestKit,
	 * requestPattern, etc...
	 **/
  void sendRequest(uint8_t type, uint8_t param);

  void requestKit(uint8_t kit); 
  void requestKitX(uint8_t kit);

  void requestSound(uint8_t sound);
  void requestSoundX(uint8_t sound);
  
  void requestPattern(uint8_t pattern);
  void requestPatternX(uint8_t pattern);
  
  void requestSong(uint8_t song);
  void requestSongX(uint8_t song);

  void requestSettings(uint8_t setting);
  void requestSettingsX(uint8_t setting);
  
  void requestGlobal(uint8_t global);
  void requestGlobalX(uint8_t global);

  /* requests */
  /**
	 * Wait for a blocking answer to a status request. Timeout is in clock ticks.
	 **/
	bool waitBlocking(A4BlockCurrentStatusCallback *cb, uint16_t timeout = 3000);
 
  bool getBlockingKit(uint8_t kit, uint16_t timeout = 3000);
  bool getBlockingPattern(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingGlobal(uint8_t global, uint16_t timeout = 3000);
  bool getBlockingSound(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingSettings(uint8_t global, uint16_t timeout = 3000);

  /*X denotes get from RAM/unsaved  */
  bool getBlockingKitX(uint8_t kit, uint16_t timeout = 3000);
  bool getBlockingPatternX(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingGlobalX(uint8_t global, uint16_t timeout = 3000);
  bool getBlockingSoundX(uint8_t pattern, uint16_t timeout = 3000);
  bool getBlockingSettingsX(uint8_t global, uint16_t timeout = 3000);

  void muteTrack(uint8_t track, bool mute = true);
  void unmuteTrack(uint8_t track) {
                muteTrack(track, false);
  }

};

/**
 * The standard always present object representing the A4 to which the
 * minicommand is connected.
 **/
extern A4Class Analog4;

/* @} */


#endif /* A4_H__ */
