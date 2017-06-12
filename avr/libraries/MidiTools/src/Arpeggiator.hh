/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef ARPEGGIATOR_H__
#define ARPEGGIATOR_H__

#include "WProgram.h"

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_tools Midi Tools
 *
 * @{
 **/

/**
 * \addtogroup midi_arpeggiator Midi Arpeggiator Class
 *
 * @{
 **/

typedef enum {
	/**
	 * \addtogroup midi_arpeggiator 
	 *
	 * @{
	 **/
	
  ARP_STYLE_UP = 0,
  ARP_STYLE_DOWN,
  ARP_STYLE_UPDOWN,
  ARP_STYLE_DOWNUP,
  ARP_STYLE_UP_AND_DOWN,
  ARP_STYLE_DOWN_AND_UP,
  ARP_STYLE_CONVERGE,
  ARP_STYLE_DIVERGE,
  ARP_STYLE_CON_AND_DIVERGE,
  ARP_STYLE_PINKY_UP,
  ARP_STYLE_PINKY_UPDOWN,
  ARP_STYLE_THUMB_UP,
  ARP_STYLE_THUMB_UPDOWN,
  ARP_STYLE_RANDOM,
  ARP_STYLE_RANDOM_ONCE,
  ARP_STYLE_ORDER,
  ARP_STYLE_CNT

	/* @} */
} arp_style_t;

typedef enum {
	/**
	 * \addtogroup midi_arpeggiator 
	 *
	 * @{
	 **/
	
  RETRIG_OFF = 0,
  RETRIG_NOTE,
  RETRIG_BEAT,
  RETRIG_CNT

	/* @} */
} arp_retrig_type_t;

extern const char *retrig_names[RETRIG_CNT];
extern const char *arp_names[ARP_STYLE_CNT];

#define NUM_NOTES 8
#define MAX_ARP_LEN 64

class ArpeggiatorClass : public MidiCallback {
	/**
	 * \addtogroup midi_arpeggiator 
	 *
	 * @{
	 **/
	
public:
  uint8_t notes[NUM_NOTES];
  uint8_t velocities[NUM_NOTES];

  uint8_t orderedNotes[NUM_NOTES];
  uint8_t orderedVelocities[NUM_NOTES];
  uint8_t numNotes;

  uint8_t arpNotes[MAX_ARP_LEN];
  uint8_t arpVelocities[MAX_ARP_LEN];
  
  uint8_t arpSpeed;
  uint8_t arpLen;
  uint8_t arpStep;
  uint8_t arpCount;
  uint8_t arpTrack;
  uint8_t arpTimes;
  uint8_t arpOctaves;
  uint8_t arpOctaveCount;
  uint8_t retrigSpeed;

  bool muted;

  arp_style_t arpStyle;
  arp_retrig_type_t arpRetrig;

  uint16_t speedCounter;

  ArpeggiatorClass();
  void setup();
  void retrigger();
  void bubbleSortUp();
  void bubbleSortDown();
  void calculateArp();
  void reorderNotes();
  void addNote(uint8_t pitch, uint8_t velocity);
  void removeNote(uint8_t pitch);

  void onNoteOffCallback(uint8_t *msg);
  void onNoteOnCallback(uint8_t *msg);

	/* @} */
};

class MDArpeggiatorClass : public ArpeggiatorClass, public ClockCallback {
	/**
	 * \addtogroup midi_arpeggiator 
	 *
	 * @{
	 **/
	
public:
  uint8_t recordPitches[64];
  int recordLength;
  int recordStart;
  bool triggerRecording;
  bool recording;
  bool endRecording;

  MDArpeggiatorClass() : ArpeggiatorClass() {
    recordLength = 32;
    recordStart = 0;
    triggerRecording = false;
    recording = false;
    endRecording = false;
  }

  void setup();
  
  /* recording */
  void recordNote(int pos, uint8_t track, uint8_t note, uint8_t velocity);
  void recordNoteSecond(int pos, uint8_t track);
  void playNext(uint32_t _my16thpos, bool recording = false);
  void on16Callback();
  void startRecording();

	/* @} */
};

/* @} @} @} */

#endif /* ARPEGGIATOR_H__ */
