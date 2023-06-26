#include "MCL_impl.h"
#include "ResourceManager.h"

const char *c_wav_root = "/Samples/WAV";
const char *c_syx_root = "/Samples/SYX";
const char *c_wav_suffix = ".wav";
const char *c_syx_suffix = ".syx";

const char *c_wav_name = "WAV";
const char *c_syx_name = "SYSEX";

static bool s_query_returned = false;

void SampleBrowserPage::setup() {
  SD.mkdir(c_wav_root, true);
  SD.mkdir(c_syx_root, true);
  sysex = &(Midi.midiSysex);
  show_samplemgr = false;
  FileBrowserPage::setup();
  _cd(c_wav_root);
  position.reset();
}

void SampleBrowserPage::display() {
  if (filemenu_active) {
    draw_menu();
    return;
  }
  if (FileBrowserPage::selection_change) {
    draw_sidebar();
    if (clock_diff(FileBrowserPage::selection_change_clock, slowclock) < 200) {
      goto end;
    }
    char temp_entry[FILE_ENTRY_SIZE];

    get_entry(encoders[1]->getValue(), temp_entry);
    uint8_t len = strlen(temp_entry);

    if (len > 4) {
      bool is_wav = strcmp(c_wav_suffix, (&temp_entry[len - 4])) == 0;
      Wav wav_file;

      if (!is_wav || (is_wav && !wav_file.open(temp_entry, false))) {
        FileBrowserPage::selection_change = false;
        goto end;
      }

      oled_display.setCursor(0, 23);

      float sample_rate_f = (wav_file.header.fmt.sampleRate * 0.001f);
      uint16_t sample_rate = (uint16_t)sample_rate_f;
      oled_display.print(sample_rate);
      oled_display.print(F("."));

      uint8_t decimal =
          ((sample_rate_f - (float)sample_rate) * (float)10.0f) + 0.5f;
      oled_display.print(decimal);
      oled_display.print(F("k "));

      oled_display.print(wav_file.header.fmt.bitRate);
      oled_display.print(F("/"));
      oled_display.print(wav_file.header.fmt.numChannels);
      float seconds = wav_file.header.get_length() / (float)wav_file.header.fmt.sampleRate;
      int16_t minutes = seconds * 0.01666666667f;
      int16_t ms = ((float)seconds - int(seconds)) * 1000;

      oled_display.setCursor(0, 30);

      oled_display.print(minutes);
      oled_display.print(F(":"));
      oled_display.print(int(seconds));
      oled_display.print(F(":"));
      oled_display.print(ms);

      wav_file.close();
    }
  }
  FileBrowserPage::selection_change = false;
end:
  draw_filebrowser();
  oled_display.display();
}

void SampleBrowserPage::init() {
  FileBrowserPage::selection_change = true;
  file_types.reset();
  file_types.add(c_wav_suffix);
  file_types.add(c_syx_suffix);

  trig_interface.off();
  filemenu_active = false;
  select_dirs = false;
  show_overwrite = false;

  if (show_samplemgr) {
    strcpy(title, "MD-ROM");
    draw_dirs = false;
    show_dirs = false;
    show_save = false;
    show_filemenu = false;
    show_new_folder = false;
    show_parent = false;
    query_sample_slots();
  } else {
    strcpy(title, "SAMPLE");
    draw_dirs = true;
    show_dirs = true;
    show_save = true;
    show_filemenu = true;
    show_new_folder = true;
    show_parent = true;
    query_filesystem();
  }

  R.Clear();
  R.use_machine_names_short();
}

// send current selected sample file to slot
void SampleBrowserPage::send_sample(int slot, char *newname, bool silent) {
  bool success;
  if (file.isOpen()) {
    char temp_entry[FILE_ENTRY_SIZE];
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    file.close();
    bool is_syx =
        strcmp(c_syx_suffix, &temp_entry[strlen(temp_entry) - 4]) == 0;
    if (!silent) {
      if (!mcl_gui.wait_for_confirm("Sample Slot", "Overwrite?")) {
        return;
      }
    }
    if (is_syx) {
      success = midi_sds.sendSyx(temp_entry, slot);
    } else {
      char *ptr = newname;
      if (newname == nullptr) {
        if (isdigit(temp_entry[0]) && isdigit(temp_entry[1]) &&
            (temp_entry[2] != '.') && (temp_entry[2] != '\0')) {
          ptr = temp_entry + 2;
        }
      }
      success =
          midi_sds.sendWav(temp_entry, ptr, slot, /* show progress */ true);
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

void SampleBrowserPage::recv_wav(int slot, bool silent) {
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
  if (SD.exists(temp_entry)) { gfx.alert("File exists!", temp_entry); return; }
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

void SampleBrowserPage::on_new() {
  if (!show_samplemgr) {
    pending_action = PA_NEW;
    show_samplemgr = true;
    show_ram_slots = true;
    init();
  } else {
    // shouldn't happen.
    // show_save = false for samplemgr.
    show_samplemgr = false;
    init();
  }
}

void SampleBrowserPage::on_cancel() {
  pending_action = 0;
  if (show_samplemgr) {
    show_samplemgr = false;
//    init();
  } else {
    // TODO cd .. ?
    _cd_up();
  }
}

void SampleBrowserPage::on_select(const char *__) {
  if (!show_samplemgr) {
    pending_action = PA_SELECT;
    show_samplemgr = true;
    show_ram_slots = false;
    DEBUG_PRINTLN("on select");
    init();
  } else {
    auto slot = encoders[1]->cur;
    switch (pending_action) {
    case PA_NEW:
      recv_wav(slot);
      break;
    case PA_SELECT:
      send_sample(slot);
      break;
    }
    pending_action = 0;
    show_samplemgr = false;
    init();
  }
}

bool SampleBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && show_filemenu) {
    FileBrowserPage::handleEvent(event);
    bool state = (param2->cur == 0);
    file_menu_page.menu.enable_entry(FM_NEW_FOLDER, !state);
    file_menu_page.menu.enable_entry(FM_DELETE, !state); // delete
    file_menu_page.menu.enable_entry(FM_RENAME, !state); // rename
    file_menu_page.menu.enable_entry(FM_OVERWRITE, !state);
    file_menu_page.menu.enable_entry(FM_RECVALL, state);
    file_menu_page.menu.enable_entry(FM_SENDALL, state);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mcl.setPage(PAGE_SELECT_PAGE);
    return true;
  }

  return FileBrowserPage::handleEvent(event);
}

void SampleBrowserPage::query_sample_slots() {
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

bool SampleBrowserPage::_handle_filemenu() {
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
      if (wav_name[5] != '[') {
        recv_wav(n, true);
      }
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
      if (!isdigit(wav_name[0]) || !isdigit(wav_name[1]))
        continue;
      uint8_t slot = (wav_name[0] - '0') * 10 + wav_name[1] - '0' - 1;
      DEBUG_PRINTLN("slot pos:");
      DEBUG_PRINTLN(slot);
      DEBUG_PRINTLN((uint8_t)wav_name[0]);
      DEBUG_PRINTLN((uint8_t)wav_name[1]);
      if (slot > 48) {
        continue;
      }

      file.open(wav_name);
      mcl_gui.draw_progress("Send Samples", n, numEntries);
      send_sample(slot, wav_name + 2, true);
    }
    break;
  }
}
void SampleBrowserPage::start() {}

void SampleBrowserPage::end() {
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

MCLEncoder samplebrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder samplebrowser_param2(0, 36, ENCODER_RES_SYS);
SampleBrowserPage sample_browser(&samplebrowser_param1, &samplebrowser_param2);