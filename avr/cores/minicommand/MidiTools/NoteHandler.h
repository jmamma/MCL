/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef NOTEHANDLER_H__
#define NOTEHANDLER_H__

#include "WProgram.h"
#include "ListPool.hh"

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
 * \addtogroup midi_note_handler Midi Note Handler
 *
 * @{
 **/

typedef struct incoming_note_s {
	/**
	 * \addtogroup midi_note_handler 
	 *
	 * @{
	 **/
	
  uint8_t channel;
  uint8_t pitch;
  uint8_t velocity;

	/* @} */
} incoming_note_t;

class NoteHandler {
	/**
	 * \addtogroup midi_note_handler 
	 *
	 * @{
	 **/
	
 public:
  ListPool<incoming_note_t, 8> incomingNotes;

  NoteHandler() {
  }

  void setup();
  void destroy();
  uint8_t getLastPressedNotes(uint8_t *pitches, uint8_t *velocities, uint8_t maxNotes = 8);
  uint8_t getLastPressedNotesOrderPitch(uint8_t *pitches, uint8_t *velocities, uint8_t maxNotes = 8);
  void onNoteOnCallback(uint8_t *msg);
  void onNoteOffCallback(uint8_t *msg);

	/* @} */
	
};

/* @} @} @} */

#endif /* NOTEHANDLER_H__ */
