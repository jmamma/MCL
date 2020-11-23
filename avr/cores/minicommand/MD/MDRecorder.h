/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MDRECORDER_H__
#define MDRECORDER_H__

#include "WProgram.h"
#include "ListPool.hh"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_recorder MachineDrum Recorder
 * 
 * @{
 **/


/**
 * Stores a note send to the machinedrum.
 **/
typedef struct md_recorder_event_s {
	/**
	 * \addtogroup md_recorder
	 * @{
	 **/

  uint8_t channel;
  uint8_t pitch;
  uint8_t value;
  uint8_t step;

	/* @} */
} md_recorder_event_t;

typedef enum {
  MD_PLAYBACK_NONE = 0,
  MD_PLAYBACK_HITS = 1,
  MD_PLAYBACK_CCS = 2
} md_playback_phase_t;

/**
 * This class records notes sent to the MachineDrum. It is in an early
 * stage and will definitely change in the future.
 **/
class MDRecorderClass : public MidiCallback, public ClockCallback {
	/**
	 * \addtogroup md_recorder
	 * @{
	 **/

 public:
  MDRecorderClass();

  uint8_t rec16th_counter;
  uint8_t recordLength;
  bool recordingTriggered;
  uint8_t recordingBoundary;
  bool recording;

  bool looping;
  uint8_t play16th_counter;
  bool playbackTriggered;
  uint8_t playbackBoundary;
  bool playing;
  md_playback_phase_t md_playback_phase;

  bool muted;
  
  void setup();
  
  ListWithPool<md_recorder_event_t, 128> eventList;
  ListElt<md_recorder_event_t> *playPtr;

  void startRecord(uint8_t length, uint8_t boundary = 0);
  void stopRecord();
  void startPlayback(uint8_t boundary = 0);
  void startMDPlayback(uint8_t boundary = 0);
  void stopPlayback();

  void onNoteOnCallback(uint8_t *msg);
  void onCCCallback(uint8_t *msg);
  void on16Callback();

	/* @} */
};

/* @} @} */

extern MDRecorderClass MDRecorder;

#endif /* MDRECORDER_H__ */
