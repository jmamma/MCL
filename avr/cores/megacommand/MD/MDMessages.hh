/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDMESSAGES_H__
#define MDMESSAGES_H__

#include "MDPattern.hh"

extern uint8_t machinedrum_sysex_hdr[5];

/**
 * \addtogroup MD Elektron MachineDrum
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
class MDGlobal {
	/**
	 * \addtogroup md_sysex_global
	 * @{
	 **/
	
public:
	/* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES. */
	
	/** Original position of the global inside the MD (0 to 7). **/
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
	
	
	MDGlobal() {
	}
	
	/** Read in a global message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/** Convert the global object into a sysex buffer to be sent to the machinedrum. **/
	uint16_t toSysex(uint8_t *sysex, uint16_t len);
	/**
	 * Convert the global object and encode it into a sysex encoder,
	 * for example to send directly to the UAR.
	 **/
	uint16_t toSysex(ElektronDataToSysexEncoder &encoder);
	
	/* @} */
};

/* @} */

/**
 * \addtogroup md_sysex_kit MachineDrum Kit Message
 * @{
 **/

/**
 * This class stores the LFO settings for a track, inside the Kit object.
 **/
class MDLFO {
	/**
	 * \addtogroup md_sysex_kit
	 * @{
	 **/
	
public:
	/* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE PARAMETERS */
	
	/** The destination track of this LFO. **/
	uint8_t destinationTrack;
	/** The destination parameter of this LFO. **/
	uint8_t destinationParam;
	/** The first shape of this LFO. **/
	uint8_t shape1;
	/** The second shape of this LFO. **/
	uint8_t shape2;
	/** The LFO type. **/
	uint8_t type;
	/** The internal state of the LFO, must not all be 0!. **/
	uint8_t state[31];
	/** The LFO speed. **/
	uint8_t speed;
	/** The LFO depth. **/
	uint8_t depth;
	/** The LFO mix setting. **/
	uint8_t mix;
	
	/* @} */
};

/**
 * This class stores the complete settings for a track inside the Kit object.
 **/
class MDMachine {
	/**
	 * \addtogroup md_sysex_kit
	 * @{
	 **/
	
public:
	uint8_t params[24];
	uint8_t track;
	uint8_t level;
	uint32_t model;
	MDLFO lfo;
	uint8_t trigGroup;
	uint8_t muteGroup;
	
	/* @} */
};

/**
 * This class is a short version of the full kit class to store just
 * the models, names and position of a kit for studio firmwares.
 **/

class MDKitShort {
	/**
	 * \addtogroup md_sysex_kit
	 * @{
	 **/
	
public:
	uint8_t origPosition;
	char name[17];
	uint32_t models[16];
	
	MDKitShort() {
	}
	
	/** Read in a kit message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	
	/* @} */
};

/**
 * This class stores the settings for a complete kit on the
 * machinedrum, including effect and machine settings.
 **/
class MDKit {
	/**
	 * \addtogroup md_sysex_kit
	 * @{
	 **/
	
public:
	uint8_t origPosition;
	char name[17];
	
	/** The parameters for each track. **/
	uint8_t params[16][24];
	/** The levels of each track. **/
	uint8_t levels[16];
	/** The selected drum model for each track. **/
	uint32_t models[16];
	/** The LFO settings for each track. **/
	MDLFO lfos[16];
	
	/** The settings of the reverb effect. **/
	uint8_t reverb[8];
	/** The settings of the delay effect. **/
	uint8_t delay[8];
	/** The settings of the EQ effect. **/
	uint8_t eq[8];
	/** The settings of the compressor effect. **/
	uint8_t dynamics[8];
	
	/** The trig group selected for each track (255: OFF). **/
	uint8_t trigGroups[16];
	/** The mute group selected for each track (255: OFF). **/
	uint8_t muteGroups[16];
	
	/** Read in a kit message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/** Convert a kit object to a sysex buffer ready to be sent to the MD. **/
	uint16_t toSysex(uint8_t *sysex, uint16_t len);
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

/**
 * \addtogroup md_sysex_song MachineDrum Song Message
 * @{
 **/

/**
 * This class stores a single row in a song.
 **/
class MDRow {
	/**
	 * \addtogroup md_sysex_song
	 * @{
	 **/
	
public:
	/* DO NOT CHANGE THE ORDER OF DECLARATION OF THESE VARIABLES. */
	uint8_t pattern;
	uint8_t kit;
	uint8_t loopTimes;
	uint8_t jump;
	uint16_t mutes;
	uint16_t tempo;
	uint8_t startPosition;
	uint8_t endPosition;
	
	/* @} */
};

/**
 * This class stores a song of the MachineDrum.
 **/
class MDSong {
	/**
	 * \addtogroup md_sysex_song
	 * @{
	 **/
	
public:
	uint8_t origPosition;
	char name[17];
	MDRow rows[256];
	uint8_t numRows;
	
	/** Read in a song message from a sysex buffer. **/
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/** Convert a song object to a sysex buffer ready to be sent to the MD. **/
	uint16_t toSysex(uint8_t *sysex, uint16_t len);
	/**
	 * Convert the global object and encode it into a sysex encoder,
	 * for example to send directly to the UAR.
	 **/
	uint16_t toSysex(ElektronDataToSysexEncoder &encoder);
	
	/* @} */
};

/* @} */

#endif /* MDMESSAGES_H__ */
