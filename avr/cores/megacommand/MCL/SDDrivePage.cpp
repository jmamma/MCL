#include "MCL_impl.h"
#include "MidiSysexFile.h"

//  !note match only supports 3-char suffix
const char *c_snapshot_suffix = ".snp";
const char *c_samplepack_suffix = ".spk";
const char *c_sysex_suffix = ".syx";

const char* c_snapshot_filetype_name = "Snapshot";
const char* c_samplepack_filetype_name = "Sample";
const char* c_sysex_filetype_name = "SYSEX";

const char *c_snapshot_root = "/SDDrive/MD";

static int s_samplemgr_slot_count = 48;

#define FT_SNP 0
#define FT_SPK 1
#define FT_SYX 2

void SDDrivePage::setup() {
  SD.mkdir(c_snapshot_root, true);
  SD.chdir(c_snapshot_root);
  strcpy(lwd, c_snapshot_root);
  filetypes[0] = c_snapshot_suffix;
  filetypes[1] = c_samplepack_suffix;
  filetypes[2] = c_sysex_suffix;
  filetype_names[0] = c_snapshot_filetype_name;
  filetype_names[1] = c_samplepack_filetype_name;
  filetype_names[2] = c_sysex_filetype_name;
  filetype_max = FT_SYX;
  FileBrowserPage::setup();
}

void SDDrivePage::init() {

  DEBUG_PRINT_FN();
  trig_interface.off();
  strcpy(title, "SD-Drive");

  show_save = true;
  show_dirs = true;
  show_filemenu = true;
  show_new_folder = true;
  show_overwrite = true;
  show_filetypes = true;
  FileBrowserPage::init();
}

void SDDrivePage::display() {
  FileBrowserPage::display();
  if (progress_max != 0) {
    mcl_gui.draw_progress_bar(progress_i, progress_max, false);
  }
}

#define SNP_NAME_LEN 14

void SDDrivePage::save_snapshot() {
  DEBUG_PRINT_FN();

  char entry_name[SNP_NAME_LEN] = "new_snapshot";
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
    char msg[24] = "Overwrite ";
    strcat(msg, entry_name);
    strcat(msg, "?");
    if (!mcl_gui.wait_for_confirm("File exists", msg)) {
      return;
    }
  }
  #ifndef OLED_DISPLAY
  gfx.display_text("Please Wait", "Saving Snap");
  #endif

  DEBUG_PRINTLN(F("creating new snapshot:"));
  DEBUG_PRINTLN(temp_entry);
  if (!file.open(temp_entry, O_WRITE | O_CREAT)) {
    DEBUG_PRINTLN(F("error openning"));
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
    if (!mcl_sd.write_data_v<MDGlobal>(&MD.global, &file)) {
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
    if (!mcl_sd.write_data_v<MDPattern>(&MD.pattern, &file)) {
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
    if (!mcl_sd.write_data_v<MDKit>(&MD.kit, &file)) {
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
  DEBUG_PRINTLN(F("loading snapshot"));
  DEBUG_PRINTLN(temp_entry);
  if (!file.open(temp_entry, O_READ)) {
    DEBUG_PRINTLN(F("error openning"));
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
    if (!mcl_sd.read_data_v<MDGlobal>(&MD.global, &file)) {
      goto load_error;
    }
    MD.setSysexRecPos(2, i);
    {
      ElektronDataToSysexEncoder encoder(&MidiUart);
      delay(20);
      MD.global.toSysex(&encoder);
      #ifndef OLED_DISPLAY
      delay(20);
      #endif
    }
  }
  //  Patterns
  progress_max = 128;
  for (int i = 0; i < 128; ++i) {
    progress_i = i;
    mcl_gui.draw_progress("Loading pattern", i, 128);
    if (!mcl_sd.read_data_v_noinit<MDPattern>(&MD.pattern, &file)) {
      goto load_error;
    }
    MD.setSysexRecPos(8, i);
    delay(20);
    MD.pattern.toSysex();
      #ifndef OLED_DISPLAY
      delay(20);
      #endif
  }
  //  Kits
  progress_max = 64;
  for (int i = 0; i < 64; ++i) {
    progress_i = i;
    mcl_gui.draw_progress("Loading kit", i, 64);
    if (!mcl_sd.read_data_v<MDKit>(&MD.kit, &file)) {
      goto load_error;
    }
    MD.setSysexRecPos(4, i);
    delay(20);
    MD.kit.toSysex();
      #ifndef OLED_DISPLAY
      delay(20);
      #endif

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

void SDDrivePage::send_sysex()
{
  if (!file.isOpen()) {
    return;
  }

  if (!MD.connected) {
    gfx.alert("Error", "MD is not connected");
    return;
  }

  char temp_entry[16];
  file.getName(temp_entry, 16);
  file.close();

  if (!mcl_gui.wait_for_confirm("Send SYSEX", temp_entry)) {
    return;
  }

#ifndef OLED_DISPLAY
  gfx.display_text("Please Wait", "Sending SYSEX");
#else
  mcl_gui.draw_popup("Sending SYSEX");
#endif
  // TODO Midi2?
  MidiSysexFile msf(&Midi);
  msf.send(temp_entry);
}

void SDDrivePage::recv_sysex()
{
  // TODO receive sysex dump
}

void SDDrivePage::send_sample_pack(int start_slot) {
  DEBUG_PRINT_FN();

  if (!file.isOpen()) {
    return;
  }

  if (!MD.connected) {
    gfx.alert("Error", "MD is not connected");
    return;
  }

  if (!mcl_gui.wait_for_confirm("Load sample pack", "Overwrite MD data?")) {
    return;
  }

  char temp_entry[16];
  file.getName(temp_entry, 16);
  file.close();

  if (!file.open(temp_entry, O_READ)) {
    DEBUG_PRINTLN("error openning");
    gfx.alert("Error", "Cannot open file for read");
    return;
  }

#ifndef OLED_DISPLAY
  gfx.display_text("Please Wait", "Loading samples");
#endif

  int slot = start_slot;
  int len = 0;
  while(slot < s_samplemgr_slot_count) {
    len = file.readBytesUntil('\n', temp_entry, sizeof(temp_entry) - 1);
    if (len <= 0) break;
    if (temp_entry[len-1] == '\r') {
      --len;
    }
    temp_entry[len] = 0;
#ifdef OLED_DISPLAY
    char line2[16] = "Sending #";
    itoa(slot, line2 + 9, 10);
    mcl_gui.draw_infobox("Loading samples", line2);
    oled_display.display();
#endif
    midi_sds.sendWav(temp_entry, slot, false);
    ++slot;
  }
}

void SDDrivePage::recv_sample_pack() {
  // TODO save samplepack
}

void SDDrivePage::on_new() {
  switch (filetype_idx) {
    case FT_SNP:
      save_snapshot();
      break;
    case FT_SPK:
      recv_sample_pack();
      break;
    case FT_SYX:
      recv_sysex();
      break;
  }
  init();
}

void SDDrivePage::on_select(const char *__) { 
  if (show_samplemgr) {
    // must be pending spk
    auto slot = encoders[1]->cur;
    s_samplemgr_slot_count = min(48, ((MCLEncoder *)encoders[1])->max + 1);
    send_sample_pack(slot);
    show_samplemgr = false;
    init();
  } else {
    switch (filetype_idx) {
      case FT_SNP: 
        load_snapshot(); 
        break;
      case FT_SPK:
        show_samplemgr = true;
        init();
        break;
      case FT_SYX:
        send_sysex();
        break;
    }
  }
}

MCLEncoder sddrive_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder sddrive_param2(0, 36, ENCODER_RES_SYS);
SDDrivePage sddrive_page(&sddrive_param1, &sddrive_param2);
