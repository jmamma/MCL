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

bool MDSound::fetch_sound(uint8_t track) {
  DEBUG_PRINT_FN();
  machine_count = 0;
  machine1.model = MD.kit.models[track];
  machine1.level = MD.kit.levels[track];
  memcpy(&machine1.params, &(MD.kit.params[track]), 24);
  memcpy(&machine1.lfo, &(MD.kit.lfos[track]), sizeof(MDLFO));

  uint8_t trigGroup = MD.kit.trigGroups[track];
  if (machine1.lfo.destinationTrack == track) {
    machine1.lfo.destinationTrack = 0;
  } else if (machine1.lfo.destinationTrack == trigGroup) {
    machine1.lfo.destinationTrack = 1;
  } else {
    machine1.lfo.destinationTrack = 255;
    machine1.lfo.depth = 0;
  }
  machine_count++;

  // If track uses trigGroup, assume sound is made up of two models.

  if ((trigGroup < 16) && (trigGroup != track)) {
  machine2.model = MD.kit.models[trigGroup];
  machine2.level = MD.kit.levels[trigGroup];
  memcpy(&machine2.params, &(MD.kit.params[trigGroup]), 24);
  memcpy(&machine2.lfo, &(MD.kit.lfos[trigGroup]), sizeof(MDLFO));


    if (machine2.lfo.destinationTrack == track) {
      machine2.lfo.destinationTrack = 1;
    }

    else if (machine2.lfo.destinationTrack == track - 1) {
      machine2.lfo.destinationTrack = 0;
    }
    else {
     machine2.lfo.destinationTrack = 255;
     machine2.lfo.depth = 0;
    }

    machine_count++;
  }
}

bool MDSound::load_sound(uint8_t track) {
  DEBUG_PRINT_FN();

  DEBUG_PRINTLN(machine1.model);

  PGM_P tmp;
  char str[3] = "  ";
  tmp = getMachineNameShort(machine1.model, 2);
  m_strncpy_p(str, tmp, 3);
  DEBUG_PRINTLN(str);

  if ((machine_count > 1) && (track != 15)) {
    DEBUG_PRINTLN("loading second machine");
    tmp = getMachineNameShort(machine2.model, 2);
     m_strncpy_p(str, tmp, 3);
    DEBUG_PRINTLN(str);

    if (machine2.lfo.destinationTrack < 16) {
      machine2.lfo.destinationTrack += track;
    }
    machine1.trigGroup = track + 1;
    machine2.trigGroup = track;

    mcl_actions.md_set_machine(track + 1, &machine2, &MD.kit, true);
    MD.kit.models[track + 1] = machine2.model;
    memcpy(&(MD.kit.params[track + 1]), &machine2.params, 24);
    memcpy(&(MD.kit.lfos[track + 1]), &machine2.lfo, sizeof(MDLFO));

  } else {
    machine1.trigGroup = track;
  }

  if (machine1.lfo.destinationTrack < 16) {
    machine1.lfo.destinationTrack += track;
  }


  mcl_actions.md_set_machine(track, &machine1, &MD.kit ,true);

  if (machine_count == 1) {
  MD.kit.trigGroups[track] = track;
  }
  else {
  MD.kit.trigGroups[track] = track + 1;
  }

  MD.kit.models[track] = machine1.model;
  memcpy(&(MD.kit.params[track]), &machine1.params, 24);
  memcpy(&(MD.kit.lfos[track]), &machine1.lfo, sizeof(MDLFO));


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
