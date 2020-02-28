#include "MCL_impl.h"
#include "MidiSDS.hh"

const char *c_sound_root = "/Sounds/MD";
const char *c_snd_suffix = ".snd";
const char *c_wav_suffix = ".wav";

const char *c_snd_name = "SOUND";
const char *c_wav_name = "WAV";

#define FT_SND 0
#define FT_WAV 1

#define PA_NEW 0
#define PA_SELECT 1

static char s_samplename[5];
static volatile bool s_query_returned = false;

void SoundBrowserPage::setup() {
  SD.mkdir(c_sound_root, true);
  SD.chdir(c_sound_root);
  strcpy(lwd, c_sound_root);
  filetypes[0] = c_snd_suffix;
  filetypes[1] = c_wav_suffix;
  filetype_names[0] = c_snd_name;
  filetype_names[1] = c_wav_name;
  filetype_max = FT_WAV;
  sysex = &(Midi.midiSysex);
  FileBrowserPage::setup();
}

void SoundBrowserPage::init() {
  DEBUG_PRINT_FN();
  trig_interface.off();

  if (samplemgr_show) {
    show_dirs = false;
    show_save = false;
    show_filemenu = false;
    show_new_folder = false;
    show_overwrite = false;
    show_filetypes = false;
    strcpy(title, "MD-ROM");
    query_sample_slots();
  } else {
    strcpy(match, ".snd");
    strcpy(title, "Sounds");
    show_dirs = true;
    show_save = true;
    show_filemenu = true;
    show_new_folder = true;
    show_overwrite = true;
    show_filetypes = true;
    FileBrowserPage::init();
  }
}

void SoundBrowserPage::query_sample_slots() {
  DEBUG_PRINT_FN();
  encoders[1]->cur = 0;
  encoders[1]->old = 0;
  numEntries = 0;
  cur_file = 255; // XXX why 255?
  uint8_t data[3] = {0x70, 0x34, 0x00};
  call_handle_filemenu = false;
  sysex->addSysexListener(this);
  for (int i = 0; i < 48; ++i) {
    mcl_gui.draw_progress("Loading slots", i, 47);
    s_query_returned = false;
    data[2] = i;
    MD.sendRequest(data, 3);
    uint16_t time_start = read_slowclock();
    while (!s_query_returned) {
      handleIncomingMidi();
      uint16_t time_now = read_slowclock();
      if (clock_diff(time_start, time_now) > 200) {
        DEBUG_PRINT(i);
        DEBUG_PRINTLN(": timeout");
        break;
      }
    }
    if (s_query_returned) {
      DEBUG_PRINTLN(s_samplename);
      char temp_entry[16];
      sprintf(temp_entry, "%02d - %s", i, s_samplename);
      add_entry(temp_entry);
    } else {
      add_entry("ERROR");
    }
  }
  sysex->removeSysexListener(this);
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;
}

void SoundBrowserPage::save_sound() {
  DEBUG_PRINT_FN();

  MDSound sound;
  char sound_name[] = "        ";

  grid_page.prepare();
  PGM_P tmp;
  tmp = getMDMachineNameShort(MD.kit.models[MD.currentTrack], 2);
  memcpy(sound_name, MD.kit.name, 4);
  m_strncpy_p(&sound_name[5], tmp, 3);

  if (mcl_gui.wait_for_input(sound_name, "Sound Name", 8)) {
    char temp_entry[16];
    sprintf(temp_entry, "%s.snd", sound_name);
    DEBUG_PRINTLN("creating new sound:");
    DEBUG_PRINTLN(temp_entry);
    sound.file.open(temp_entry, O_RDWR | O_CREAT);
    sound.fetch_sound(MD.currentTrack);
    sound.write_sound();
    sound.file.close();
    gfx.alert("File Saved", temp_entry);
  }
}

void SoundBrowserPage::load_sound() {

  DEBUG_PRINT_FN();
  grid_page.prepare();
  if (file.isOpen()) {
    char temp_entry[16];
    MDSound sound;
    file.getName(temp_entry, 16);
    file.close();
    DEBUG_PRINTLN(F("loading sound"));
    DEBUG_PRINTLN(temp_entry);
    if (!sound.file.open(temp_entry, O_READ)) {
      DEBUG_PRINTLN(F("error openning"));
      gfx.alert("Error", "Opening");
      return;
    }
    sound.read_sound();
    if (sound.id != SOUND_ID) {
      sound.file.close();
      gfx.alert("Error", "Not compatible");
      return;
    }
    sound.load_sound(MD.currentTrack);
    gfx.alert("Loaded", "Sound");
    sound.file.close();
  }
}

// send current selected wav file to slot
void SoundBrowserPage::send_wav(int slot) {
  DEBUG_PRINT_FN();
  if (file.isOpen()) {
    char temp_entry[16];
    file.getName(temp_entry, 16);
    file.close();
    DEBUG_PRINTLN("sending sample");
    DEBUG_PRINTLN(temp_entry);
    // TODO loop stuff? do we have info?
    midi_sds.sendWav(temp_entry, slot);
    gfx.alert("Sample sent", temp_entry);
  }
}

void SoundBrowserPage::recv_wav(int slot) {
  DEBUG_PRINT_FN();

  char wav_name[] = "        ";

  if (mcl_gui.wait_for_input(wav_name, "Sample Name", 8)) {
    char temp_entry[16];
    strcpy(temp_entry, wav_name);
    strcat(temp_entry, ".wav");
    DEBUG_PRINTLN("Receiving new sample:");
    DEBUG_PRINTLN(temp_entry);
    // TODO
    midi_sds.sendDumpRequest(slot);
    gfx.alert("Sample received", temp_entry);
  }
}

void SoundBrowserPage::on_new() {
  if (!samplemgr_show) {
    switch (filetype_idx) {
    case FT_SND:
      save_sound();
      break;
    case FT_WAV:
      pending_action = PA_NEW;
      samplemgr_show = true;
      break;
    }
    init();
  } else {
    // shouldn't happen.
    // show_save = false for samplemgr.
    samplemgr_show = false;
    init();
  }
}

void SoundBrowserPage::on_cancel() {
  if (samplemgr_show) {
    // on cancel, break out of sample manager
    samplemgr_show = false;
  }
}

void SoundBrowserPage::on_select(const char *__) {
  if (!samplemgr_show) {
    switch (filetype_idx) {
    case FT_SND:
      load_sound();
      break;
    case FT_WAV:
      pending_action = PA_SELECT;
      samplemgr_show = true;
      init();
      break;
    }
  } else {
    auto slot = encoders[1]->cur;
    switch (pending_action) {
    case PA_NEW:
      recv_wav(slot);
      break;
    case PA_SELECT:
      send_wav(slot);
      break;
    }
    samplemgr_show = false;
    init();
  }
}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    if (samplemgr_show) {
      samplemgr_show = false;
      init();
    }
    // intercept cancel event
    return true;
  }

  return FileBrowserPage::handleEvent(event);
}

// MidiSysexListenerClass implementation
void SoundBrowserPage::start() {}

void SoundBrowserPage::end() {}

void SoundBrowserPage::end_immediate() {
  DEBUG_PRINT_FN("begin pattern matching");
  if (sysex->getByte(3) != 0x02)
    return;
  if (sysex->getByte(4) != 0x00)
    return;
  if (sysex->getByte(5) != 0x72)
    return;
  if (sysex->getByte(6) != 0x34)
    return;
  DEBUG_PRINT_FN("end pattern matching");
  for (int i = 0; i < 4; ++i) {
    s_samplename[i] = sysex->getByte(7 + i);
    DEBUG_PRINT((int)s_samplename[i]);
    DEBUG_PRINT(" ");
  }
  DEBUG_PRINTLN();
  s_samplename[4] = 0;
  s_query_returned = true;
}

MCLEncoder soundbrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
