#include "SDDrivePage.h"
#include "MCL.h"

const char *c_snapshot_suffix = ".snp";
const char *c_snapshot_root = "/Snapshots/MD";

void SDDrivePage::setup() {
  SD.mkdir(c_snapshot_root, true);
  SD.chdir(c_snapshot_root);
  strcpy(lwd, c_snapshot_root);
  FileBrowserPage::setup();
}

void SDDrivePage::init() {

  DEBUG_PRINT_FN();
  md_exploit.off();
  //  !note match only supports 3-char suffix
  strcpy(match, c_snapshot_suffix);
  show_dirs = true;
  strcpy(title, "Snapshots");
  FileBrowserPage::init();
}

void SDDrivePage::save_snapshot() {
  DEBUG_PRINT_FN();

  MDSound sound;
  char sound_name[] = "________";

  if (mcl_gui.wait_for_input(sound_name, "Snapshot Name", 8)) {
    if (file.isOpen()) {
      file.close();
    }

    MidiUart.sendRaw(MIDI_STOP);
    MidiClock.handleImmediateMidiStop();
    // Stop everything
    grid_page.prepare();

    char temp_entry[16];
    strcpy(temp_entry, sound_name);
    strcat(temp_entry, c_snapshot_suffix);
    DEBUG_PRINTLN("creating new snapshot:");
    DEBUG_PRINTLN(temp_entry);
    if (!file.open(temp_entry, O_WRITE | O_CREAT)) {
      DEBUG_PRINTLN("error openning");
      gfx.alert("Error", "Cannot open file for write");
      return;
    }
    //  Globals
    for (int i = 0; i < 8; ++i) {
      mcl_gui.draw_progress("Saving global", i, 8);
      MD.getBlockingGlobal(i);
      mcl_sd.write_data(&MD.global, sizeof(MD.global), &file);
    }
    //  Patterns
    for (int i = 0; i < 128; ++i) {
      mcl_gui.draw_progress("Saving pattern", i, 128);
      DEBUG_PRINT(i);
      MD.getBlockingPattern(i);
      mcl_sd.write_data(&MD.pattern, sizeof(MD.pattern), &file);
    }
    //  Kits
    for (int i = 0; i < 64; ++i) {
      mcl_gui.draw_progress("Saving kit", i, 64);
      MD.getBlockingKit(i);
      mcl_sd.write_data(&MD.kit, sizeof(MD.kit), &file);
    }
    //  Songs
    // for(int i=0;i<64;++i){
    // MD.getBlockingSong(i);
    // mcl_sd.write_data(&MD.song, sizeof(MD.song), &file); // <--- ??
    //}
    //  Save complete
    file.close();
    gfx.alert("File Saved", temp_entry);
  }
}

void SDDrivePage::load_snapshot() {

  DEBUG_PRINT_FN();
  if (file.isOpen()) {
    char temp_entry[16];
    file.getName(temp_entry, 16);
    file.close();
    DEBUG_PRINTLN("loading snapshot");
    DEBUG_PRINTLN(temp_entry);
    if (!file.open(temp_entry, O_READ)) {
      DEBUG_PRINTLN("error openning");
      gfx.alert("Error", "Cannot open file for read");
      return;
    }

    MidiUart.sendRaw(MIDI_STOP);
    MidiClock.handleImmediateMidiStop();
    // Stop everything
    grid_page.prepare();

    //  Globals
    for (int i = 0; i < 8; ++i) {
      mcl_gui.draw_progress("Loading global", i, 8);
      mcl_sd.read_data(&MD.global, sizeof(MD.global), &file);
      mcl_actions.md_setsysex_recpos(2, i);
      {
        ElektronDataToSysexEncoder encoder(&MidiUart);
        MD.global.toSysex(encoder);
      }
    }
    //  Patterns
    for (int i = 0; i < 128; ++i) {
      mcl_gui.draw_progress("Loading pattern", i, 128);
      mcl_sd.read_data(&MD.pattern, sizeof(MD.pattern), &file);
      mcl_actions.md_setsysex_recpos(8, i);
      MD.pattern.toSysex();
    }
    //  Kits
    for (int i = 0; i < 64; ++i) {
      mcl_gui.draw_progress("Loading kit", i, 64);
      mcl_sd.read_data(&MD.kit, sizeof(MD.kit), &file);
      mcl_actions.md_setsysex_recpos(4, i);
      MD.kit.toSysex();
    }
    //  Load complete
    file.close();
    gfx.alert("Loaded", "Snapshot");
  }
}

bool SDDrivePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {

    if (encoders[1]->getValue() == 0) {
      save_snapshot();
      init();
      return false;
    }

    if (encoders[1]->getValue() == 1) {
      create_folder();
      return false;
    }

    char temp_entry[16];
    char dir_entry[16];
    uint32_t pos = BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = (uint8_t *)pos;
    memcpy_bank1(&temp_entry[0], ptr, 16);

    if ((temp_entry[0] == '.') && (temp_entry[1] == '.')) {
      file.close();

      SD.vwd()->getName(dir_entry, 16);
      lwd[strlen(lwd) - strlen(dir_entry) - 1] = '\0';
      DEBUG_PRINTLN(lwd);
      SD.chdir(lwd);

      init();
      return false;
    }

    file.open(temp_entry, O_READ);

    if (file.isDirectory()) {
      file.close();
      SD.vwd()->getName(dir_entry, 16);
      strcat(lwd, dir_entry);
      if (dir_entry[strlen(dir_entry) - 1] != '/') {
        strcat(lwd, "/");
      }
      DEBUG_PRINTLN(lwd);
      DEBUG_PRINTLN(temp_entry);
      SD.chdir(temp_entry);
      init();
      return false;
    }

    load_snapshot();

    return true;
  }

  // if ((EVENT_RELEASED(event, Buttons.BUTTON1) &&
  // BUTTON_DOWN(Buttons.BUTTON3)) ||
  if (EVENT_RELEASED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON1)) {
    char temp_entry[16];
    char dir_entry[16];
    uint32_t pos = BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = (uint8_t*)pos;
    memcpy_bank1(&temp_entry[0], ptr, 16);
    SD.remove(temp_entry);
    init();
    return false;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON2) ||
      EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
    //  EVENT_RELEASED(event, Buttons.BUTTON4)) {
    GUI.setPage(&grid_page);
    return true;
  }
  return false;
}

MCLEncoder sddrive_param1(1, 10, ENCODER_RES_SYS);
MCLEncoder sddrive_param2(0, 36, ENCODER_RES_SYS);
SDDrivePage sddrive_page(&sddrive_param1, &sddrive_param2);
