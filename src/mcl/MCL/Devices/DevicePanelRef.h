#pragma once

#include "platform.h"
#include <stdint.h>

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#else
#include "../Drivers/MidiDevice.h"
#include "Devices/DeviceManager.h"
#endif

namespace DevicePanelRef {

#if defined(PLATFORM_DESKTOP) || defined(PLATFORM_WASM)
#define DEVICE_PANEL_REF_INLINE static ALWAYS_INLINE()
#else
#define DEVICE_PANEL_REF_INLINE ALWAYS_INLINE() static inline
#endif

DEVICE_PANEL_REF_INLINE void set_primary_key_repeat(uint8_t enabled) {
#if defined(__AVR__)
  MD.set_key_repeat(enabled);
#else
  device_manager.primary_device()->panel()->set_key_repeat(enabled);
#endif
}

DEVICE_PANEL_REF_INLINE void popup_text(uint8_t action_string,
                                        uint8_t persistent = 0) {
#if defined(__AVR__)
  MD.popup_text(action_string, persistent);
#else
  device_manager.primary_device()->panel()->popup_text(action_string,
                                                       persistent);
#endif
}

DEVICE_PANEL_REF_INLINE void popup_text(char *text,
                                        uint8_t persistent = 0) {
#if defined(__AVR__)
  MD.popup_text(text, persistent);
#else
  device_manager.primary_device()->panel()->popup_text(text, persistent);
#endif
}

DEVICE_PANEL_REF_INLINE void popup_text_P(const char *text_P,
                                          uint8_t persistent = 0) {
#if defined(__AVR__)
  MD.popup_text_P(text_P, persistent);
#else
  device_manager.primary_device()->panel()->popup_text_P(text_P, persistent);
#endif
}

DEVICE_PANEL_REF_INLINE void popup_text_P(const char *text1_P,
                                          const char *text2_P,
                                          uint8_t persistent = 0) {
#if defined(__AVR__)
  MD.popup_text_P(text1_P, text2_P, persistent);
#else
  device_manager.primary_device()->panel()->popup_text_P(text1_P, text2_P,
                                                         persistent);
#endif
}

#undef DEVICE_PANEL_REF_INLINE

} // namespace DevicePanelRef
