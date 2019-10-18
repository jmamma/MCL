#include "SDDrivePage.h"
#include "MCL.h"

const char *c_snapshot_suffix = ".snp";
const char *c_snapshot_root = "/SDDrive/MD";

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
  strcpy(title, "SD-Drive");
  FileBrowserPage::init();
}

void SDDrivePage::save_snapshot() {
  DEBUG_PRINT_FN();

  char entry_name[] = "        ";

  if (mcl_gui.wait_for_input(entry_name, "Snapshot Name", 8)) {
    if (file.isOpen()) {
      file.close();
    }

    MidiUart.sendRaw(MIDI_STOP);
    MidiClock.handleImmediateMidiStop();
    // Stop everything
    grid_page.prepare();

    char temp_entry[16];
    strcpy(temp_entry, entry_name);
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
      if (!mcl_sd.read_data(&MD.global, sizeof(MD.global), &file)) {
        goto load_error;
      }
      mcl_actions.md_setsysex_recpos(2, i);
      {
        ElektronDataToSysexEncoder encoder(&MidiUart);
        MD.global.toSysex(encoder);
      }
    }
    //  Patterns
    for (int i = 0; i < 128; ++i) {
      mcl_gui.draw_progress("Loading pattern", i, 128);
      if (!mcl_sd.read_data(&MD.pattern, sizeof(MD.pattern), &file)) {
        goto load_error;
      }
      mcl_actions.md_setsysex_recpos(8, i);
      MD.pattern.toSysex();
    }
    //  Kits
    for (int i = 0; i < 64; ++i) {
      mcl_gui.draw_progress("Loading kit", i, 64);
      if (!mcl_sd.read_data(&MD.kit, sizeof(MD.kit), &file)) {
        goto load_error;
      }
      mcl_actions.md_setsysex_recpos(4, i);
      MD.kit.toSysex();
    }
    //  Load complete
    file.close();
    gfx.alert("Loaded", "Snapshot");
    return;
  load_error:
    file.close();
    gfx.alert("Snapshot loading failed!", "SD card read failure");
    return;
  }
}

void SDDrivePage::on_new() {
  save_snapshot();
  init();
}

void SDDrivePage::on_select(const char *__) { load_snapshot(); }

MCLEncoder sddrive_param1(1, 10, ENCODER_RES_SYS);
MCLEncoder sddrive_param2(0, 36, ENCODER_RES_SYS);
SDDrivePage sddrive_page(&sddrive_param1, &sddrive_param2);
