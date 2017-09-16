#ifndef A4MESSAGES_H__
#define A4MESSAGES_H__

#include "A4Pattern.hh"

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
	
	
	A4Global() {
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

class A4Sound {
	/**
	 * \addtogroup md_sysex_kit
	 * @{
	 **/
	
public:
    bool    workSpace; //When transferring sounds, we must decide if we are going to send them to the RAM workspace, or permanent memory.
	uint8_t origPosition;
    uint8_t payload[415 - 10 - 2 - 4 - 1];
	bool fromSysex(uint8_t *sysex, uint16_t len);
	/** Convert the global object into a sysex buffer to be sent to the machinedrum. **/
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

//2679 - 10 /*header/  - 4 /len check/ - 1 /F7 = 2664
//398 * 4
class A4Kit {
	/**
	 * \addtogroup md_sysex_kit
	 * @{
	 **/
	
public:
    bool    workSpace;
    uint8_t origPosition;
    //Unknown data structure, probably includes levels kit name etc.
    uint8_t payload_start[38];
    A4Sound sounds[4];
    //Unknown data strucutre, probably includes CV and FX settings.
    uint8_t payload_end[1034]; //2664-398*4-38
    

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
