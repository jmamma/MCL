#pragma once

#include "Arduino.h"
#include "GUI/PageIndex.h"

enum PageSelectIcon : uint8_t {
  PAGE_ICON_NONE = 0,
  PAGE_ICON_GRID,
  PAGE_ICON_MIXER,
  PAGE_ICON_PERF,
  PAGE_ICON_ROUTE,
  PAGE_ICON_STEP,
  PAGE_ICON_LFO,
  PAGE_ICON_PIANOROLL,
  PAGE_ICON_CHROMA,
  PAGE_ICON_SAMPLE,
  PAGE_ICON_WAVD,
  PAGE_ICON_RHYTMECHO,
  PAGE_ICON_GATEBOX,
  PAGE_ICON_RAM1,
  PAGE_ICON_RAM2,
};

struct PageSelectEntry {
  const char *Name;
  PageIndex Page;
  // Packed as: bits 0..9 icon resource offset + 1, 10..14 height.
  uint16_t IconMeta;
};
