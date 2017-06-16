/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"
#include "helpers.h"
#include "Arpeggiator.hh"

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

const char *retrig_names[RETRIG_CNT] = {
  "OFF ",
  "NOTE",
  "BEAT"
};

const char *arp_names[ARP_STYLE_CNT] = {
	"UP     ",
	"DOWN   ",
	"UPDOWN ",
	"DOWNUP ",
	"UP&DOWN",
	"DOWN&UP",
	"CONVERG",
	"DIVERGE",
	"CONDIV ",
	"PINK_UP",
	"PINK_UD",
	"THMB_UP",
	"THMB_UD",
	"RANDOM ",
	"RANDOM1",
	"ORDER  "
};
  

ArpeggiatorClass::ArpeggiatorClass() {
  numNotes = 0;
  arpLen = 0;
  arpStep = 0;
  arpCount = 0;
  arpTimes = 0;
  arpStyle = ARP_STYLE_UP;
  arpRetrig = RETRIG_OFF;
  arpSpeed = 0;
  speedCounter = 0;
  arpTrack = 0;
  retrigSpeed = 1;
  arpOctaves = 0;
  arpOctaveCount = 0;
  muted = false;

  for (int i = 0; i < NUM_NOTES; i++) {
    orderedNotes[i] = 128;
  }
}

void ArpeggiatorClass::setup() {
  Midi2.addOnNoteOnCallback(this, (midi_callback_ptr_t)&ArpeggiatorClass::onNoteOnCallback);
  Midi2.addOnNoteOffCallback(this, (midi_callback_ptr_t)&ArpeggiatorClass::onNoteOffCallback);
}

void ArpeggiatorClass::onNoteOffCallback(uint8_t *msg) {
  removeNote(msg[1]);
}

void ArpeggiatorClass::onNoteOnCallback(uint8_t *msg) {
  if (msg[2] != 0) {
    addNote(msg[1], msg[2]);
  } else {
    removeNote(msg[1]);
  }
}
  
void ArpeggiatorClass::retrigger() {
  arpStep = 0;
  arpCount = 0;
  speedCounter = 0;
  arpOctaveCount = 0;
  if (arpStyle == ARP_STYLE_RANDOM_ONCE) {
    calculateArp();
  }
}

void ArpeggiatorClass::bubbleSortUp() {
  bool completed = true;
  do {
    completed = true;
    for (int i = 0; i < numNotes-1; i++) {
      if (orderedNotes[i] > orderedNotes[i+1]) {
				completed = false;
				uint8_t tmp = orderedNotes[i];
				orderedNotes[i] = orderedNotes[i+1];
				orderedNotes[i+1] = tmp;
				tmp = orderedVelocities[i];
				orderedVelocities[i] = orderedVelocities[i+1];
				orderedVelocities[i+1] = tmp;
      }
    }
  } while (!completed);
}

void ArpeggiatorClass::bubbleSortDown() {
  bool completed = true;
  do {
    completed = true;
    for (int i = 0; i < numNotes-1; i++) {
      if (orderedNotes[i] < orderedNotes[i+1]) {
				completed = false;
				uint8_t tmp = orderedNotes[i];
				orderedNotes[i] = orderedNotes[i+1];
				orderedNotes[i+1] = tmp;
				tmp = orderedVelocities[i];
				orderedVelocities[i] = orderedVelocities[i+1];
				orderedVelocities[i+1] = tmp;
      }
    }
  } while (!completed);
}

void ArpeggiatorClass::calculateArp() {
  USE_LOCK();
  SET_LOCK();

  arpStep = 0;
  switch (arpStyle) {
  case ARP_STYLE_UP:
    bubbleSortUp();
    m_memcpy(arpNotes, orderedNotes, numNotes);
    m_memcpy(arpVelocities, orderedVelocities, numNotes);
    arpLen = numNotes;
    break;
    
  case ARP_STYLE_DOWN:
    bubbleSortDown();
    m_memcpy(arpNotes, orderedNotes, numNotes);
    m_memcpy(arpVelocities, orderedVelocities, numNotes);
    arpLen = numNotes;
    break;
    
  case ARP_STYLE_ORDER: {
    m_memcpy(arpNotes, orderedNotes, numNotes);
    m_memcpy(arpVelocities, orderedVelocities, numNotes);
    arpLen = numNotes;
  }
    break;
    
  case ARP_STYLE_UPDOWN:
    if (numNotes > 1) {
      bubbleSortUp();
      m_memcpy(arpNotes, orderedNotes, numNotes);
      m_memcpy(arpVelocities, orderedVelocities, numNotes);
      for (int i = 0; i < numNotes - 2; i++) {
				arpNotes[numNotes + i] = orderedNotes[numNotes - 2 - i];
				arpVelocities[numNotes + i] = arpVelocities[numNotes - 2 - i];
      }
      arpLen = numNotes + numNotes - 2;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
  
  case ARP_STYLE_DOWNUP:
    if (numNotes > 1) {
      bubbleSortDown();
      m_memcpy(arpNotes, orderedNotes, numNotes);
      m_memcpy(arpVelocities, orderedVelocities, numNotes);
      for (int i = 0; i < numNotes - 2; i++) {
				arpNotes[numNotes + i] = orderedNotes[numNotes - 2 - i];
				arpVelocities[numNotes + i] = arpVelocities[numNotes - 2 - i];
      }
      arpLen = numNotes + numNotes - 2;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_UP_AND_DOWN:
    if (numNotes > 1) {
      bubbleSortUp();
      m_memcpy(arpNotes, orderedNotes, numNotes);
      m_memcpy(arpVelocities, orderedVelocities, numNotes);
      for (int i = 0; i < numNotes; i++) {
				arpNotes[numNotes + i] = orderedNotes[numNotes - 1 - i];
				arpVelocities[numNotes + i] = arpVelocities[numNotes - 1 - i];
      }
      arpLen = numNotes + numNotes;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_DOWN_AND_UP:
    if (numNotes > 1) {
      bubbleSortDown();
      m_memcpy(arpNotes, orderedNotes, numNotes);
      m_memcpy(arpVelocities, orderedVelocities, numNotes);
      for (int i = 0; i < numNotes; i++) {
				arpNotes[numNotes + i] = orderedNotes[numNotes - 1 - i];
				arpVelocities[numNotes + i] = arpVelocities[numNotes - 1 - i];
      }
      arpLen = numNotes + numNotes;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_CONVERGE:
    bubbleSortUp();
    if (numNotes > 1) {
      arpLen = 0;
      for (int i = 0; i < (numNotes >> 1); i++) {
				arpNotes[arpLen] = orderedNotes[i];
				arpVelocities[arpLen] = orderedVelocities[i];
				arpLen++;
				arpNotes[arpLen] = orderedNotes[numNotes-i-1];
				arpVelocities[arpLen] = orderedVelocities[numNotes-i-1];
				arpLen++;
      }
      if (numNotes & 1) {
				arpNotes[arpLen] = orderedNotes[(numNotes >> 1)];
				arpVelocities[arpLen] = orderedVelocities[(numNotes >> 1)];
				arpLen++;
      }
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_DIVERGE:
    bubbleSortUp();
    if (numNotes > 1) {
      arpLen = 0;
      if (numNotes & 1) {
				arpNotes[arpLen] = orderedNotes[numNotes >> 1];
				arpVelocities[arpLen] = orderedVelocities[numNotes >> 1];
				arpLen++;
      }
      for (int i = (numNotes >> 1) - 1; i >= 0; i--) {
				arpNotes[arpLen] = orderedNotes[i];
				arpVelocities[arpLen] = orderedVelocities[i];
				arpLen++;
				arpNotes[arpLen] = orderedNotes[numNotes-i-1];
				arpVelocities[arpLen] = orderedVelocities[numNotes-i-1];
				arpLen++;
      }
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
      
  case ARP_STYLE_CON_AND_DIVERGE:
    bubbleSortUp();
    if (numNotes > 1) {
      arpLen = 0;
      for (int i = 0; i < (numNotes >> 1); i++) {
				arpNotes[arpLen] = orderedNotes[i];
				arpVelocities[arpLen] = orderedVelocities[i];
				arpLen++;
				arpNotes[arpLen] = orderedNotes[numNotes-i-1];
				arpVelocities[arpLen] = orderedVelocities[numNotes-i-1];
				arpLen++;
      }
      if (numNotes & 1) {
				arpNotes[arpLen] = orderedNotes[(numNotes >> 1)];
				arpVelocities[arpLen] = orderedVelocities[(numNotes >> 1)];
				arpLen++;
      }
      for (int i = (numNotes >> 1) - 1; i >= 0; i--) {
				arpNotes[arpLen] = orderedNotes[i];
				arpVelocities[arpLen] = orderedVelocities[i];
				arpLen++;
				arpNotes[arpLen] = orderedNotes[numNotes-i-1];
				arpVelocities[arpLen] = orderedVelocities[numNotes-i-1];
				arpLen++;
      }
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_PINKY_UP:
    bubbleSortUp();
    if (numNotes > 1) {
      for (int i = 0; i < numNotes-1; i++) {
				arpNotes[i*2] = orderedNotes[numNotes-1];
				arpNotes[i*2+1] = orderedNotes[i];
      }
      arpLen = numNotes * 2;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_PINKY_UPDOWN:
    bubbleSortUp();
    if (numNotes > 1) {
      for (int i = 0; i < numNotes-1; i++) {
				arpNotes[i*2] = orderedNotes[numNotes-1];
				arpNotes[i*2+1] = orderedNotes[i];
      }
      for (int i = 0; i < numNotes-1; i++) {
				arpNotes[i*2 + 2 * numNotes] = orderedNotes[numNotes-1];
				arpNotes[i*2+1 + 2 * numNotes] = orderedNotes[numNotes - i - 2];
      }
      arpLen = numNotes * 4;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_THUMB_UP:
    bubbleSortUp();
    if (numNotes > 1) {
      for (int i = 1; i < numNotes; i++) {
				arpNotes[i*2] = orderedNotes[0];
				arpNotes[i*2+1] = orderedNotes[i];
      }
      arpLen = numNotes * 2;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_THUMB_UPDOWN:
    bubbleSortUp();
    if (numNotes > 1) {
      for (int i = 1; i < numNotes; i++) {
				arpNotes[i*2] = orderedNotes[0];
				arpNotes[i*2+1] = orderedNotes[i];
      }
      for (int i = 0; i < numNotes-1; i++) {
				arpNotes[i*2 + 2 * numNotes] = orderedNotes[0];
				arpNotes[i*2+1 + 2 * numNotes] = orderedNotes[numNotes - i - 1];
      }
      arpLen = numNotes * 4;
    } else {
      arpNotes[0] = orderedNotes[0];
      arpVelocities[0] = orderedVelocities[0];
      arpLen = 1;
    }
    break;
   
  case ARP_STYLE_RANDOM:
    arpLen = numNotes;
    break;
   
  case ARP_STYLE_RANDOM_ONCE:
    m_memcpy(arpNotes, orderedNotes, numNotes);
    m_memcpy(arpVelocities, orderedVelocities, numNotes);
    for (int i = 0; i < numNotes; i++) {
      uint8_t rand = random() % numNotes;
      uint8_t tmp;
      tmp = arpNotes[i];
      arpNotes[i] = arpNotes[rand];
      arpNotes[rand] = tmp;
      tmp = arpVelocities[i];
      arpVelocities[i] = arpVelocities[rand];
      arpVelocities[i] = tmp;
    }
    arpLen = numNotes;
    break;
   
  default:
    break;
  }

  CLEAR_LOCK();
}

void ArpeggiatorClass::reorderNotes() {
  uint8_t write = 0;
  for (int i = 0; i < NUM_NOTES; i++) {
    if (orderedNotes[i] != 128) {
      orderedNotes[write] = orderedNotes[i];
      orderedVelocities[write] = orderedVelocities[i];
      if (i != write)
				orderedNotes[i] = 128;
      write++;
    }
  }
}

void ArpeggiatorClass::addNote(uint8_t pitch, uint8_t velocity) {
  // replace
  bool replaced = false;
  for (int i = 0; i < NUM_NOTES; i++) {
    if (orderedNotes[i] == pitch) {
      orderedVelocities[i] = velocity;
      replaced = true;
    }
  }
  if (replaced)
    return;

  uint8_t replaceNote = 128;
  if (numNotes == NUM_NOTES) {
    orderedNotes[0] = 128;
    numNotes--;
    reorderNotes();
  }

  orderedNotes[numNotes] = pitch;
  orderedVelocities[numNotes] = velocity;
  numNotes++;
    
  calculateArp();
  if (arpRetrig == RETRIG_NOTE || numNotes == 1) {
    retrigger();
  }      
}

void ArpeggiatorClass::removeNote(uint8_t pitch) {
  for (int i = 0; i < NUM_NOTES; i++) {
    if (orderedNotes[i] == pitch) {
      orderedNotes[i] = 128;
      reorderNotes();
      numNotes--;
      break;
    }
  }
  calculateArp();
}


/* @} @} @} */
