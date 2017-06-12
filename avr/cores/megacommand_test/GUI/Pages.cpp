/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"
#include "GUI.h"
#include "Pages.hh"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_pages GUI Pages
 *
 * @{
 *
 * \file
 * GUI Pages
 **/

void Page::update() {
}

void Page::redisplayPage() {
  GUI.setLine(GUI.LINE1);
  GUI.clearLine();
  GUI.setLine(GUI.LINE2);
  GUI.clearLine();
  redisplay = true;
}
  

void EncoderPage::update() {
  encoder_t _encoders[GUI_NUM_ENCODERS];

  USE_LOCK();
  SET_LOCK();
  m_memcpy(_encoders, Encoders.encoders, sizeof(_encoders));
  Encoders.clearEncoders();
  CLEAR_LOCK();
  
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL) 
      encoders[i]->update(_encoders + i);
  }
}

void EncoderPage::clear() {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL)
      encoders[i]->clear();
  }
}

void EncoderPage::display() {
  if (redisplay) {
    displayNames();
  }
  GUI.setLine(GUI.LINE2);
  for (uint8_t i = 0; i < 4; i++) {
    if (encoders[i] != NULL)
      if (encoders[i]->hasChanged() || redisplay || encoders[i]->redisplay) {
				encoders[i]->displayAt(i);
      }
  }
}

void EncoderPage::finalize() {
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    if (encoders[i] != NULL) 
      encoders[i]->checkHandle();
  }
}

void EncoderPage::displayNames() {
  GUI.setLine(GUI.LINE1);
  for (uint8_t i = 0; i < 4; i++) {
    if (encoders[i] != NULL)
      GUI.put_string(i, encoders[i]->getName());
    else
      GUI.put_p_string(i, PSTR("    "));
  }
}


void SwitchPage::display() {
  if (redisplay) {
    GUI.setLine(GUI.LINE1);
    GUI.put_string_at_fill(0, name);
    GUI.setLine(GUI.LINE2);
    for (int i = 0; i < 4; i++) {
      if (pages[i] != NULL) {
				GUI.put_string_fill(i, pages[i]->shortName);
      }
    }
  }
}

bool SwitchPage::handleEvent(gui_event_t *event) {
  for (int i = Buttons.ENCODER1; i <= Buttons.ENCODER4; i++) {
    if (pages[i] != NULL && EVENT_PRESSED(event, i)) {
      if (parent != NULL) {
				parent->setPage(pages[i - Buttons.ENCODER1]);
      }
      return true;
    }
  }
  return false;
}

void EncoderSwitchPage::display() {
  if (redisplay) {
    GUI.setLine(GUI.LINE1);
    GUI.clearLine();
    if (pages[0] != NULL) {
      GUI.put_string_at(0, pages[0]->name);
    }
    if (pages[3] != NULL) {
      int l = m_strlen(pages[3]->name);
      GUI.put_string_at(15 - l, pages[3]->name);
    }

    GUI.setLine(GUI.LINE2);
    GUI.clearLine();
    if (pages[1] != NULL) {
      GUI.put_string_at(0, pages[1]->name);
    }
    if (pages[2] != NULL) {
      int l = m_strlen(pages[2]->name);
      GUI.put_string_at(15 - l, pages[2]->name);
    }
  }
}

bool EncoderSwitchPage::handleEvent(gui_event_t *event) {
  for (int i = Buttons.BUTTON1; i <= Buttons.BUTTON4; i++) {
    if (pages[i] != NULL && EVENT_PRESSED(event, i)) {
      if (parent != NULL) {
				parent->setPage(pages[i - Buttons.BUTTON1]);
      }
      return true;
    }
  }
  return false;
}

void ScrollSwitchPage::addPage(Page *page) {
  pages.add(page);
  pageEncoder.max = pages.length() - 1;
}

void ScrollSwitchPage::display() {
  if (redisplay) {
    GUI.setLine(GUI.LINE1);
    GUI.put_p_string(PSTR("SELECT PAGE:"));
    GUI.setLine(GUI.LINE2);
    Page *page = pages.arr[pageEncoder.getValue()];
    if (page != NULL) {
      GUI.put_string_fill(page->name);
    }
  }
}

void ScrollSwitchPage::loop() {
  if (pageEncoder.hasChanged()) {
    redisplay = true;
  }
}

bool ScrollSwitchPage::setSelectedPage() {
  Page *page = pages.arr[pageEncoder.getValue()];
  if (page != NULL) {
    if (parent != NULL) {
      parent->setPage(page);
      return true;
    }
  }
  return false;
}

bool ScrollSwitchPage::handleEvent(gui_event_t *event) {
  Page *page = pages.arr[pageEncoder.getValue()];
  if (page != NULL) {
    if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
      if (parent != NULL) {
				parent->setPage(page);
      }
      return true;
    }
  }
  return false;
}

/* @} */
/* @} */
