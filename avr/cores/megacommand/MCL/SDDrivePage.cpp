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
  strcpy(title, "SD-Drive");

  show_save = true;
  show_dirs = true;
  show_filemenu = true;
  show_new_folder = true;
  show_overwrite = true;
  FileBrowserPage::init();
}

void SDDrivePage::display() {
  FileBrowserPage::display();
  if (progress_max != 0) {
    mcl_gui.draw_progress_bar(progress_i, progress_max, false);
  }
}

void SDDrivePage::save_snapshot() {
  DEBUG_PRINT_FN();

  char entry_name[] = "        ";
  bool error_is_md = false;

  if (!MD.connected) {
    gfx.alert("Error", "MD is not connected");
    return;
  }

  if (!mcl_gui.wait_for_input(entry_name, "Snapshot Name", 8)) {
    return;
  }

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

  // File exists?
  if (file.open(temp_entry, O_READ)) {
    file.close();
    char msg[24] = {'\0'};
    strcpy(msg, "Overwrite ");
    strcat(msg, entry_name);
    strcat(msg, "?");

    if (!mcl_gui.wait_for_confirm("File exists", msg)) {
      return;
    }
  }
  #ifndef OLED_DISPLAY
  gfx.display_text("Please Wait", "Saving Snap");
  #endif

  DEBUG_PRINTLN("creating new snapshot:");
  DEBUG_PRINTLN(temp_entry);
  if (!file.open(temp_entry, O_WRITE | O_CREAT)) {
    DEBUG_PRINTLN("error openning");
    error_is_md = false;
    goto save_error;
  }
  //  Globals
  progress_max = 8;
  for (int i = 0; i < 8; ++i) {
    progress_i = i;
    mcl_gui.draw_progress("Saving global", i, 8);
    if (!MD.getBlockingGlobal(i)) {
      error_is_md = true;
      goto save_error;
    }
    if (!mcl_sd.write_data(&MD.global, sizeof(MD.global), &file)) {
      error_is_md = false;
      goto save_error;
    }
  }
  //  Patterns
  progress_max = 128;
  for (int i = 0; i < 128; ++i) {
    progress_i = i;
    mcl_gui.draw_progress("Saving pattern", i, 128);
    DEBUG_PRINT(i);
    if (!MD.getBlockingPattern(i)) {
      error_is_md = true;
      goto save_error;
    }
    if (!mcl_sd.write_data(&MD.pattern, sizeof(MD.pattern), &file)) {
      error_is_md = false;
      goto save_error;
    }
  }
  //  Kits
  progress_max = 64;
  for (int i = 0; i < 64; ++i) {
    progress_i = 64;
    mcl_gui.draw_progress("Saving kit", i, 64);
    if (!MD.getBlockingKit(i)) {
      error_is_md = true;
      goto save_error;
    }
    if (!mcl_sd.write_data(&MD.kit, sizeof(MD.kit), &file)) {
      error_is_md = false;
      goto save_error;
    }
  }
  //  Songs
  // for(int i=0;i<64;++i){
  // MD.getBlockingSong(i);
  // mcl_sd.write_data(&MD.song, sizeof(MD.song), &file); // <--- ??
  //}
  //  Save complete
  progress_max = 0;
  file.close();
  gfx.alert("File Saved", temp_entry);
  return;

save_error:
  progress_max = 0;
  file.close();
  if (error_is_md) {
    gfx.alert("File not saved", "MD read error!");
  } else {
    gfx.alert("File not saved", "SD card write error!");
  }
}

void SDDrivePage::load_snapshot() {

  DEBUG_PRINT_FN();

  if (!file.isOpen()) {
    return;
  }

  if (!MD.connected) {
    gfx.alert("Error", "MD is not connected");
    return;
  }

  if (!mcl_gui.wait_for_confirm("Load snapshot", "Overwrite MD data?")) {
    return;
  }

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
  #ifndef OLED_DISPLAY
  gfx.display_text("Please Wait", "Restoring Snap");
  #endif

  MidiUart.sendRaw(MIDI_STOP);
  MidiClock.handleImmediateMidiStop();
  // Stop everything
  grid_page.prepare();

  //  Globals
  progress_max = 8;
  for (int i = 0; i < 8; ++i) {
    progress_i = i;
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
  progress_max = 128;
  for (int i = 0; i < 128; ++i) {
    progress_i = i;
    mcl_gui.draw_progress("Loading pattern", i, 128);
    if (!mcl_sd.read_data(&MD.pattern, sizeof(MD.pattern), &file)) {
      goto load_error;
    }
    mcl_actions.md_setsysex_recpos(8, i);
    MD.pattern.toSysex();
  }
  //  Kits
  progress_max = 64;
  for (int i = 0; i < 64; ++i) {
    progress_i = i;
    mcl_gui.draw_progress("Loading kit", i, 64);
    if (!mcl_sd.read_data(&MD.kit, sizeof(MD.kit), &file)) {
      goto load_error;
    }
    mcl_actions.md_setsysex_recpos(4, i);
    MD.kit.toSysex();
  }
  //  Load complete
  progress_max = 0;
  file.close();
  gfx.alert("Loaded", "Snapshot");
  return;
load_error:
  progress_max = 0;
  file.close();
  gfx.alert("Snapshot loading failed!", "SD card read failure");
  return;
}

void SDDrivePage::on_new() {
  save_snapshot();
  init();
}

void SDDrivePage::on_select(const char *__) { load_snapshot(); }

MCLEncoder sddrive_param1(1, 10, ENCODER_RES_SYS);
MCLEncoder sddrive_param2(0, 36, ENCODER_RES_SYS);
SDDrivePage sddrive_page(&sddrive_param1, &sddrive_param2);
