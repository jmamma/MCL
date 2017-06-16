/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"

#include "Events.hh"
#include "GUI.h"
#include "ModalGui.hh"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_modal Modal GUIs
 *
 * @{
 *
 * \file
 * Modal GUIs
 **/

#ifndef HOST_MIDIDUINO
class ModalGuiPage : public Page {
public:
  char line1[16];
  char line2[16];
  bool hasPressedKey;
  bool hasReleasedKey;
  int pressedKey;
  uint16_t buttonMask;
  uint16_t releaseMask;
  
  ModalGuiPage() {
    hasPressedKey = false;
    pressedKey = 0;
    line1[0] = '\0';
    line2[0] = '\0';
  }

  int getModalKey(uint16_t _buttonMask, uint16_t _releaseMask = 0) {
    buttonMask = _buttonMask;
    releaseMask = _releaseMask;
    hasPressedKey = false;

    if (GUI.sketch != NULL) {
      GUI.sketch->pushPage(this);
      while (!hasPressedKey) {
				__mainInnerLoop(true);
      }
      GUI.sketch->popPage(this);

      return pressedKey;
    } else {
      return 0;
    }
  }
  
  virtual void display() {
    if (redisplay) {
      GUI.setLine(GUI.LINE1);
      GUI.put_string_fill(line1);
      GUI.setLine(GUI.LINE2);
      GUI.put_string_fill(line2);
    }
  }

  virtual bool handleEvent(gui_event_t *event) {
    // check buttons and encoders, small hack really, because all in one byte
    for (int i = 0; i < 8; i++) {
      if (EVENT_PRESSED(event, i) && buttonMask & _BV(i)) {
				pressedKey = i;
				hasPressedKey = true;
      }
      if (EVENT_RELEASED(event, i) && releaseMask & _BV(i)) {
				pressedKey = -1;
				hasPressedKey = true;
      }
    }
    return true;
  }
};

ModalGuiPage modalGuiPage;

int showModalGui(char *line1, char *line2, uint16_t buttonMask, uint16_t releaseMask) {
  m_strncpy_fill(modalGuiPage.line1, line1, 16);
  m_strncpy_fill(modalGuiPage.line2, line2, 16);
  return modalGuiPage.getModalKey(buttonMask, releaseMask);
}

int showModalGui_p(PGM_P line1, PGM_P line2, uint16_t buttonMask, uint16_t releaseMask) {
  m_strncpy_p_fill(modalGuiPage.line1, line1, 16);
  m_strncpy_p_fill(modalGuiPage.line2, line2, 16);
  return modalGuiPage.getModalKey(buttonMask, releaseMask);
}

class NameModalGuiPage : public EncoderPage {
public:
  char line1[16];
  char name[17];
  bool hasPressedKey;
  int pressedKey;
  int cursorPos;
  bool lineChanged;

  CharEncoder charEncoders[4];
  
  
  NameModalGuiPage() {
    hasPressedKey = false;
    pressedKey = 0;
    line1[0] = '\0';
    for (int i = 0; i < 4; i++) {
      encoders[i] = &charEncoders[i];
    }
  }

  bool handleEvent(gui_event_t *event) {
    if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
      moveCursor(cursorPos - 4);
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      moveCursor(cursorPos + 4);
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      hasPressedKey = true;
      pressedKey = 0;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      hasPressedKey = true;
      pressedKey = 1;
    }
    return true;
  }

  void show() {
    moveCursor(0);
    LCD.blinkCursor(true);
    LCD.moveCursor(1, cursorPos);
  }

  void hide() {
    LCD.moveCursor(1, cursorPos);
    LCD.blinkCursor(false);
    delay(10);
  }

  virtual void loop() {
    for (int i = 0; i < 4; i++) {
      if (charEncoders[i].hasChanged()) {
				MidiUart.sendCC(2, i);
				name[cursorPos + i] = charEncoders[i].getChar();
				lineChanged = true;
      }
    }
  }

  virtual void finalize() {
    EncoderPage::finalize();
    if (lineChanged) {
      LCD.blinkCursor(true);
      LCD.moveCursor(1, cursorPos);
      lineChanged = false;
    }
  }

  char *getName(char *initName) {
    if (initName != NULL) {
      m_strncpy_fill(name, initName, 16);
    } else {
      m_strncpy_fill(name, (char *)"", 16);
    }
    
    cursorPos = 0;
    hasPressedKey = false;
    pressedKey = 0;
    lineChanged = true;

    if (GUI.sketch != NULL) {
      GUI.sketch->pushPage(this);

      while (hasPressedKey == false) {
				__mainInnerLoop(true);
      }
      GUI.sketch->popPage(this);

      if (pressedKey == 1) {
				return NULL;
      } else {
				return name;
      }
    } else {
      return NULL;
    }
  }

  void moveCursor(int pos) {
    if (pos < 0)
      pos = 0;
    if (pos > 12)
      pos = 12;
    cursorPos = pos;
    for (int i = 0; i < 4; i++) {
      charEncoders[i].setChar(name[cursorPos + i]);
    }
    LCD.moveCursor(1, cursorPos);
  }
  
  virtual void display() {
    if (redisplay || lineChanged) {
      MidiUart.sendCC(0, redisplay ? 1 : 0);
      MidiUart.sendCC(1, lineChanged ? 1 : 0);
      GUI.setLine(GUI.LINE1);
      GUI.put_string_fill(line1);
      GUI.setLine(GUI.LINE2);
      GUI.put_string_fill(name);
    }
  }
};

NameModalGuiPage nameModalGuiPage;

char *getNameModalGui(char *line1, char *initName) {
  m_strncpy(nameModalGuiPage.line1, line1, 16);
  return nameModalGuiPage.getName(initName);
}
#endif
