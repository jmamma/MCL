#include "SoundBrowserPage.h"
#include "MCL.h"
#include "MDSound.h"

const char* c_sound_root = "/Sounds/MD";
const char* c_snd_suffix = ".snd";
const char* c_syx_suffix = ".syx";
const char* c_wav_suffix = ".wav";

const char* c_snd_name = "SOUND";
const char* c_syx_name = "SDS";
const char* c_wav_name = "WAV";

#define FT_SND 0
#define FT_SYX 1
#define FT_WAV 2

void SoundBrowserPage::setup() {
  SD.mkdir(c_sound_root, true);
  SD.chdir(c_sound_root);
  strcpy(lwd, c_sound_root);
  filetypes[0] = c_snd_suffix;
  filetypes[1] = c_syx_suffix;
  filetypes[2] = c_wav_suffix;
  filetype_names[0] = c_snd_name;
  filetype_names[1] = c_syx_name;
  filetype_names[2] = c_wav_name;
  filetype_max = FT_WAV;
  FileBrowserPage::setup();
}
void SoundBrowserPage::init() {

  DEBUG_PRINT_FN();
  trig_interface.off();
  char *files = "Sounds";
  strcpy(title, files);

  show_dirs = true;
  show_save = true;
  show_filemenu = true;
  show_new_folder = true;
  show_overwrite = true;
  show_filetypes = true;
  FileBrowserPage::init();
}

void SoundBrowserPage::save_sound() {
  DEBUG_PRINT_FN();

  MDSound sound;
  char sound_name[] = "        ";

  grid_page.prepare();
  PGM_P tmp;
  tmp = getMachineNameShort(MD.kit.models[MD.currentTrack], 2);
  memcpy(sound_name, MD.kit.name, 4);
  m_strncpy_p(&sound_name[5], tmp, 3);

  if (mcl_gui.wait_for_input(sound_name, "Sound Name", 8)) {
    char temp_entry[16];
    strcpy(temp_entry, sound_name);
    strcat(temp_entry, ".snd");
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
    gfx.alert("Loaded", "Sound");
    sound.file.close();
  }
}

void SoundBrowserPage::on_new() {
  switch (filetype_idx) {
    case FT_SND:
      save_sound();
      init();
      break;
    case FT_SYX:
      // TODO recv syx
      break;
    case FT_WAV:
      // TODO recv wav
      break;
  }
}

void SoundBrowserPage::on_cancel() {
  GUI.setPage(&grid_page);
}

void SoundBrowserPage::on_select(const char *__) { 
  switch (filetype_idx) {
    case FT_SND:
      load_sound();
      break;
    case FT_SYX:
      // TODO send syx
      break;
    case FT_WAV:
      // TODO send wav
      break;
  }
}

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
