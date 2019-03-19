#include "MCL.h"
#include "SoundBrowserPage.h"
#include "MDSound.h"

void SoundBrowserPage::init() {

  DEBUG_PRINT_FN();
  md_exploit.off();
  char *snd = ".snd";
  strcpy(match, snd);
  dir_browser = true;
  char *files = "Sounds";
  strcpy(title, files);
  FileBrowserPage::init();
}

void SoundBrowserPage::save_sound() {
  DEBUG_PRINT_FN();

  char *snd = ".snd";
  MDSound sound;
  char *sound_name = "________";
  char *my_title = "Sound Name";

  grid_page.prepare();
  PGM_P tmp;
  tmp = getMachineNameShort(MD.kit.models[MD.currentTrack], 2);
  memcpy(sound_name,MD.kit.name,4);
  m_strncpy_p(&sound_name[5], tmp, 3);

  if (mcl_gui.wait_for_input(sound_name, my_title, 8)) {
    char temp_entry[16];
    strcpy(temp_entry, sound_name);
    strcat(temp_entry, snd);
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
  char *snd = ".snd";
  grid_page.prepare();
  if (file.isOpen()) {
    char temp_entry[16];
    MDSound sound;
    file.getName(temp_entry, 16);
    file.close();
    DEBUG_PRINTLN("loading sound");
    DEBUG_PRINTLN(temp_entry);
    if (!sound.file.open(temp_entry, O_READ)) {
    DEBUG_PRINTLN("error openning");
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
    gfx.alert("Loaded","Sound");
    sound.file.close();
  }

}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {

    if (encoders[1]->getValue() == 0) {
      save_sound();
      init();
      return;
    }

    if (encoders[1]->getValue() == 1) {
      create_folder();
      return;
    }

    char temp_entry[16];
    char dir_entry[16];
    uint32_t pos = FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = pos;
    switch_ram_bank(1);
    memcpy(&temp_entry[0], ptr, 16);
    switch_ram_bank(0);
    char *up_one_dir = "..";

    if ((temp_entry[0] == '.') && (temp_entry[1] == '.')) {
      file.close();
      SD.chdir(lwd);

      SD.vwd()->getName(dir_entry, 16);
      lwd[strlen(lwd) - strlen(dir_entry) - 1] = '\0';
      DEBUG_PRINTLN(lwd);

      init();
      return;
    }

    file.open(temp_entry, O_READ);

    if (file.isDirectory()) {
      file.close();
      SD.vwd()->getName(dir_entry, 16);
      strcat(lwd, dir_entry);
      if (dir_entry[strlen(dir_entry) - 1] != '/') {
        char *slash = "/";
        strcat(lwd, slash);
      }
      DEBUG_PRINTLN(lwd);
      DEBUG_PRINTLN(temp_entry);
      SD.chdir(temp_entry);
      init();
      return;
    }

    load_sound();

    return true;
  }
  
 // if ((EVENT_RELEASED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON3)) || 
  if (EVENT_RELEASED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON1)) {
     char temp_entry[16];
    char dir_entry[16];
    uint32_t pos = FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = pos;
    switch_ram_bank(1);
    memcpy(&temp_entry[0], ptr, 16);
    switch_ram_bank(0);
    SD.remove(temp_entry);
    init();
    return;
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

MCLEncoder soundbrowser_param1(1, 10, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
