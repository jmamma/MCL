/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include <GUI.h>
#include <MD.h>
#include <Vector.hh>

void mdTaskOnStatusResponseCallback(uint8_t type, uint8_t value);
void mdTaskOnKitCallback();
void mdTaskOnGlobalCallback();

void MDTaskClass::setup(uint16_t _interval, bool _autoLoadKit, bool _autoLoadGlobal, bool _reloadGlobal) {
  interval = _interval;
  autoLoadKit = _autoLoadKit;
  autoLoadGlobal = _autoLoadGlobal;
  reloadGlobal = _reloadGlobal;
  
  MDSysexListener.setup();
  MDSysexListener.addOnStatusResponseCallback
    (this, (md_status_callback_ptr_t)&MDTaskClass::onStatusResponseCallback);
  
  MDSysexListener.addOnGlobalMessageCallback
    (this, (md_callback_ptr_t)&MDTaskClass::onGlobalMessageCallback);
  MDSysexListener.addOnKitMessageCallback
    (this, (md_callback_ptr_t)&MDTaskClass::onKitMessageCallback);
}

void MDTaskClass::destroy() {
  MDSysexListener.removeOnStatusResponseCallback(this);
  MDSysexListener.removeOnGlobalMessageCallback(this);
  MDSysexListener.removeOnKitMessageCallback(this);
}

void MDTaskClass::onStatusResponseCallback(uint8_t type, uint8_t value) {
  switch (type) {
  case MD_CURRENT_KIT_REQUEST:
    if (MD.currentKit != value) {
      MD.currentKit = value;
      if (autoLoadKit) {
				MD.requestKit(MD.currentKit);
      } else {
				kitChangeCallbacks.call();
      }
    }
    if (reloadKit) {
      MD.requestKit(MD.currentKit);
      reloadKit = false;
    }
    break;
    
  case MD_CURRENT_GLOBAL_SLOT_REQUEST:
    if (MD.currentGlobal != value) {
      MD.currentGlobal = value;
      if (autoLoadGlobal) {
				MD.requestGlobal(MD.currentGlobal);
      } else {
				globalChangeCallbacks.call();
      }
		}
		if (reloadGlobal) {
			MD.requestGlobal(MD.currentGlobal);
			reloadGlobal = false;
		}
    break;

  case MD_CURRENT_PATTERN_REQUEST:
    if (MD.currentPattern != value) {
      MD.currentPattern = value;
      patternChangeCallbacks.call();
    }
    break;
  }

  redisplay = true;
}

void MDTaskClass::onGlobalMessageCallback() {
  MD.loadedGlobal = false;
  if (MD.global.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
    MD.loadedGlobal = true;
    globalChangeCallbacks.call();
  } else {
		GUI.flash_strings_fill("GLOBAL SYSEX", "ERROR");
	}
}

void MDTaskClass::onKitMessageCallback() {
  MD.loadedKit = false;
  if (MD.kit.fromSysex(MidiSysex.data + 5, MidiSysex.recordLen - 5)) {
    MD.loadedKit = true;
    if (verbose) {
      GUI.setLine(GUI.LINE1);
      GUI.flash_p_string_fill(PSTR("SWITCH KIT"));
      GUI.setLine(GUI.LINE2);
      GUI.flash_string_fill(MD.kit.name);
    }
    kitChangeCallbacks.call();
  } else {
		GUI.flash_strings_fill("KIT SYSEX", "ERROR");
  }
}

void MDTaskClass::run() {
  //MD.sendRequest(MD_STATUS_REQUEST_ID, MD_CURRENT_KIT_REQUEST);
 // MD.sendRequest(MD_STATUS_REQUEST_ID, MD_CURRENT_GLOBAL_SLOT_REQUEST);
 // MD.sendRequest(MD_STATUS_REQUEST_ID, MD_CURRENT_PATTERN_REQUEST);
}  

MDTaskClass MDTask(3000);

void initMDTask() {
  MDTask.setup();
  MDTask.autoLoadKit = true;
  MDTask.reloadGlobal = true;
  GUI.addTask(&MDTask);
}
