#include "MNM.h"
#include "Vector.hh"

void mnmTaskOnStatusResponseCallback(uint8_t type, uint8_t value);
void mnmTaskOnKitCallback();
void mnmTaskOnGlobalCallback();

void MNMTaskClass::setup(uint16_t _interval, bool _autoLoadKit, bool _autoLoadGlobal, bool _reloadGlobal) {
  interval = _interval;
  autoLoadKit = _autoLoadKit;
  autoLoadGlobal = _autoLoadGlobal;
  reloadGlobal = _reloadGlobal;

  MNMSysexListener.setup();
  MNMSysexListener.addOnStatusResponseCallback
    (this, (mnm_status_callback_ptr_t)&MNMTaskClass::onStatusResponseCallback);
  MNMSysexListener.addOnGlobalMessageCallback
    (this, (mnm_callback_ptr_t)&MNMTaskClass::onGlobalMessageCallback);
  MNMSysexListener.addOnKitMessageCallback
    (this, (mnm_callback_ptr_t)&MNMTaskClass::onKitMessageCallback);
}

void MNMTaskClass::destroy() {
  MNMSysexListener.removeOnStatusResponseCallback(this);
  MNMSysexListener.removeOnGlobalMessageCallback(this);
  MNMSysexListener.removeOnKitMessageCallback(this);
}

void MNMTaskClass::onStatusResponseCallback(uint8_t type, uint8_t value) {
  switch (type) {
  case MNM_CURRENT_KIT_REQUEST:
    if (MNM.currentKit != value) {
      MNM.currentKit = value;
      if (autoLoadKit) {
				MNM.requestKit(MNM.currentKit);
      } else {
				kitChangeCallbacks.call();
      }
    }
    if (reloadKit) {
      MNM.requestKit(MNM.currentKit);
      reloadKit = false;
    }
    break;
    
  case MNM_CURRENT_GLOBAL_SLOT_REQUEST:
    if (MNM.currentGlobal != value) {
      MNM.currentGlobal = value;
      if (autoLoadGlobal) {
				MNM.requestGlobal(MNM.currentGlobal);
      } else {
				globalChangeCallbacks.call();
      }
    }
    if (reloadGlobal) {
      MNM.requestGlobal(MNM.currentGlobal);
      reloadGlobal = false;
    }
    break;

  case MNM_CURRENT_PATTERN_REQUEST:
    if (MNM.currentPattern != value) {
      MNM.currentPattern = value;
      patternChangeCallbacks.call();
    }
    break;

  case MNM_CURRENT_AUDIO_TRACK_REQUEST:
    if (MNM.currentTrack != value) {
      MNM.currentTrack = value;
      currentTrackChangeCallbacks.call();
    }
    break;
  }
  redisplay = true; 
}

void MNMTaskClass::onGlobalMessageCallback() {
  MNM.loadedGlobal = false;
  if (MNM.global.fromSysex(MidiSysex.data, MidiSysex.recordLen)) {
    MNM.loadedGlobal = true;
    globalChangeCallbacks.call();
  }
}

void MNMTaskClass::onKitMessageCallback() {
  MNM.loadedKit = false;
  if (MNM.kit.fromSysex(MidiSysex.data, MidiSysex.recordLen)) {
    MNM.loadedKit = true;
    kitChangeCallbacks.call();
    if (verbose) {
      GUI.setLine(GUI.LINE1);
      GUI.flash_p_string_fill(PSTR("SWITCH KIT"));
      GUI.setLine(GUI.LINE2);
      GUI.flash_string_fill(MNM.kit.name);
    }
  } else {
  }
}

MNMTaskClass MNMTask(3000);

void initMNMTask() {
  MNMTask.setup();
  MNMTask.autoLoadKit = true;
  MNMTask.reloadGlobal = true;
  GUI.addTask(&MNMTask);
}
