#ifndef MNM_TASK_H__
#define MNM_TASK_H__

#include "Vector.hh"

extern MNMClass MNM;

class MNMTaskClass : public Task, public MNMCallback {
public:
  bool autoLoadKit;
  bool autoLoadGlobal;
  bool autoLoadPattern;
  bool reloadKit;
  bool reloadGlobal;
  bool redisplay;
	bool verbose;

  MNMTaskClass(uint16_t interval) : Task(interval) {
    redisplay = false;
    autoLoadKit = reloadKit = false;
    autoLoadGlobal = true;
		autoLoadPattern = false;
    reloadGlobal = false;
		verbose = true;
  }

  CallbackVector<MNMCallback, 8>kitChangeCallbacks;
  CallbackVector<MNMCallback, 8>globalChangeCallbacks;
  CallbackVector<MNMCallback, 8>patternChangeCallbacks;
  CallbackVector<MNMCallback, 8>currentTrackChangeCallbacks;

  void addOnKitChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    kitChangeCallbacks.add(obj, func);
  }
  void removeOnKitChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    kitChangeCallbacks.remove(obj, func);
  }
  void removeOnKitChangeCallback(MNMCallback *obj) {
    kitChangeCallbacks.remove(obj);
  }
  
  void addOnGlobalChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    globalChangeCallbacks.add(obj, func);
  }
  void removeOnGlobalChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    globalChangeCallbacks.remove(obj, func);
  }
  void removeOnGlobalChangeCallback(MNMCallback *obj) {
    globalChangeCallbacks.remove(obj);
  }
  
  void addOnPatternChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    patternChangeCallbacks.add(obj, func);
  }
  void removeOnPatternChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    patternChangeCallbacks.remove(obj, func);
  }
  void removeOnPatternChangeCallback(MNMCallback *obj) {
    patternChangeCallbacks.remove(obj);
  }
  
  void addOnCurrentTrackChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    currentTrackChangeCallbacks.add(obj, func);
  }
  void removeOnCurrentTrackChangeCallback(MNMCallback *obj, mnm_callback_ptr_t func) {
    currentTrackChangeCallbacks.remove(obj, func);
  }
  void removeOnCurrentTrackChangeCallback(MNMCallback *obj) {
    currentTrackChangeCallbacks.remove(obj);
  }
  
  void setup(uint16_t interval = 3000, bool autoLoadKit = false, bool autoLoadGlobal = true,
	     bool reloadGlobal = true);

  virtual void run() {
		if (autoLoadKit) {
			MNM.sendRequest(MNM_STATUS_REQUEST_ID, MNM_CURRENT_KIT_REQUEST);
		}
		if (autoLoadGlobal) {
			MNM.sendRequest(MNM_STATUS_REQUEST_ID, MNM_CURRENT_GLOBAL_SLOT_REQUEST);
		}
		if (autoLoadPattern) {
			MNM.sendRequest(MNM_STATUS_REQUEST_ID, MNM_CURRENT_PATTERN_REQUEST);
		}
    MNM.sendRequest(MNM_STATUS_REQUEST_ID, MNM_CURRENT_AUDIO_TRACK_REQUEST);
  }

  void onStatusResponseCallback(uint8_t type, uint8_t value);
  void onGlobalMessageCallback();
  void onKitMessageCallback();

  virtual void destroy();
};

void initMNMTask();

extern MNMTaskClass MNMTask;

#endif /* MNM_TASK_H__ */
