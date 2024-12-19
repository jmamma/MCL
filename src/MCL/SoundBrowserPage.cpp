#include "SoundBrowserPage.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "MD.h"
#include "MDSound.h"

const char *c_snd_root = "/Sounds";
const char *c_snd_suffix = ".snd";

static bool s_query_returned = false;

void SoundBrowserPage::setup() {
  SD.mkdir(c_snd_root, true);
  FileBrowserPage::setup();
  _cd(c_snd_root);
  position.reset();
}

void SoundBrowserPage::init() {
  FileBrowserPage::selection_change = true;
  file_types.reset();
  file_types.add(c_snd_suffix);

  trig_interface.off();

  strcpy(title, "SOUND");
  show_dirs = true;
  show_save = true;
  show_filemenu = true;
  show_new_folder = true;
  show_parent = true;
  SD.chdir(lwd);
  query_filesystem();

  strcpy(str_save, "[ SAVE ]");

  R.Clear();
  R.use_machine_names_short();
}

void SoundBrowserPage::save_sound() {

  MDSound sound;
  char sound_name[8];
  uint8_t l = min(strlen(MD.kit.name),4);
  memcpy(sound_name, MD.kit.name, l);
  const char *tmp = getMDMachineNameShort(MD.kit.get_model(MD.currentTrack), 2);
  if (tmp) {
    copyMachineNameShort(tmp, sound_name + l + 1);
  }
  sound_name[l+3] = '\0';
  sound_name[l] = '_';

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



void SoundBrowserPage::on_new() {
 save_sound();
 init();
}

void SoundBrowserPage::on_cancel() {
  //if (strcmp(lwd, "/") == 0) { mcl.popPage(); return; }
  //_cd_up();
   mcl.popPage();
}

void SoundBrowserPage::on_select(const char *__) {
  load_sound();
}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON3) && show_filemenu) {
    FileBrowserPage::handleEvent(event);
    bool state = (param2->cur == 0);
    file_menu_page.menu.enable_entry(FM_NEW_FOLDER, !state);
    file_menu_page.menu.enable_entry(FM_DELETE, !state); // delete
    file_menu_page.menu.enable_entry(FM_RENAME, !state); // rename
    file_menu_page.menu.enable_entry(FM_RECVALL, state);
    file_menu_page.menu.enable_entry(FM_SENDALL, state);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mcl.popPage();
    return true;
  }

  return FileBrowserPage::handleEvent(event);
}


MCLEncoder soundbrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
