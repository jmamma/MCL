#include "MCL.h"

uint8_t last_ext_track;

MCLEncoder seq_param1(0, 3, ENCODER_RES_SEQ);
MCLEncoder seq_param2(0, 64, ENCODER_RES_SEQ);
MCLEncoder seq_param3(0, 10, ENCODER_RES_SEQ);
MCLEncoder seq_param4(0, 64, ENCODER_RES_SEQ);

MCLEncoder seq_lock1(0, 127, ENCODER_RES_PARAM);
MCLEncoder seq_lock2(0, 127, ENCODER_RES_PARAM);

MCLEncoder ptc_param_oct(0, 8, ENCODER_RES_SEQ);
MCLEncoder ptc_param_finetune(0, 64, ENCODER_RES_SEQ);
MCLEncoder ptc_param_len(0, 64, ENCODER_RES_SEQ);
MCLEncoder ptc_param_scale(0, 15, ENCODER_RES_SEQ);

SeqParamPage seq_param_page[NUM_PARAM_PAGES];
SeqStepPage seq_step_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);
SeqRtrkPage seq_rtrk_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);
SeqRlckPage seq_rlck_page(&seq_param1, &seq_param2, &seq_param3, &seq_param4);
SeqExtStepPage seq_extstep_page(&seq_param1, &seq_param2, &seq_param3,
                                &seq_param4);
SeqPtcPage seq_ptc_page(&ptc_param_oct, &ptc_param_finetune, &ptc_param_len, &ptc_param_scale);

void mcl_save_sound() {
  DEBUG_PRINT_FN();
  file_browser.dir_browser = true;
  GUI.pushPage(&file_browser);
  while (GUI.currentPage() == &file_browser) {
     GUI.loop();
  }
  MDSound sound;
  char *sound_name = "new_sound";
  char *my_title = "Sound Name";
  if (mcl_gui.wait_for_input(sound_name, my_title, 8)) {
    char snd = ".snd";
    char temp_entry[16];
    strcat(temp_entry,sound_name);
    strcat(temp_entry,snd);
    sound.file.open(temp_entry, O_RDWR | O_CREAT);
    sound.fetch_sound(MD.currentTrack);
    sound.write_sound();
    sound.file.close();
  }
}

void mcl_load_sound() {
  DEBUG_PRINT_FN();
  file_browser.dir_browser = false;
  GUI.pushPage(&file_browser);
  //If file_browser file is still open, then it means we selected
  //a file for pening.
  if (file_browser.file.isOpen()) {
   char temp_entry[16];
   MDSound sound;
   file_browser.file.getName(temp_entry, 16);
   file_browser.file.close();
   sound.file.open(temp_entry,O_RDWR);
   sound.load_sound(MD.currentTrack);
   sound.file.close();
  }
}

MCLEncoder track_menu_param1(0, 8, ENCODER_RES_PAT);
MCLEncoder track_menu_param2(0, 8, ENCODER_RES_PAT);
MenuPage track_menu_page(&track_menu_layout, &track_menu_param1, &track_menu_param2);


//SeqLFOPage seq_lfo_page[NUM_LFO_PAGES];
