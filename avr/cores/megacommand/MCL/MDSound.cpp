#include "MCL_impl.h"

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
  machine1.model = MD.kit.get_model(track);
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
    machine1.lfo.depth = machine1.params[MODEL_LFOD] = 0;
  }

  machine1.track = 0;
  machine1.normalize_level();
  machine_count++;

  // If track uses trigGroup, assume sound is made up of two models.

  if ((trigGroup < 16) && (trigGroup != track)) {
    machine2.model = MD.kit.get_model(trigGroup);
    machine2.level = MD.kit.levels[trigGroup];
    memcpy(&machine2.params, &(MD.kit.params[trigGroup]), 24);
    memcpy(&machine2.lfo, &(MD.kit.lfos[trigGroup]), sizeof(MDLFO));

    if (machine2.lfo.destinationTrack == trigGroup) {
      machine2.lfo.destinationTrack = 1;
    }

    else if (machine2.lfo.destinationTrack == track) {
      machine2.lfo.destinationTrack = 0;
    } else {
      machine2.lfo.destinationTrack = 255;
      machine2.lfo.depth = machine2.params[MODEL_LFOD] = 0;
    }

    machine_count++;
    machine2.track = 1;
    machine2.normalize_level();
  }
}

bool MDSound::load_sound(uint8_t track) {
  DEBUG_PRINT_FN();

  DEBUG_PRINTLN(machine1.model);
#ifdef DEBUG_MODE
  char str[3] = "  ";
  const char* tmp = getMDMachineNameShort(machine1.model, 2);
  copyMachineNameShort(tmp, str);
  DEBUG_PRINTLN(str);
#endif

  bool send_level = true, send = true;
  if ((machine_count > 1) && (track != 15)) {
#ifdef DEBUG_MODE
    DEBUG_PRINTLN(F("loading second machine"));
    tmp = getMDMachineNameShort(machine2.model, 2);
    copyMachineNameShort(tmp, str);
    DEBUG_PRINTLN(str);
#endif
    if (machine2.lfo.destinationTrack < 16) {
      machine2.lfo.destinationTrack += track;
    }
    machine1.trigGroup = track + 1;
    machine2.trigGroup = track;

    MD.sendMachine(track + 1, &machine2, send_level, send);

  } else {
    machine1.trigGroup = track;
  }

  if (machine1.lfo.destinationTrack < 16) {
    machine1.lfo.destinationTrack += track;
  }

  if (machine_count == 1) {
    machine1.trigGroup = track;
  } else {
    machine1.trigGroup = track + 1;
  }

  MD.sendMachine(track, &machine1, send_level, send);
}

bool MDSound::read_data(void *data, uint32_t size, uint32_t position) {
  DEBUG_PRINT_FN();

  // DEBUG_PRINTLN(size);
  // DEBUG_PRINTLN(position);
  bool ret;

  ret = file.seekSet(position);
  if (!ret) {
    DEBUG_PRINTLN(F("could not seek"));
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }
  if (!file.isOpen()) {
    DEBUG_PRINTLN(F("file not open"));
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
    DEBUG_PRINTLN(F("could not seek"));
    DEBUG_PRINTLN(position);
    DEBUG_PRINTLN(file.fileSize());
    return false;
  }

  ret = mcl_sd.write_data(data, size, &file);
  return ret;
}
