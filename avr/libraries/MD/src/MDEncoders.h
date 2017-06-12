/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_ENCODERS_H__
#define MD_ENCODERS_H__

#include <MD.h>
#include <GUI.h>

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_encoders MachineDrum encoders
 * 
 * @{
 **/

#ifdef MIDIDUINO_USE_GUI

/**
 * This encoder controls a specific track parameter on the machinedrum by sending CC messages.
 *
 * It depends on the global channel settings.
 **/
class MDEncoder : public CCEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
public:
  uint8_t track;
  uint8_t param;

  virtual uint8_t getCC();
  virtual uint8_t getChannel();
  virtual void initCCEncoder(uint8_t _channel, uint8_t _cc);
  void initMDEncoder(uint8_t _track = 0, uint8_t _param = 0, char *_name = NULL, uint8_t init = 0) {
    track = _track;
    param = _param;
    setName(_name);
    setValue(init);
  }

  MDEncoder(uint8_t _track = 0, uint8_t _param = 0, char *_name = NULL, uint8_t init = 0);

	/** Load the value of the encoder from the stored parameter value in the currently loaded kit.
	 **/
  void loadFromKit();
	
	/* @} */
};

/**
 * This encoder controls a parameter of a machinedrum effect by sending sysex messages.
 **/
class MDFXEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  uint8_t effect;
  uint8_t param;

  void initMDFXEncoder(uint8_t _param = 0, uint8_t _effect = MD_FX_ECHO, char *_name = NULL, uint8_t init = 0) {
    effect = _effect;
    param = _param;
    setName(_name);
    setValue(init);
  }
  MDFXEncoder(uint8_t _param = 0, uint8_t _effect = MD_FX_ECHO, char *_name = NULL, uint8_t init = 0);
	/**
	 * Load the value of the encoder from the stored effect parameter value in the currently loaded kit.
	 **/
  void loadFromKit();
	void display();
    int update(encoder_t *enc);
    
    void displayAt(int i);
	/* @} */
};

/**
 * This encoder controls a LFO parameter on a given track by sending sysex messages.
 **/
class MDLFOEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  uint8_t track;
  uint8_t param;

  void initMDLFOEncoder(uint8_t _param = MD_LFO_TRACK, uint8_t _track = 0,
			char *_name = NULL, uint8_t init = 0) {
    param = _param;
    track = _track;
    if (_name == NULL) {
      setLFOParamName();
    } else {
      setName(_name);
    }
    setValue(init);
  }

  MDLFOEncoder(uint8_t _param = MD_LFO_TRACK, uint8_t _track = 0,
	       char *_name = NULL, uint8_t init = 0);
  
  void loadFromKit();
  void setLFOParamName();
  void setParam(uint8_t _param);

  virtual void displayAt(int i);
	
	/* @} */
};

/**
 * This encoder allows to select a track on the machinedrum (from 0 to
 * 15), and displays the currently loaded machine on the track when
 * the encoder is moved. It depends on the current kit being correctly
 * loaded on the machinedrum.
 **/
class MDTrackFlashEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
 MDTrackFlashEncoder(char *_name = NULL, uint8_t init = 0) : RangeEncoder(0, 15, _name, init) {
  }

  virtual void displayAt(int i);
	
	/* @} */
};

/**
 * This encoder is similar to the MDTrackFlashEncoder, but only
 * displays melodic tracks, and "XXX" when the track is not loaded
 * with a melodic machine.
 **/
class MDMelodicTrackFlashEncoder : public MDTrackFlashEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
 MDMelodicTrackFlashEncoder(char *_name = NULL, uint8_t init = 0) : MDTrackFlashEncoder(_name, init) {
  }

  virtual void displayAt(int i);
	
	/* @} */
};

/**
 * This encoder allows to switch the kits on the machinedrum. When the
 * encoder is moved, the selected kit is automatically loaded on the
 * machinedrum.
 **/
class MDKitSelectEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  MDKitSelectEncoder(const char *_name = NULL, uint8_t init = 0);

  virtual void displayAt(int i);
	
	/* @} */
};

/**
 * This encoder allows to select a pattern on the MachineDrum (0 -
 * 63). It automatically displays the correct name for the pattern.
 **/
class MDPatternSelectEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  MDPatternSelectEncoder(const char *_name = NULL, uint8_t init = 0);

  virtual void displayAt(int i);
  void loadFromMD();
	
	/* @} */
};

/**
 * This encoder allows to select a parameter (0 to 23) on the given
 * track. It automatically displays the name of the parameter.
 **/
class MDParamSelectEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
 MDParamSelectEncoder(uint8_t _track = 0, const char *_name = NULL, uint8_t init = 0) :
  track(_track), RangeEncoder(0, 23, _name, init) {
  }

  uint8_t track;
  virtual void displayAt(int i);

	/* @} */
};

/**
 * This encoder allows to select and load a specific machine on the
 * given track. It automatically loads the selected machine on the
 * given track, and flashes the name of the select model.
 **/
class MDAssignMachineEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  MDAssignMachineEncoder(uint8_t _track = 0, const char *_name = NULL, uint8_t init = 0);

  uint8_t track;
  virtual void displayAt(int i);
  void loadFromMD();

	/* @} */
};

/**
 * This encoder allows to select the trigger group triggered track for the given track.
 * It flashes the model loaded on the selected destination track.
 **/
class MDTrigGroupEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  MDTrigGroupEncoder(uint8_t _track = 0, const char *_name = NULL, uint8_t init = 0);

  uint8_t track;
  virtual void displayAt(int i);
  void loadFromMD();

	/* @} */
};

/**
 * This encoder allows to select the mute group muted track for the given track.
 * It flashes the model loaded on the selected destination track.
 **/
class MDMuteGroupEncoder : public RangeEncoder {
	/**
	 * \addtogroup md_encoders
	 *
	 * @{
	 **/
	
 public:
  MDMuteGroupEncoder(uint8_t _track = 0, const char *_name = NULL, uint8_t init = 0);

  uint8_t track;
  virtual void displayAt(int i);
  void loadFromMD();

	/* @} */
};

#endif

#endif /* MD_ENCODERS_H__ */
