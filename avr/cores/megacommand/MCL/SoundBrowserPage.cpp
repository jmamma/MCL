#include "MCL_impl.h"
#include "ResourceManager.h"

const char *c_sound_root = "/Sounds/MD";
const char *c_snd_suffix = ".snd";
const char *c_wav_suffix = ".wav";

const char *c_snd_name = "SOUND";
const char *c_wav_name = "WAV";

static bool s_query_returned = false;

#define FT_SND 0
#define FT_WAV 1

#define PA_NEW 0
#define PA_SELECT 1

void SoundBrowserPage::cleanup() {
  // always call setup() when entering this page.
  this->isSetup = false;
}

void SoundBrowserPage::setup() {
  SD.mkdir(c_sound_root, true);
  SD.chdir(c_sound_root);
  strcpy(lwd, c_sound_root);
  show_samplemgr = false;
  sysex = &(Midi.midiSysex);
  FileBrowserPage::setup();
}

void SoundBrowserPage::init() {
  trig_interface.off();
  filemenu_active = false;
  select_dirs = false;

  filetypes[0] = c_snd_suffix;
  filetypes[1] = c_wav_suffix;
  filetype_names[0] = c_snd_name;
  filetype_names[1] = c_wav_name;
  filetype_max = FT_WAV;

  if (show_samplemgr) {
    strcpy(title, "MD-ROM");
    show_dirs = false;
    show_save = false;
    show_filemenu = false;
    show_new_folder = false;
    show_overwrite = true;
    show_filetypes = false;
    show_parent = false;
    query_sample_slots();
  } else {
    strcpy(match, ".snd");
    strcpy(title, "Sounds");
    show_dirs = true;
    show_save = true;
    //show_save = (filetype_idx == FT_SND);
    show_filemenu = true;
    show_new_folder = true;
    show_overwrite = true;
    show_filetypes = true;
    show_parent = true;
    query_filesystem();
  }

  R.Clear();
  R.use_machine_names_short();
}

void SoundBrowserPage::save_sound() {

  MDSound sound;
  char sound_name[] = "        ";

  grid_page.prepare();
  PGM_P tmp;
  tmp = getMDMachineNameShort(MD.kit.get_model(MD.currentTrack), 2);
  memcpy(sound_name, MD.kit.name, 4);
  strncpy_P(&sound_name[5], tmp, 3);

  if (mcl_gui.wait_for_input(sound_name, "Sound Name", 8)) {
    char temp_entry[FILE_ENTRY_SIZE];
    strcpy(temp_entry, sound_name);
    strcat(temp_entry, ".snd");
    sound.file.open(temp_entry, O_RDWR | O_CREAT);
    sound.fetch_sound(MD.currentTrack);
    sound.write_sound();
    sound.file.close();
    gfx.alert("File Saved", temp_entry);
  }
}

void SoundBrowserPage::load_sound() {

  grid_page.prepare();
  if (file.isOpen()) {
    char temp_entry[FILE_ENTRY_SIZE];
    MDSound sound;
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    file.close();
    DEBUG_PRINTLN(F("loading sound"));
    DEBUG_PRINTLN(temp_entry);
    if (!sound.file.open(temp_entry, O_READ)) {
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
  if (file.isOpen()) {
    char temp_entry[FILE_ENTRY_SIZE];
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    file.close();
    midi_sds.sendWav(temp_entry, slot, /* show progress */ true);
    gfx.alert("Sample sent", temp_entry);
  }
}

void SoundBrowserPage::recv_wav(int slot) {
  char wav_name[] = "        ";


  if (mcl_gui.wait_for_input(wav_name, "Sample Name", 8)) {
    char temp_entry[FILE_ENTRY_SIZE];
    strcpy(temp_entry, wav_name);
    strcat(temp_entry, ".wav");
    // TODO
    midi_sds.sendDumpRequest(slot);
    gfx.alert("Sample received", temp_entry);
  }
}

void SoundBrowserPage::on_new() {
  if (!show_samplemgr) {
    switch (filetype_idx) {
    case FT_SND:
      save_sound();
      break;
    case FT_WAV:
      pending_action = PA_NEW;
      show_samplemgr = true;
      break;
    }
    init();
  } else {
    // shouldn't happen.
    // show_save = false for samplemgr.
    show_samplemgr = false;
    init();
  }
}

void SoundBrowserPage::on_cancel() {
  if (show_samplemgr) {
    show_samplemgr = false;
  } else {
    // TODO cd .. ?
  }
}

void SoundBrowserPage::on_select(const char *__) {
  if (!show_samplemgr) {
    switch (filetype_idx) {
    case FT_SND:
      load_sound();
      break;
    case FT_WAV:
      pending_action = PA_SELECT;
      show_samplemgr = true;
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
    show_samplemgr = false;
    init();
  }
}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  return FileBrowserPage::handleEvent(event);
}

void SoundBrowserPage::query_sample_slots() {
  encoders[1]->cur = 0;
  encoders[1]->old = 0;
  numEntries = 0;
  cur_file = 255; // XXX why 255?
  cur_row = 0;
  uint8_t data[2] = {0x70, 0x34};
  call_handle_filemenu = false;
  s_query_returned = false;

  sysex->addSysexListener(this);
  MD.sendRequest(data, 2);
  auto time_start = read_slowclock();
  auto time_now = time_start;
  do {
    handleIncomingMidi();
    time_now = read_slowclock();
  } while(!s_query_returned && clock_diff(time_start, time_now) < 1000);

  if (!s_query_returned) {
    add_entry("ERROR");
  }
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;

  sysex->removeSysexListener(this);
}

// MidiSysexListenerClass implementation
void SoundBrowserPage::start() {}

void SoundBrowserPage::end() {}

void SoundBrowserPage::end_immediate() {
  if (sysex->getByte(3) != 0x02)
    return;
  if (sysex->getByte(4) != 0x00)
    return;
  if (sysex->getByte(5) != 0x72)
    return;
  if (sysex->getByte(6) != 0x34)
    return;
  int nr_samplecount = sysex->getByte(7);
  if (nr_samplecount > 48)
    return;

  char s_tmpbuf[5];
  char temp_entry[FILE_ENTRY_SIZE];

  for (int i = 0, j = 7; i < nr_samplecount; ++i) {
    for (int k = 0; k < 5; ++k) {
      s_tmpbuf[k] = sysex->getByte(++j);
    }
    bool slot_occupied = s_tmpbuf[4];
    s_tmpbuf[4] = 0;
    strcpy(temp_entry, "00 - ");
    if (i < 10) {
      itoa(i, temp_entry+1, 10);
    } else {
      itoa(i, temp_entry, 10);
    }
    if (slot_occupied) {
      strcat(temp_entry, s_tmpbuf);
    } else {
      strcat(temp_entry, "[EMPTY]");
    }
    add_entry(temp_entry);
  }

  s_query_returned = true;
}



MCLEncoder soundbrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
