#include "MCL_impl.h"
#include "ResourceManager.h"

const char* c_sound_root = "/Sounds/MD";

void SoundBrowserPage::setup() {
  SD.mkdir(c_sound_root, true);
  SD.chdir(c_sound_root);
  strcpy(lwd, c_sound_root);
  FileBrowserPage::setup();
}
void SoundBrowserPage::init() {

  DEBUG_PRINT_FN();
  trig_interface.off();
  strcpy(match, ".snd");
  strcpy(title, "Sounds");

  show_dirs = true;
  show_save = true;
  show_filemenu = true;
  show_new_folder = true;
  show_overwrite = true;
  FileBrowserPage::init();

  R.Clear();
  R.use_machine_names_short();
}

void SoundBrowserPage::save_sound() {
  DEBUG_PRINT_FN();

  MDSound sound;
  char sound_name[] = "        ";

  grid_page.prepare();
  PGM_P tmp;
  tmp = getMDMachineNameShort(MD.kit.get_model(MD.currentTrack), 2);
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

void SoundBrowserPage::on_new() {
  save_sound();
  init();
}

void SoundBrowserPage::on_cancel() {
  GUI.setPage(&grid_page);
}

void SoundBrowserPage::on_select(const char *__) { load_sound(); }

bool SoundBrowserPage::handleEvent(gui_event_t* event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    return true;
  }


  return FileBrowserPage::handleEvent(event);
}

MCLEncoder soundbrowser_param1(1, 10, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
