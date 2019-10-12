#include "MCL.h"
#include "SDDrivePage.h"

const char* c_snapshot_suffix = ".snap";

void SDDrivePage::setup() {
  SD.mkdir("/Snapshots/MD", true);
  FileBrowserPage::setup();
}

void SDDrivePage::init() {

  DEBUG_PRINT_FN();
  md_exploit.off();
  strcpy(match, c_snapshot_suffix);
  dir_browser = true;
  strcpy(title, "Snapshots");
  FileBrowserPage::init();

}

void SDDrivePage::save_snapshot() {
  DEBUG_PRINT_FN();

  MDSound sound;
  char sound_name[] = "________";
  char my_title[] = "Snapshot Name";

  if (mcl_gui.wait_for_input(sound_name, my_title, 8)) {
    char temp_entry[16];
    strcpy(temp_entry, sound_name);
    strcat(temp_entry, c_snapshot_suffix);
    DEBUG_PRINTLN("creating new snapshot:");
    DEBUG_PRINTLN(temp_entry);
    //sound.file.open(temp_entry, O_RDWR | O_CREAT);
    //sound.fetch_sound(MD.currentTrack);
    //sound.write_sound();
    //sound.file.close();
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
    //if (!sound.file.open(temp_entry, O_READ)) {
      //DEBUG_PRINTLN("error openning");
      //gfx.alert("Error", "Opening");
      //return;
    //}
    //sound.file.close();
    gfx.alert("Loaded","Snapshot");
  }

}


bool SDDrivePage::handleEvent(gui_event_t *event) 
{
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
    volatile uint8_t *ptr = (uint8_t*)pos;
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
  
 // if ((EVENT_RELEASED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON3)) || 
  if (EVENT_RELEASED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON1)) {
     char temp_entry[16];
    char dir_entry[16];
    uint32_t pos = BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = pos;
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
