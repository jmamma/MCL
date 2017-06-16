/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "Sketch.hh"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_sketch Sketch class
 *
 * @{
 **/

void SketchSwitchPage::display() {
  if (redisplay) {
    for (int i = 0; i < 4; i++) {
      char n1[5], n2[5];
      if (sketches[i] != NULL) {
				sketches[i]->getName(n1, n2);
				GUI.setLine(GUI.LINE1);
				GUI.put_string(i, n1);
				GUI.setLine(GUI.LINE2);
				GUI.put_string(i, n2);
      }
    }
  }
}

bool SketchSwitchPage::handleEvent(gui_event_t *event) {
  for (int i = Buttons.ENCODER1; i <= Buttons.ENCODER4; i++) {
    if (sketches[i] != NULL) {
      if (EVENT_PRESSED(event, i)) {
				if (BUTTON_DOWN(Buttons.BUTTON1)) {
					sketches[i]->mute(true);
				} else if (BUTTON_DOWN(Buttons.BUTTON2)) {
					sketches[i]->doExtra(true);
				} else if (BUTTON_DOWN(Buttons.BUTTON3)) {
					if (tmpPage != NULL)
						GUI.popPage(tmpPage);
					tmpPage = sketches[i]->getPage(0);
					if (tmpPage != NULL)
						GUI.pushPage(tmpPage);
				} else if (BUTTON_DOWN(Buttons.BUTTON4)) {
					if (tmpPage != NULL)
						GUI.popPage(tmpPage);
					tmpPage = sketches[i]->getPage(1);
					if (tmpPage != NULL)
						GUI.pushPage(tmpPage);
				} else {
					GUI.setSketch(sketches[i]);
				}
				return true;
      } else if (EVENT_RELEASED(event, i)) {
				if (BUTTON_DOWN(Buttons.BUTTON2)) {
					sketches[i]->doExtra(false);
				} else if (BUTTON_DOWN(Buttons.BUTTON1)) {
					sketches[i]->mute(false);
				}
				return true;
      }
    }
  }

  return false;
}

bool SketchSwitchPage::handlePopEvent(gui_event_t *event) {
  if (GUI.sketch == &_defaultSketch) {
    if (EVENT_RELEASED(event, Buttons.BUTTON3) || EVENT_RELEASED(event, Buttons.BUTTON4)) {
      if (tmpPage != NULL) {
				GUI.popPage(tmpPage);
				tmpPage = NULL;
				return true;
      }
    } else {
      return false;
    }
  } else {
    return false;
  }

	return false;
}

bool SketchSwitchPage::handleGlobalEvent(gui_event_t *event) {
  if (handlePopEvent(event))
    return true;
  
  bool allButtonsDown = true;
  bool aButtonPressed = false;
  for (uint8_t i = Buttons.BUTTON1; i < Buttons.BUTTON4; i++) {
    if (EVENT_PRESSED(event, i)) {
      aButtonPressed = true;
    }
    if (!BUTTON_DOWN(i)) {
      allButtonsDown = false;
    }
  }
  if (allButtonsDown && aButtonPressed) {
    GUI.setSketch(&_defaultSketch);
    GUI.setPage(this);
    return true;
  } else {
    return false;
  }
}

/* @} */
/* @} */

