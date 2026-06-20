#include "GUI/Pages/PlatformPanel.h"

#ifdef MCL_HAS_EXTENDED_PANEL_INPUT

#ifdef MCL_HAS_TBD_DRIVER

#include "TBD/UI/TbdPanel.h"

PlatformPanel platform_panel;

void PlatformPanel::loop() {
  tbd_panel.loop();
}

bool PlatformPanel::handleEvent(gui_event_t *event) {
  return tbd_handleEvent(event);
}

#else

#include "../../../Drivers/MD/MD.h"
#include "../../../Drivers/MidiDevice.h"
#include "Devices/DeviceManager.h"
#include "GUI/Pages/Grid/GridPage.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "GUI_hardware.h"
#include "KeyInterface.h"
#include "MCL.h"
#include "MidiClock.h"
#include "NoteInterface.h"
#include "GUI/Pages/Sequencer/SeqPages.h"
#include "helpers.h"

PlatformPanel platform_panel;

namespace {

static bool is_seq_page(PageIndex pg) {
  return pg == SEQ_STEP_PAGE || pg == SEQ_EXTSTEP_PAGE || pg == SEQ_PTC_PAGE;
}

static bool b_button_is_scale(PageIndex pg) {
  switch (pg) {
  case GRID_PAGE:
    return GUI.currentPage() == mcl.getPage(GRID_PAGE) &&
           !grid_page.show_slot_menu;
  case MIXER_PAGE:
  case GRID_SAVE_PAGE:
  case GRID_LOAD_PAGE:
    return true;
  case SEQ_STEP_PAGE:
  case SEQ_EXTSTEP_PAGE:
  case SEQ_PTC_PAGE:
    return !SeqPage::show_seq_menu;
  default:
    return false;
  }
}

static void handle_local_transport(uint8_t msg) {
  switch (msg) {
  case MIDI_START:
    MidiClock.handleImmediateMidiStart();
    MidiClock.handleMidiStart();
    break;
  case MIDI_STOP:
    MidiClock.handleImmediateMidiStop();
    MidiClock.handleMidiStop();
    break;
  case MIDI_CONTINUE:
    MidiClock.handleImmediateMidiContinue();
    MidiClock.handleMidiContinue();
    break;
  default:
    break;
  }
}

} // namespace

void PlatformPanel::loop() {}

bool PlatformPanel::handleEvent(gui_event_t *event) {
  if (!EVENT_BUTTON(event)) {
    return false;
  }

  const bool is_press = event->mask == EVENT_BUTTON_PRESSED;
  const bool is_release = event->mask == EVENT_BUTTON_RELEASED;
  if (!is_press && !is_release) {
    return false;
  }

  const uint8_t source = event->source;
  const PageIndex pg = mcl.currentPage();

  if (source <= ButtonsClass::BUTTON4) {
    return false;
  }

  uint8_t key = 255;

  if (source >= ButtonsClass::TRIG_BUTTON1 &&
      source < ButtonsClass::TRIG_BUTTON1 + 16) {
    if (pg == PAGE_SELECT_PAGE) {
      return false;
    }
    key = source - ButtonsClass::TRIG_BUTTON1;
  } else {
    const bool copy_mode =
        key_interface.is_key_down(MDX_KEY_NO) ||
        key_interface.is_key_down(MDX_KEY_FUNC) ||
        (is_seq_page(pg) &&
         (key_interface.is_key_down(MDX_KEY_SCALE) ||
          note_interface.notes_count_on() > 0)) ||
        (pg == PERF_PAGE_0 && note_interface.notes_count_on() > 0);

    switch (source) {
    case ButtonsClass::FUNC_BUTTON1:
      key = copy_mode ? MDX_KEY_COPY : MDX_KEY_REC;
      break;

    case ButtonsClass::FUNC_BUTTON2:
      key = copy_mode ? MDX_KEY_CLEAR : MDX_KEY_PLAY;
      if (is_press && key == MDX_KEY_PLAY) {
        if (BUTTON_DOWN(ButtonsClass::FUNC_BUTTON1)) {
          seq_step_page.enable_record();
          if (MidiClock.state != MidiClockClass::STARTED) {
            handle_local_transport(MIDI_START);
          }
          key = 255;
        } else if (MidiClock.state == MidiClockClass::PAUSED) {
          handle_local_transport(MIDI_CONTINUE);
        } else if (MidiClock.state == MidiClockClass::STARTED) {
          handle_local_transport(MIDI_STOP);
        } else {
          handle_local_transport(MIDI_START);
        }
      }
      break;

    case ButtonsClass::FUNC_BUTTON3:
      key = copy_mode ? MDX_KEY_PASTE : MDX_KEY_STOP;
      if (is_press && key == MDX_KEY_STOP &&
          (MidiClock.state == MidiClockClass::STARTED ||
           MidiClock.state == MidiClockClass::PAUSED)) {
        handle_local_transport(MIDI_STOP);
      }
      break;

    case ButtonsClass::FUNC_BUTTON5:
      key = MDX_KEY_FUNC;
      break;

    case ButtonsClass::FUNC_BUTTON6:
      key = MDX_KEY_UP;
      break;
    case ButtonsClass::FUNC_BUTTON7:
      key = MDX_KEY_LEFT;
      break;
    case ButtonsClass::FUNC_BUTTON8:
      key = MDX_KEY_DOWN;
      break;
    case ButtonsClass::FUNC_BUTTON9:
      key = MDX_KEY_RIGHT;
      break;

    case ButtonsClass::TBD_BUTTON_B:
      if (b_button_is_scale(pg)) {
        key = MDX_KEY_SCALE;
      } else {
        event->source = ButtonsClass::BUTTON3;
        return false;
      }
      break;

    case ButtonsClass::TBD_BUTTON_TR:
      event->source = ButtonsClass::BUTTON4;
      return false;

    default:
      break;
    }
  }

  if (key == 255) {
    return false;
  }

  key_interface.key_event(key, is_release);
  return true;
}

#endif // MCL_HAS_TBD_DRIVER

bool platform_panel_handleEvent(gui_event_t *event) {
  return platform_panel.handleEvent(event);
}

#endif // MCL_HAS_EXTENDED_PANEL_INPUT
