/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ENCODERS_H__
#define ENCODERS_H__

#include <inttypes.h>
#include "helpers.h"
#include "GUI_private.h"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_encoders Encoder classes
 *
 * @{
 *
 * \file
 * Encoder classes
 **/
class Encoder;

/**
 * Prototype for encoder handling functions. These functions are
 * called when the value of an encoder has changed. Examples for
 * encoder_handle_t functions are CCEncoderHandler or TempoEncoderHandle.
 **/
typedef void (*encoder_handle_t)(Encoder *enc);

/** Encoder handling function to send a CC value (enc has to be of class CCEncoder). **/
void CCEncoderHandle(Encoder *enc);
/** Encoder handling function to modify the tempo when the encoder value is changed. **/ 
void TempoEncoderHandle(Encoder *enc);

/**
 * \addtogroup gui_encoder_class Encoder
 * @{
 **/

/** Encoder parent class. **/
class Encoder {
	/**
	 * \addtogroup gui_encoder_class
	 * @{
	 **/
	
public:
	/** Old value (before move), current value. **/
	int old, cur;
	/** Short name. **/
	char name[4];
	/**
	 * If this variable is set to true, and pressmode to false, an
	 * encoder-turn with the encoder pressed down will lead to an
	 * increment by 5 times the value (default true).
	 *
	 * This will work with the parent update() method, not if update()
	 * is overloaded.
	 **/
	bool fastmode;
	/**
	 * If this variable is set to true, turning the encoder while the
	 * button is pressed will have no effect on the encoder value.
	 *
	 * This will work with the parent update() method, not if update()
	 * is overloaded.
	 **/
	bool pressmode;
	
	/** Handling function. **/
	encoder_handle_t handler;
	
	/** Create a new encoder with short name and handling function. **/
	Encoder(const char *_name = NULL, encoder_handle_t _handler = NULL);
	void clear();
	
	/** Returns the encoder name. **/
	virtual char *getName() { return name; }
	/** Set the encoder name (max 3 characters). **/
	virtual void setName(const char *_name);
	
	/** Should the encoder be displayed again? **/
	bool redisplay;
	
	/**
	 * Updates the value of an encoder according to the movements of the
	 * hardware (recorded in the encoder_t structure). The default
	 * handler adds the normal increment, and handles pressing down the
	 * encoder according to pressmode and fastmode.
	 **/
	virtual int update(encoder_t *enc);
	
	/**
	 * Handle a modification of the encoder, the default version calls
	 * the handling function handler if it is different from NULL.
	 **/
	virtual void checkHandle();
	
	/** Returns true if the encoder value changed. **/
	virtual bool hasChanged();
	
	/** Return the current value. **/
	virtual int getValue() { return cur; }
	/** Return the old value. **/
	virtual int getOldValue() { return old; }
	/**
	 * Set the value of the encoder to value. If handle is true,
	 * checkHandle() is called after the modification of the encoder
	 * value. This will make setValue() behave as if the user had moved
	 * the encoder.
	 **/
	virtual void setValue(int value, bool handle = false);
	
	/**
	 * Display the encoder at index i on the screen. The index is a
	 * multiple of 4 characters.  This can be overloaded to implement
	 * custom and fancy ways of displaying encoders.
	 **/
	virtual void displayAt(int i);
	
#ifdef HOST_MIDIDUINO
	virtual ~Encoder() { }
#endif
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_rangeencoder_class RangeEncoder
 * @{
 **/

/** Encoder with minimum and maximum value. **/
class RangeEncoder : public Encoder {
	/**
	 * \addtogroup gui_rangeencoder_class
	 * @{
	 **/
	
public:
	/** Minimum value of the encoder. **/
	int min;
	/** Maximum value of the encoder. **/
	int max;
	
	/**
	 * Create a new range-limited encoder with max and min value, short
	 * name, initial value, and handling function. The initRangeEncoder
	 * will be called with the constructor arguments.
	 **/
	RangeEncoder(int _max = 127, int _min = 0, const char *_name = NULL, int init = 0,
				 encoder_handle_t _handler = NULL) : Encoder(_name, _handler) {
		initRangeEncoder(_max, _min, _name, init, _handler);
	}
	
	/**
	 * Initialize the encoder with the same argument as the constructor.
	 *
	 * The initRangeEncoder functions automatically determines which of
	 * min and max is the minimum value. As of now this can't be used to
	 * have an "inverted" encoder.
	 *
	 * The initial value is called without calling the handling function.
	 **/
	void initRangeEncoder(int _max = 128, int _min = 0, const char *_name = NULL, int init = 0,
						  encoder_handle_t _handler = NULL) {
		setName(_name);
		handler = _handler;
		if (_min > _max) {
			min = _max;
			max = _min;
		} else {
			min = _min;
			max = _max;
		}
		setValue(init);
	}
	
	/**
	 * Update the value of the encoder according to pressmode and
	 * fastmode, and limit the resulting value using limit_value().
	 **/
	virtual int update(encoder_t *enc);
	
	/* @} */
};

void VarRangeEncoderHandle(Encoder *enc);

/** @} **/

/**
 * \addtogroup gui_varrangeencoder_class Variable-updating RangeEncoder
 * @{
 **/

/**
 * Encoder class that updates a variable. The value of the encoder is
 * written to the variable on each modification.
 **/
class VarRangeEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_varrangeencoder_class
	 * @{
	 **/
	
public:
	/**
	 * Pointer to the variable updated by the encoder. If this is
	 * different than NULL, the variable will be updated by the value
	 * of the encoder on each encoder modification.
	 **/
	uint8_t *var;
	
	/**
	 * Create a variable-updating encoder storing its value in the
	 * variable pointed to by _var.
	 **/
	VarRangeEncoder(uint8_t *_var, int _max = 127, int _min = 0, const char *_name = NULL, int init = 0) :
    RangeEncoder(_max, _min, _name, init, VarRangeEncoderHandle) {
		var = _var;
		if (var != NULL) {
			*var = init;
		}
		handler = VarRangeEncoderHandle;
	}
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_enumencoder_class Enumeration encoder
 * @{
 **/

/** Enumeration encoder, displaying a limited amount of choices as strings. **/
class EnumEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_enumencoder_class
	 * @{
	 **/
	
public:
	const char **enumStrings;
	int cnt;
	
	/**
	 * Create an enumeration encoder allowing to choose between _cnt
	 * different options. Each option should be described by a 3
	 * character string in the strings[] array. Turning the encoder will
	 * display the correct name.
	 **/
	EnumEncoder(const char *strings[] = NULL, int _cnt = 0, const char *_name = NULL, int init = 0,
				encoder_handle_t _handler = NULL) :
    RangeEncoder(_cnt - 1, 0, _name, init, _handler) {
		enumStrings = strings;
		cnt = _cnt;
	}
	
	void initEnumEncoder(const char *strings[], int _cnt, const char *_name = NULL, int init = 0) {
		enumStrings = strings;
		cnt = _cnt;
		min = 0;
		max = _cnt - 1;
		setValue(init);
		setName(_name);
	}
	
	virtual void displayAt(int i);
	
	/* @} */
};

/** Enumeration encoder with enumeration names stored in program space. **/
class PEnumEncoder : public EnumEncoder {
	/**
	 * \addtogroup gui_enumencoder_class
	 * @{
	 **/
	
public:
	PEnumEncoder(const char *strings[], int _cnt, const char *_name = NULL, int init = 0,
				 encoder_handle_t _handler = NULL) :
    EnumEncoder(strings, _cnt, _name, init, _handler) {
	}
	
	virtual void displayAt(int i);
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_boolencoder_class Boolean encoder 
 * @{
 **/

static const char *boolEnumStrings[] = { "OFF", "ON" };

/** Specialized enumeration encoder allowing the user to choose between "OFF" (0) and "ON" (1). **/
class BoolEncoder : public EnumEncoder {
	/**
	 * \addtogroup gui_boolencoder_class 
	 * @{
	 **/
	
public:
	BoolEncoder(const char *_name = NULL, bool init = false, encoder_handle_t _handler = NULL) :
    EnumEncoder(boolEnumStrings, 2, _name, init ? 1 : 0, _handler) {
	}
	
	void initBoolEncoder(const char *_name = NULL, bool init = false) {
		initEnumEncoder(boolEnumStrings, 2, _name, init ? 1 : 0);
	}
    
	
	bool getBoolValue() {
		return getValue() == 1;
	}
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_miditrackencoder_class MIDI Track encoder 
 * @{
 **/

/**
 * Encoder that allows to choose a MIDI track from 0 to 15 (displaying
 * 1 to 16 in order not to confuse musicians.
 **/
class MidiTrackEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_miditrackencoder_class
	 * @{
	 **/
public:
	MidiTrackEncoder(char *_name = NULL, uint8_t init = 0) : RangeEncoder(15, 0, _name, init) {
	}
	
	virtual void displayAt(int i);
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_ccencoder_class CC encoder 
 * @{
 **/

/**
 * Encoder that sends out a CC message on each modification.
 **/
class CCEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_ccencoder_class
	 * @{
	 **/
	
public:
	/** The CC number used when the CC message is sent. **/
	uint8_t cc;
	/** The MIDI channel number (from 0 to 15) to use when sending the CC message. **/
	uint8_t channel;
	
	virtual uint8_t getCC() {
		return cc;
	}
	virtual uint8_t getChannel() {
		return channel;
	}
	virtual void initCCEncoder(uint8_t _channel, uint8_t _cc) {
		cc = _cc;
		channel = _channel;
	}
	
	/** Create a CC encoder sending CC messages with number _cc on _channel. **/
	CCEncoder(uint8_t _cc = 0, uint8_t _channel = 0, const char *_name = NULL, int init = 0) :
    RangeEncoder(127, 0, _name, init) {
		initCCEncoder(_channel, _cc);
		handler = CCEncoderHandle;
	}
	
	/* @} */
};

char hex2c(uint8_t hex);

/** @} **/

/**
 * \addtogroup gui_autonameccencoder_class Auto-naming CC encoder 
 * @{
 **/

/**
 * CC Encoder that automatically updates its name according to its CC
 * nummber (channel number in hex from 0 to A, followed by CC number
 * in hex, 00 to 7F.
 **/
class AutoNameCCEncoder : public CCEncoder {
	/**
	 * \addtogroup gui_autonameccencoder_class
	 * @{
	 **/
	
public:
	AutoNameCCEncoder(uint8_t _cc = 0, uint8_t _channel = 0, const char *_name = NULL, int init = 0) :
    CCEncoder(_cc, _channel, _name, init) {
		if (_name == NULL) {
			setCCName();
		}
	}
	
	/** Automatically set the encoder name according to its CC number. **/
	void setCCName() {
		char name[4];
		name[0] = hex2c(getChannel());
		uint8_t cc = getCC();
		name[1] = hex2c(cc >> 4);
		name[2] = hex2c(cc & 0xF);
		name[3] = '\0';
		setName(name);
	}
	
	virtual void initCCEncoder(uint8_t _channel, uint8_t _cc);
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_tempoencoder_class Tempo encoder
 * @{
 **/

/**
 * Encoder that automatically updates the tempo of the midi clock when
 * changed. Ranges from 20 to 255 (because the max value of a range
 * encoder is 255 at the moment).
 **/
class TempoEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_tempoencoder_class
	 * @{
	 **/
	
public:
	TempoEncoder(const char *_name = NULL) : RangeEncoder(255, 20, _name) {
		handler = TempoEncoderHandle;
	}
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_charencoder_class Character encoder
 * @{
 **/

/**
 * Encoder that allows the user to choose between a character (a to z, A to Z, 0 to 9).
 **/
class CharEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_charencoder_class
	 * @{
	 **/
	
public:
	CharEncoder();
	char getChar();
	void setChar(char c);
	
	/* @} */
};

/** @} **/

/**
 * \addtogroup gui_notepitchencoder_class Note pitch encoder
 * @{
 **/

/**
 * Encoder that allows the user to choose a MIDI pitch (from 0 to
 * 127), displaying the correct name and octave.
 **/
class NotePitchEncoder : public RangeEncoder {
	/**
	 * \addtogroup gui_notepitchencoder_class
	 * @{
	 **/
public:
	NotePitchEncoder(char *_name = NULL);
	
	void displayAt(int i);
	
	/* @} */
};

#include "RecordingEncoder.hh"

/** @} @} @} **/

#endif /* ENCODERS_H__ */
