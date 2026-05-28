/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "DeviceManager.h"
#include "MCLSysConfig.h"
#include "NoteInterface.h"
#include "../Midi/Midi.h"
#include "../Drivers/MidiDevice.h"
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "global.h"

namespace {

MidiClass *note_interface_secondary_midi() {
#ifdef PLATFORM_TBD
  PortSlot slots[SLOT_COUNT];
  resolve_slots(slots);
  const PortSlot &slot =
      mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT ? slots[SLOT_ELEKT]
                                                   : slots[SLOT_GENER];
  if (slot.midi != nullptr) {
    return slot.midi;
  }
#endif
  MidiDevice *secondary = device_manager.secondary_device();
  return (secondary != nullptr && secondary->midi != nullptr)
             ? secondary->midi
             : generic_midi_device.midi;
}

void setup_note_interface_midi(NoteInterfaceMidiEvents *events,
                               MidiClass *midi) {
  if (midi == nullptr) return;
  midi->addOnNoteOnCallback(
      events,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  midi->addOnNoteOffCallback(
      events,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2);
}

void cleanup_note_interface_midi(NoteInterfaceMidiEvents *events,
                                 MidiClass *midi) {
  if (midi == nullptr) return;
#if !defined(__AVR__)
  midi->removeOnNoteOnCallback(
      events,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi);
  midi->removeOnNoteOffCallback(
      events,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi);
#endif
  midi->removeOnNoteOnCallback(
      events,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  midi->removeOnNoteOffCallback(
      events,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2);
}

void cleanup_note_interface_all_midi(NoteInterfaceMidiEvents *events) {
  cleanup_note_interface_midi(events, &Midi);
  cleanup_note_interface_midi(events, &Midi2);
  cleanup_note_interface_midi(events, &MidiUSB);
#ifdef PLATFORM_TBD
  cleanup_note_interface_midi(events, &MidiP4);
#endif
}

} // namespace

void NoteInterface::setup() { ni_midi_events.setup_callbacks(); }

void NoteInterface::init_notes() {
   notes_on = 0;
   notes_off = 0;
   //notes_ignore = 0;
   memset(note_hold, 0, sizeof(note_hold));
}

bool NoteInterface::is_note(uint8_t note_num) {
   uint32_t mask = notes_on | notes_off;
   return IS_BIT_SET32(mask, note_num);
}

void NoteInterface::clear_note(uint8_t note_num) {
   if (note_num < NI_MAX_NOTES) {
     CLEAR_BIT32(notes_on, note_num);
     CLEAR_BIT32(notes_off, note_num);
   }
}

void NoteInterface::add_note_event(uint8_t note_num, uint8_t event_mask, uint8_t port) {
  gui_event_t event;
  event.source = note_num;
  event.mask = event_mask;
  event.port = port;
  event.type = NOTE;
  event.modifiers = 0;
  GUI.putEvent(&event);
}

void NoteInterface::note_on_event(uint8_t note_num, uint8_t port) {
  if (!state) {
    return;
  }
  if (note_num > NI_MAX_NOTES) {
    return;
  }
  if (IS_BIT_SET32(notes_ignore, note_num)) {
    CLEAR_BIT32(notes_ignore, note_num);
    return;
  }
  SET_BIT32(notes_on, note_num);
  CLEAR_BIT32(notes_off, note_num);

  if (note_num < GRID_WIDTH) {
    note_hold[port] = read_clock_ms();
  }
  add_note_event(note_num, EVENT_BUTTON_PRESSED, port);
}
void NoteInterface::note_off_event(uint8_t note_num, uint8_t port) {
  if (!state) {
    return;
  }
  if (IS_BIT_SET32(notes_ignore, note_num)) {
    CLEAR_BIT32(notes_ignore, note_num);
    return;
  }
  CLEAR_BIT32(notes_on, note_num);
  SET_BIT32(notes_off, note_num);
  add_note_event(note_num, EVENT_BUTTON_RELEASED, port);
}

uint8_t NoteInterface::note_to_track_map(uint8_t note, uint8_t device) {
  uint8_t note_to_track_map[7] = {0, 2, 4, 5, 7, 9, 11};
  for (uint8_t i = 0; i < 7; i++) {
    if (note_to_track_map[i] == (note % 12)) {
      if (device == DEVICE_A4) {
        return i + NUM_MD_TRACKS;
      }

      return i;
    }
  }
  return 255;
}

uint8_t NoteInterface::get_first_md_note() {
  uint32_t on = notes_on;

  uint8_t n = 0;
  while (on) {
    if (on & 1) {
      return n;
    }
    n++;
    on >>= 1;
  }
  return 255;
}

uint8_t NoteInterface::notes_count_on() {
  return popcount32(notes_on);
}
uint8_t NoteInterface::notes_count_off() {
  return popcount32(notes_off);
}
uint8_t NoteInterface::notes_count() {
  return popcount32(notes_off | notes_on);
}

void NoteInterface::draw_notes(uint8_t line_number) {
}

#if !defined(__AVR__)
void NoteInterfaceMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  MidiDevice *primary = device_manager.primary_device();
  if (primary->supports_capability(MidiDeviceCapability::MdTrigInterface)) {
    return;
  }
  uint8_t note_num = note_interface.note_to_track_map(msg[1], primary->id);
  note_interface.note_on_event(note_num, primary->port);
}
#endif

void NoteInterfaceMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {

  if (device_manager.secondary_device()->id !=
      note_interface.uart2_device) {
    return;
  }
  uint8_t note_num = note_interface.note_to_track_map(
      msg[1], device_manager.secondary_device()->id);
  DEBUG_PRINTLN(note_num);
  note_interface.note_on_event(note_num, device_manager.secondary_device()->port);
}

#if !defined(__AVR__)
void NoteInterfaceMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {
  // MD-style trig input is handled by the KeyInterface object.
  MidiDevice *primary = device_manager.primary_device();
  if (primary->supports_capability(MidiDeviceCapability::MdTrigInterface)) {
    return;
  }
  uint8_t note_num = note_interface.note_to_track_map(msg[1], primary->id);
  note_interface.note_off_event(note_num, primary->port);
}
#endif

void NoteInterfaceMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {

  if (device_manager.secondary_device()->id !=
      note_interface.uart2_device) {
    return;
  }

  uint8_t note_num = note_interface.note_to_track_map(
      msg[1], device_manager.secondary_device()->id);
  DEBUG_PRINTLN(F("note to track"));
  DEBUG_PRINTLN(note_num);
  note_interface.note_off_event(note_num, device_manager.secondary_device()->port);
}

void NoteInterfaceMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  setup_note_interface_midi(this, note_interface_secondary_midi());
  state = true;
}

void NoteInterfaceMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  cleanup_note_interface_all_midi(this);
  state = false;
}

NoteInterface note_interface;
