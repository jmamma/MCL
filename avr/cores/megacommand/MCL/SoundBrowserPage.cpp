#include "MCL_impl.h"

const char *c_sound_root = "/Sounds/MD";
const char *c_snd_suffix = ".snd";
const char *c_wav_suffix = ".wav";

const char *c_snd_name = "SOUND";
const char *c_wav_name = "WAV";

#define FT_SND 0
#define FT_WAV 1

#define PA_NEW 0
#define PA_SELECT 1

void SoundBrowserPage::setup() {
  SD.mkdir(c_sound_root, true);
  SD.chdir(c_sound_root);
  strcpy(lwd, c_sound_root);
  filetypes[0] = c_snd_suffix;
  filetypes[1] = c_wav_suffix;
  filetype_names[0] = c_snd_name;
  filetype_names[1] = c_wav_name;
  filetype_max = FT_WAV;
  FileBrowserPage::setup();
}

void SoundBrowserPage::init() {
  trig_interface.off();

  if (show_samplemgr) {
    strcpy(title, "MD-ROM");
  } else {
    strcpy(match, ".snd");
    strcpy(title, "Sounds");
    show_dirs = true;
    // show_save = true;
    show_save = (filetype_idx == FT_SND);
    show_filemenu = true;
    show_new_folder = true;
    show_overwrite = true;
    show_filetypes = true;
    show_edit_wav = (filetype_idx == FT_WAV);
  }
  FileBrowserPage::init();
}

void SoundBrowserPage::save_sound() {

  MDSound sound;
  char sound_name[] = "        ";

  grid_page.prepare();
  PGM_P tmp;
  tmp = getMDMachineNameShort(MD.kit.get_model(MD.currentTrack), 2);
  memcpy(sound_name, MD.kit.name, 4);
  strncpy_P(&sound_name[5], tmp, 3);

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

  grid_page.prepare();
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

// send current selected wav file to slot
void SoundBrowserPage::send_wav(int slot) {
  if (file.isOpen()) {
    char temp_entry[FILE_ENTRY_SIZE];
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    file.close();
    // TODO loop stuff? do we have info?
    midi_sds.sendWav(temp_entry, slot, /* show progress */ true);
    gfx.alert("Sample sent", temp_entry);
  }
}

void SoundBrowserPage::recv_wav(int slot) {
  char wav_name[] = "        ";


  if (mcl_gui.wait_for_input(wav_name, "Sample Name", 8)) {
    char temp_entry[FILE_ENTRY_SIZE];
    strcpy(temp_entry, wav_name);
    strcat(temp_entry, ".wav");
    // TODO
    midi_sds.sendDumpRequest(slot);
    gfx.alert("Sample received", temp_entry);
  }
}

void SoundBrowserPage::on_new() {
  if (!show_samplemgr) {
    switch (filetype_idx) {
    case FT_SND:
      save_sound();
      break;
    case FT_WAV:
      pending_action = PA_NEW;
      show_samplemgr = true;
      break;
    }
    init();
  } else {
    // shouldn't happen.
    // show_save = false for samplemgr.
    show_samplemgr = false;
    init();
  }
}

void SoundBrowserPage::on_cancel() {
  if (show_samplemgr) {
    show_samplemgr = false;
  } else {
    // TODO cd .. ?
  }
}

void SoundBrowserPage::on_select(const char *__) {
  if (!show_samplemgr) {
    switch (filetype_idx) {
    case FT_SND:
      load_sound();
      break;
    case FT_WAV:
      pending_action = PA_SELECT;
      show_samplemgr = true;
      init();
      break;
    }
  } else {
    auto slot = encoders[1]->cur;
    switch (pending_action) {
    case PA_NEW:
      recv_wav(slot);
      break;
    case PA_SELECT:
      send_wav(slot);
      break;
    }
    show_samplemgr = false;
    init();
  }
}

bool SoundBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  return FileBrowserPage::handleEvent(event);
}

MCLEncoder soundbrowser_param1(0, 1, ENCODER_RES_SYS);
MCLEncoder soundbrowser_param2(0, 36, ENCODER_RES_SYS);
SoundBrowserPage sound_browser(&soundbrowser_param1, &soundbrowser_param2);
