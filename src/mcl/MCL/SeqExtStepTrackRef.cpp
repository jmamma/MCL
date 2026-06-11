/* Justin Mammarella jmamma@gmail.com 2018 */

#include "SeqExtStepTrackRef.h"

#include "DeviceManager.h"
#include "MidiDeviceCapabilities.h"
#include "GUI/Pages/Sequencer/SeqPage.h"
#include "../Drivers/MidiDevice.h"

MidiDevice *SeqExtStepTrackRef::mute_mask_device() {
  return SeqPage::device_for_seq_idx(DeviceIdx::Secondary);
}

bool SeqExtStepTrackRef::supports_trig_port(uint8_t port) {
  return device_manager.port_supports(port,
                                      MidiDeviceCapability::MdTrigInterface);
}
