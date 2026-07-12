#include "MCLSysConfig.h"
#include "MCLGUI.h"
#include "hardware.h"
#include "Devices/MidiSetup.h"
#include "Grid/GridChain.h"
#include "MCLSd.h"
#include "Project.h"
#include "hardware.h"
#include "platform.h"
#include "MCLStrings.h"

// Consolidated display function to reduce code duplication
static void show_message(PGM_P line1) {
  oled_display.clearDisplay();
  oled_display.textbox_P(line1);
  oled_display.display();
}

// Common wait routine
static inline void usb_wait() {
  show_message(mclstr_please_wait);
  delay(4000);
}

// Simplified megacmd check
static inline bool megacmd_check() {
  if (!IS_MEGACMD()) {
    show_message(mclstr_mode_na);
    return false;
  }
  return true;
}

// Combined USB mode change function
static void enter_usb_mode(uint8_t mode, PGM_P line1) {
  usb_wait();
  show_message(line1);

  if (mode == USB_STORAGE) {
    LOCAL_SPI_DISABLE();
    EXTERNAL_SPI_ENABLE();
  }

  change_usb_mode(mode);
  while (1); // Infinite loop
}

// Optimized public functions
void usb_os_update() {
  enter_usb_mode(USB_SERIAL, mclstr_os_update);
}

void usb_dfu_mode() {
  enter_usb_mode(USB_DFU, mclstr_dfu_mode);
}

void usb_disk_mode() {
  if (megacmd_check()) {
    enter_usb_mode(USB_STORAGE, mclstr_usb_disk);
  }
}


void mclsys_apply_config() {
  DEBUG_PRINT_FN();
  GUI.display_mirror = mcl_cfg.display_mirror;
  if (mcl_cfg.write_cfg()) {
    proj.store_config_from_system();
  }
}

void mclsys_normalize_midi_config() {
  if (mcl_cfg.clock_rec >= MIDI_CLOCK_SOURCE_COUNT) {
    mcl_cfg.clock_rec = MIDI_CLOCK_SOURCE_PORT1;
  }
  if (mcl_cfg.midi_transport_rec >= MIDI_CLOCK_SOURCE_COUNT) {
    mcl_cfg.midi_transport_rec = MIDI_CLOCK_SOURCE_PORT1;
  }

#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_x_device != GRID_X_DEVICE_MD &&
      mcl_cfg.grid_x_device != GRID_X_DEVICE_TBD) {
    mcl_cfg.grid_x_device = GRID_X_DEVICE_OFF;
  }
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    mcl_cfg.grid_x_port = GRID_X_PORT_INT;
  } else if (mcl_cfg.grid_x_port > GRID_X_PORT_USB ||
             mcl_cfg.grid_x_port == GRID_X_PORT_INT) {
    mcl_cfg.grid_x_port = GRID_X_PORT_1;
  }
  if (mcl_cfg.grid_y_device > GRID_Y_DEVICE_OFF) {
    mcl_cfg.grid_y_device = GRID_Y_DEVICE_TBD;
  }
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    mcl_cfg.grid_y_port = GRID_Y_PORT_INT;
  } else if (mcl_cfg.grid_y_port > GRID_Y_PORT_USB ||
             mcl_cfg.grid_y_port == GRID_Y_PORT_INT) {
    mcl_cfg.grid_y_port = GRID_Y_PORT_2;
  }
#else
  if (mcl_cfg.grid_x_device != GRID_X_DEVICE_MD) {
    mcl_cfg.grid_x_device = GRID_X_DEVICE_OFF;
  }
  if (mcl_cfg.grid_x_port != GRID_X_PORT_USB) {
    mcl_cfg.grid_x_port = GRID_X_PORT_1;
  }

  if (mcl_cfg.grid_y_device < GRID_Y_DEVICE_GENER ||
      mcl_cfg.grid_y_device > GRID_Y_DEVICE_OFF) {
    mcl_cfg.grid_y_device = GRID_Y_DEVICE_GENER;
  }
  if (mcl_cfg.grid_y_port != GRID_Y_PORT_USB) {
    mcl_cfg.grid_y_port = GRID_Y_PORT_2;
  }
#endif

  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD &&
      mcl_cfg.grid_x_port == GRID_X_PORT_USB &&
      mcl_cfg.grid_y_device != GRID_Y_DEVICE_OFF &&
      mcl_cfg.grid_y_port == GRID_Y_PORT_USB) {
    mcl_cfg.grid_y_port = GRID_Y_PORT_2;
  }

  mcl_cfg.uart1_device = 2;
  mcl_cfg.uart2_device = 2;
  mcl_cfg.usb_device = 0;

  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_MD) {
    if (mcl_cfg.grid_x_port == GRID_X_PORT_USB) {
      mcl_cfg.usb_device = 1;
    } else {
      mcl_cfg.uart1_device = 1;
    }
  }

#ifdef PLATFORM_TBD
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_GENER) {
    if (mcl_cfg.grid_y_port == GRID_Y_PORT_USB) {
      mcl_cfg.usb_device = 3;
    } else {
      mcl_cfg.uart2_device = 0;
    }
  } else if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_ELEKT) {
    if (mcl_cfg.grid_y_port == GRID_Y_PORT_USB) {
      mcl_cfg.usb_device = 2;
    } else {
      mcl_cfg.uart2_device = 1;
    }
  }
#else
  if (mcl_cfg.grid_y_device != GRID_Y_DEVICE_OFF) {
    if (mcl_cfg.grid_y_port == GRID_Y_PORT_USB) {
      mcl_cfg.usb_device = 4 - mcl_cfg.grid_y_device;
    } else {
      mcl_cfg.uart2_device = mcl_cfg.grid_y_device - 1;
    }
  }
#endif
}

void mclsys_apply_config_midi() {
  mclsys_normalize_midi_config();

  mclsys_apply_config();
  midi_setup.cfg_ports();
}

bool MCLSysConfig::write_cfg() {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Writing cfg"));
  cfgfile.close();
  char path[64];
  ret = cfgfile.open(mcl_sd.full_path("/config.mcls", path, sizeof(path)), O_RDWR);
  if (!ret) {
    DEBUG_PRINTLN(F("Open cfg file failed"));
    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(MCLSysConfigData), &cfgfile);
  if (!ret) {
    DEBUG_PRINTLN(F("Write cfg failed"));
  }
  DEBUG_PRINTLN(F("Write cfg okay"));
  cfgfile.close();
  cfg_save_lastclock = read_clock_ms();
  return true;
}

bool MCLSysConfig::cfg_init() {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Initialising cfgfile"));
  cfgfile.close();
  char path[64];
  const char *cfg_path = mcl_sd.full_path("/config.mcls", path, sizeof(path));
  if (!SD.remove(cfg_path)) {
    DEBUG_PRINTLN(F("Failed to remove old config file"));
  }
  // First open the file
  ret = cfgfile.open(cfg_path, O_RDWR | O_CREAT);
  if (!ret) {
    DEBUG_PRINTLN(F("Failed to open cfgfile"));
    return false;
  }

  // Then preallocate space
  ret = cfgfile.preAllocate(GRID_SLOT_BYTES);
  if (ret) {
    DEBUG_PRINTLN(F("Created new cfgfile"));
  } else {
    DEBUG_PRINTLN(F("Failed to create new cfgfile"));
    cfgfile.close();  // Clean up if allocation fails
    return false;
  }
  memset((uint8_t *)&version, 0, sizeof(MCLSysConfigData)); //<---- flush zero to config
  ptc_groups.clear();

  version = CONFIG_VERSION;
  //number_projects = 0;
  project[0] = '\0';
  //clock_send = 0;
  //clock_rec = 0;
  uart1_turbo_speed = DEFAULT_TURBO_SPEED;
  //uart2_turbo_speed = 0;
  usb_turbo_speed = DEFAULT_TURBO_SPEED;
  //col = 0;
  //row = 0;
  //cur_row = 0;
  //cur_col = 0;
  memset(&routing, 6, sizeof(routing));
  //poly_mask = 0;
  uart2_ctrl_chan = MIDI_LOCAL_MODE;
  uart2_poly_chan = MIDI_LOCAL_MODE;
  uart2_prg_in = MIDI_LOCAL_MODE;
  uart2_prg_out = MIDI_LOCAL_MODE;
  //mutes = 0;
  //display_mirror = 0;
  //rec_quant = 0;
  tempo = 125;

  //midi_forward_1 = 0;
  //midi_forward_2 = 0;
  //midi_forward_usb = 0;

  rec_automation = 1;
  auto_normalize = 1;
  load_mode = LOAD_MANUAL;
  chain_queue_length = 1;
  chain_load_quant = 16;
  //ram_page_mode = 0;
  track_select = 1;
  track_type_select = 0b00000011;
  uart1_device = 1;
  //uart2_device = 0;
  //uart_cc_fwd = 0;
  //uart2_prg_mode = 0;
  usb_mode = USB_SERIAL;
  //midi_transport_rec = 0;
  //midi_transport_send = 0;
  midi_ctrl_port = 1;
  //md_trig_channel = 0;
  //seq_dev = 0;
  uart2_cc_mute = 128;
  uart2_cc_level = 128;
  //grid_page_mode = 0;
  uart_note_fwd = 1;
  //usb_device = 0;
  grid_x_device = GRID_X_DEVICE_MD;
  grid_x_port = GRID_X_PORT_1;
#ifdef PLATFORM_TBD
  grid_y_device = GRID_Y_DEVICE_TBD;
  grid_y_port = GRID_Y_PORT_INT;
#else
  grid_y_device = GRID_Y_DEVICE_GENER;
  grid_y_port = GRID_Y_PORT_2;
#endif
  project_config = 0;
  md_sample_bank = 0;
  md_sample_bank_capture = 0;
  active_arrangement_idx = 0;
  mclsys_normalize_midi_config();
  cfgfile.close();
  ret = write_cfg();
  if (!ret) {
    return false;
  }
  return true;
}

MCLSysConfig mcl_cfg;
