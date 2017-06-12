/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "NoteHandler.h"

void NoteHandler::setup() {
}

void NoteHandler::destroy() {
}

void NoteHandler::onNoteOnCallback(uint8_t *msg) {
}

void NoteHandler::onNoteOffCallback(uint8_t *msg) {
}

uint8_t NoteHandler::getLastPressedNotes(uint8_t *pitches, uint8_t *velocities, uint8_t maxNotes) {
  return 0;
}

uint8_t NoteHandler::getLastPressedNotesOrderPitch(uint8_t *pitches, uint8_t *velocities, uint8_t maxNotes) {
  return 0;
}
