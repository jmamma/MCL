#include "MCL_impl.h"
#include "ResourceManager.h"

const char *c_sound_root = "/Sounds/MD";
const char *c_wav_root = "/Samples/WAV";
const char *c_syx_root = "/Samples/SYX";
const char *c_snd_suffix = ".snd";
const char *c_wav_suffix = ".wav";
const char *c_syx_suffix = ".syx";

const char *c_snd_name = "SOUND";
const char *c_wav_name = "WAV";
const char *c_syx_name = "SYSEX";

static bool s_query_returned = false;

void SoundBrowserPage::cleanup() {
  // always call setup() when entering this page.
  this->isSetup = false;
}

void SoundBrowserPage::setup() {
  SD.mkdir(c_sound_root, true);
  SD.mkdir(c_wav_root, true);
  SD.mkdir(c_syx_root, true);
  SD.chdir();
  show_samplemgr = false;
  sysex = &(Midi.midiSysex);
  chdir_type();
  FileBrowserPage::setup();
}

void SoundBrowserPage::chdir_type() {
  if (filetype_idx == FT_WAV) {
   SD.chdir(c_wav_root);
   strcpy(lwd, c_wav_root);
  }
  else if (filetype_idx == FT_SND) {
   SD.chdir(c_sound_root);
   strcpy(lwd, c_sound_root);
  }
  else {
   SD.chdir(c_syx_root);
   strcpy(lwd, c_syx_root);
  }
}

void SoundBrowserPage::init() {
  trig_interface.off();
  filemenu_active = false;
  select_dirs = false;

  filetypes[0] = c_snd_suffix;
  filetypes[1] = c_wav_suffix;
  filetypes[2] = c_syx_suffix;
  filetype_names[0] = c_snd_name;
  filetype_names[1] = c_wav_name;
  filetype_names[2] = c_syx_name;
  filetype_max = FT_SYX;

  show_overwrite = false;
  if (show_samplemgr) {
    strcpy(title, "MD-ROM");
    show_dirs = false;
    show_save = false;
    show_filemenu = false;
    show_new_folder = false;
    show_filetypes = false;
    show_parent = false;
    query_sample_slots();
  } else {
    strcpy(match, ".snd");
    strcpy(title, "Select:");
    show_dirs = true;
    show_save = (filetype_idx != FT_SYX);
    show_filemenu = true;
    show_new_folder = true;
    show_filetypes = true;
    show_parent = true;
    query_filesystem();
  }

  R.Clear();
  R.use_machine_names_short();
}

void SoundBrowserPage::save_sound() {

  MDSound sound;
  char sound_name[9] = "        ";

  grid_page.prepare();
  memcpy(sound_name, MD.kit.name, 4);
  const char *tmp = getMDMachineNameShort(MD.kit.get_model(MD.currentTrack), 2);
  copyMachineNameShort(tmp, sound_name + 4);
  sound_name[6] = '\0';

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

// send current selected sample file to slot
void SoundBrowserPage::send_sample(int slot, bool is_syx, char *newname, bool silent) {
  bool success;
  if (file.isOpen()) {
    char temp_entry[FILE_ENTRY_SIZE];
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    file.close();
    if (!silent) {
    if (!mcl_gui.wait_for_confirm("Sample Slot", "Overwrite?")) {
      return;
    }
    }
    if (is_syx) {
      success = midi_sds.sendSyx(temp_entry, slot);
    } else {
      success = midi_sds.sendWav(temp_entry, newname, slot, /* show progress */ true);
    }
    if (!silent) {
    if (success) {
      gfx.alert("Sample sent", temp_entry);
    } else {
      gfx.alert("Send failed", temp_entry);
    }
    }
  }
}

void SoundBrowserPage::recv_wav(int slot, bool silent) {
  char wav_name[FILE_ENTRY_SIZE] = "";
  // should be of form "ID - NAME..."
  //                      ^--~~~~~~~
  //                         memmove
  get_entry(slot, wav_name);
  memmove(wav_name + 2, wav_name + 5, FILE_ENTRY_SIZE - 5);
  wav_name[FILE_ENTRY_SIZE - 3] = '\0';
  wav_name[FILE_ENTRY_SIZE - 2] = '\0';
  wav_name[FILE_ENTRY_SIZE - 1] = '\0';

  if (!silent) {
    if (!mcl_gui.wait_for_input(wav_name, "Sample Name",
                                sizeof(wav_name) - 1)) {
      return;
    }
  }
  char temp_entry[FILE_ENTRY_SIZE];
  strncpy(temp_entry, wav_name, sizeof(wav_name) - 1);
  strcat(temp_entry, ".wav");
  DEBUG_PRINTLN("bulk recv");
  DEBUG_PRINTLN(temp_entry);
  bool ret = midi_sds.recvWav(temp_entry, slot);
  if (!silent) {
    if (ret) {
      gfx.alert("Sample received", temp_entry);
    } else {
      gfx.alert("Receive failed", temp_entry);
    }
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
      show_ram_slots = true;
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
  pending_action = 0;
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
    case FT_SYX:
      pending_action = PA_SELECT;
      show_samplemgr = true;
      show_ram_slots = false;
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
      send_sample(slot, (filetype_idx == FT_SYX));
      break;
    }
    pending_action = 0;
    show_samplemgr = false;
    init();
  }
}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && show_filemenu) {
    FileBrowserPage::handleEvent(event);
    bool state = (param2->cur == 0) && filetype_idx == FILETYPE_WAV;
    file_menu_page.menu.enable_entry(FM_NEW_FOLDER, !state);
    file_menu_page.menu.enable_entry(FM_DELETE, !state); // delete
    file_menu_page.menu.enable_entry(FM_RENAME, !state); // rename
    file_menu_page.menu.enable_entry(FM_OVERWRITE, !state);
    file_menu_page.menu.enable_entry(FM_RECVALL, state);
    file_menu_page.menu.enable_entry(FM_SENDALL, state);
    return true;
  }

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
  } while (!s_query_returned && clock_diff(time_start, time_now) < 1000);

  if (!s_query_returned) {
    add_entry("ERROR");
  } else if (numEntries == 0) {
    add_entry("NON-UW MODEL");
  } else if (show_ram_slots) {
    bool mk1 = numEntries < 48;
    char ram[] = "R1";
    add_entry(ram);
    ram[1]++;
    add_entry(ram);
    if (!mk1) {
      ram[1]++;
      add_entry(ram);
      ram[1]++;
      add_entry(ram);
    }
  }
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;
  sysex->removeSysexListener(this);
}

// MidiSysexListenerClass implementation

bool SoundBrowserPage::_handle_filemenu() {
  if (FileBrowserPage::_handle_filemenu()) {
    return true;
  }
  switch (file_menu_page.menu.get_item_index(file_menu_encoder.cur)) {
  case FM_RECVALL:
    show_samplemgr = true;
    show_ram_slots = true;
    init();
    if (numEntries == 0) {
      gfx.alert("NON", "UW");
      goto end;
    }
    if (!mcl_gui.wait_for_confirm("Receive all", "Overwrite?")) {
      goto end;
    }
    DEBUG_PRINTLN("Recv samples");
    DEBUG_PRINTLN(numEntries);
    for (uint8_t n = 0; n < numEntries; n++) {
      DEBUG_PRINTLN("Recv wav");
      char wav_name[FILE_ENTRY_SIZE] = "";
      get_entry(n, wav_name);
      DEBUG_PRINTLN(wav_name);
      if (wav_name[5] != '[') { recv_wav(n, true); }
    }
  end:
    show_samplemgr = false;
    show_ram_slots = false;
    init();
    return true;
  case FM_SENDALL:
    if (!mcl_gui.wait_for_confirm("Send all", "Overwrite?")) {
      return;
    }
    char wav_name[FILE_ENTRY_SIZE] = "";
    for (uint8_t n = 0; n < numEntries; n++) {
      get_entry(n, wav_name);
      DEBUG_PRINTLN(wav_name);
      if (!isdigit(wav_name[0]) || !isdigit(wav_name[1])) continue;
      uint8_t slot = (wav_name[0] - '0') * 10 + wav_name[1] - '0' - 1;
      DEBUG_PRINTLN("slot pos:");
      DEBUG_PRINTLN(slot);
      DEBUG_PRINTLN((uint8_t) wav_name[0]);
      DEBUG_PRINTLN((uint8_t) wav_name[1]);
      if (slot > 48) { continue; }

      file.open(wav_name);
      mcl_gui.draw_progress("Send Samples", n, numEntries);
      send_sample(slot, false, wav_name + 2, true);
    }
    break;
  }
}
void SoundBrowserPage::start() {}

void SoundBrowserPage::end() {
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
    if (i < 9) {
      mcl_gui.put_value_at(i + 1, temp_entry + 1);
    } else {
      mcl_gui.put_value_at(i + 1, temp_entry);
    }
    // put_value_at null-terminates the string. undo that.
    temp_entry[2] = ' ';
    if (slot_occupied) {
      strcat(temp_entry, s_tmpbuf);
    } else {
      strcat(temp_entry, "[EMPTY]");
    }
    add_entry(temp_entry);
  }

  s_query_returned = true;
}

void SoundBrowserPage::end_immediate() {}

MCLEncoder soundbrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
