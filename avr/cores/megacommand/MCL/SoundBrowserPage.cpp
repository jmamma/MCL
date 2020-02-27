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

static char s_samplename[6];
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
  cur_file = 0;
  uint8_t data[3] = {0x70, 0x34, 0x00};
  sysex->addSysexListener(this);
  for (int i = 0; i < 48; ++i) {
    mcl_gui.draw_progress("Loading slots", i, 47);
    s_query_returned = false;
    MD.sendRequest(data, 3);
    while (!s_query_returned) {
      handleIncomingMidi();
    }
    add_entry(s_samplename);
  }
  sysex->removeSysexListener(this);
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
    strcpy(temp_entry, sound_name);
    strcat(temp_entry, ".snd");
    DEBUG_PRINTLN(F("creating new sound:"));
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
    // TODO
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
    samplemgr_show = false;
    auto slot = encoders[1]->cur;
    switch (pending_action) {
    case PA_NEW:
      recv_wav(slot);
      break;
    case PA_SELECT:
      send_wav(slot);
      break;
    }
  }
}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1) && !samplemgr_show) {
    // intercept cancel event for non-samplemgr
    return true;
  }

  return FileBrowserPage::handleEvent(event);
}

// MidiSysexListenerClass implementation
void SoundBrowserPage::start() {}

void SoundBrowserPage::end() {}

void SoundBrowserPage::end_immediate() {
  DEBUG_PRINT_FN();

  if (sysex->getByte(0) != 0x00)
    return;
  if (sysex->getByte(1) != 0x20)
    return;
  if (sysex->getByte(2) != 0x3C)
    return;
  if (sysex->getByte(3) != 0x02)
    return;
  if (sysex->getByte(4) != 0x00)
    return;
  if (sysex->getByte(5) != 0x72)
    return;
  if (sysex->getByte(6) != 0x34)
    return;
  DEBUG_PRINTLN("SoundBrowserPage: end_immediate passed");
  for (int i = 0; i < 5; ++i) {
    s_samplename[i] = sysex->getByte(7 + i);
  }
  s_samplename[5] = 0;
}

MCLEncoder soundbrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
