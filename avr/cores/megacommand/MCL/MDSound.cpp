#include "MCL.h"

bool MDSound::write_sound() {
  DEBUG_PRINT_FN();
  bool ret;
  ret = write_data(this, sizeof(MDSoundData), 0);
  return ret;
}

bool MDSound::read_sound() {
  DEBUG_PRINT_FN();
  bool ret;
  ret = read_data(this, sizeof(MDSoundData), 0);
  return ret;
}

bool MDSound::name_sound() { 
  char menu_name[16] = "Sound Name";
  return mcl_gui.wait_for_input(name, menu_name, 8); 
}

bool MDSound::fetch_sound(uint8_t track) {
  DEBUG_PRINT_FN();
  memcpy(&machine1, &(MD.kit.models[track]), sizeof(MDMachine));
  memcpy(&lfo1, &(MD.kit.lfos[track]), sizeof(MDLFO));

  if (lfo1.destinationTrack == track) {
    lfo1.destinationTrack = 0;
  } else if (lfo1.destinationTrack == track + 1) {
    lfo1.destinationTrack = 1;
  } else {
    lfo1.destinationTrack = 255;
  }
  uint8_t trigGroup = MD.kit.trigGroups[track];
  machine_count++;

  // If track uses trigGroup, assume sound is made up of two models.

  if ((trigGroup < 16) && (trigGroup != track)) {
    memcpy(&machine2, MD.kit.models[track], sizeof(MDMachine));
    memcpy(&lfo2, &(MD.kit.lfos[track]), sizeof(MDLFO));

    if (lfo2.destinationTrack == track) {
      lfo2.destinationTrack = 1;
    }

    if (lfo2.destinationTrack == track - 1) {
      lfo2.destinationTrack = 0;
    }

    machine_count++;
  }
}

bool MDSound::load_sound(uint8_t track) {
  DEBUG_PRINT_FN();
  memcpy(&(MD.kit.models[track]), &machine1, sizeof(MDMachine));
  memcpy(&(MD.kit.lfos[track]), &lfo1, sizeof(MDLFO));
  if (lfo1.destinationTrack < 16) {
    MD.kit.lfos[track].destinationTrack += track;
  }


  if ((machine_count > 1) && (track != 15)) {
    memcpy(&(MD.kit.models[track + 1]), &machine2, sizeof(MDMachine));
    memcpy(&(MD.kit.lfos[track + 1]), &lfo2, sizeof(MDLFO));
    MD.kit.trigGroups[track] = track + 1;
    MD.kit.trigGroups[track + 1] = track;

    if (lfo2.destinationTrack < 16) {
      MD.kit.lfos[track + 1].destinationTrack += track;
    }

    mcl_actions.md_set_machine(track + 1, NULL, &MD.kit);
    }
  else {
    MD.kit.trigGroups[track] = track;
   }

  mcl_actions.md_set_machine(track, NULL, &MD.kit);
}

bool MDSound::read_data(void *data, uint32_t size, uint32_t position) {
   DEBUG_PRINT_FN();

  // DEBUG_PRINTLN(size);
  // DEBUG_PRINTLN(position);
  bool ret;

  ret = file.seekSet(position);
  if (!ret) {
    DEBUG_PRINTLN("could not seek");
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }
  if (!file.isOpen()) {
    DEBUG_PRINTLN("file not open");
    return false;
  }
  ret = mcl_sd.read_data(data, size, &file);
  return ret;
}

bool MDSound::write_data(void *data, uint32_t size, uint32_t position) {
  DEBUG_PRINT_FN();
  //  DEBUG_PRINTLN(size);
  //  DEBUG_PRINTLN(position);
  //  DEBUG_PRINTLN(file.fileSize());
  bool ret = false;

  ret = file.seekSet(position);

  if (!ret) {
    Serial.println(SD.card()->errorCode(), HEX);
    DEBUG_PRINTLN("could not seek");
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }

  ret = mcl_sd.write_data(data, size, &file);
  return ret;
}
