/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_TASK_H__
#define MD_TASK_H__

#include "Vector.hh"
#include "Callback.hh"

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 * 
 * \addtogroup md_sysex MachineDrum Sysex Messages
 * 
 * @{
 **/

extern MDClass MD;

/**
 * \addtogroup md_task MachineDrum Sysex Task
 *
 * @{
 **/

/**
 * This task class polls the MachineDrum regularly, asking for the
 * current kit, global and pattern positions. It can be configured to
 * automatically download the sysex data for these, and call callbacks
 * when the kit/pattern/global changed.
 **/
class MDTaskClass : public Task, public MDCallback {
	/**
	 * \addtogroup md_task
	 *
	 * @{
	 **/
	
public:
	/**
	 * When set to true, this will automatically request the kit sysex
	 * data when the kit changed (default false).
	 **/
  bool autoLoadKit;
	/**
	 * When set to true, this will automatically request the global sysex
	 * data when the global changed (default false).
	 **/
  bool autoLoadGlobal;
  bool reloadKit;
  bool reloadGlobal;
  bool redisplay;
  bool verbose;

  MDTaskClass(uint16_t interval) : Task(interval) {
    redisplay = false;
    autoLoadKit = reloadKit = false;
    autoLoadGlobal = true;
    reloadGlobal = false;
    verbose = true;
  }

  CallbackVector<MDCallback, 8> kitChangeCallbacks;
  CallbackVector<MDCallback, 8> globalChangeCallbacks;
  CallbackVector<MDCallback, 8> patternChangeCallbacks;

  void addOnKitChangeCallback(MDCallback *obj, md_callback_ptr_t func) {
    kitChangeCallbacks.add(obj, func);
  }
  void removeOnKitChangeCallback(MDCallback *obj, md_callback_ptr_t func) {
    kitChangeCallbacks.remove(obj, func);
  }
  void removeOnKitChangeCallback(MDCallback *obj) {
    kitChangeCallbacks.remove(obj);
  }
  
  void addOnGlobalChangeCallback(MDCallback *obj, md_callback_ptr_t func) {
    globalChangeCallbacks.add(obj, func);
  }
  void removeOnGlobalChangeCallback(MDCallback *obj, md_callback_ptr_t func) {
    globalChangeCallbacks.remove(obj, func);
  }
  void removeOnGlobalChangeCallback(MDCallback *obj) {
    globalChangeCallbacks.remove(obj);
  }
  
  void addOnPatternChangeCallback(MDCallback *obj, md_callback_ptr_t func) {
    patternChangeCallbacks.add(obj, func);
  }
  void removeOnPatternChangeCallback(MDCallback *obj, md_callback_ptr_t func) {
    patternChangeCallbacks.remove(obj, func);
  }
  void removeOnPatternChangeCallback(MDCallback *obj) {
    patternChangeCallbacks.remove(obj);
  }
  
  void setup(uint16_t interval = 3000, bool autoLoadKit = false, bool autoLoadGlobal = true,
	     bool reloadGlobal = true);

  virtual void run();

  void onStatusResponseCallback(uint8_t type, uint8_t value);
  void onGlobalMessageCallback();
  void onKitMessageCallback();

  virtual void destroy();

	/* @} */
};

/**
 * Initialize and register the MachineDrum task.
 **/
void initMDTask();

extern MDTaskClass MDTask;
#endif /* MD_TASK_H__ */
