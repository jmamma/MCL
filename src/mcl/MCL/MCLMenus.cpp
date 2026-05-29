#include "MCLMenus.h"
#include "Project.h"
#include "ResourceManager.h"
#include "SeqPages.h"
#include "MCLSysConfig.h"
#include "MCLGUI.h"
#include "MCL.h"
#include "GridPages.h"
#include "CommonPages.h"
#include "MCLActions.h"
#include "DeviceManager.h"
#include "WavDesignerPage.h"
#include "../Drivers/MidiDevice.h"

static MCLEncoder menu_value_encoder(0, 17, ENCODER_RES_SYS);
static MCLEncoder menu_entry_encoder(0, 17, ENCODER_RES_SYS);

uint8_t opt_import_src = 0;
uint8_t opt_import_dest = 0;
uint8_t opt_import_count = 16;

namespace {

constexpr uint8_t SYSTEM_MENU_DRIVER_PRIMARY = 2;
constexpr uint8_t SYSTEM_MENU_DRIVER_SECONDARY = 3;

bool driver_config_entry(DeviceIdx device_idx, DriverConfigMenuEntry *entry) {
  MidiDevice *device = device_manager.device_for_idx(device_idx);
  return device != nullptr && device != &null_midi_device &&
         device->config_menu_entry(device_idx, entry);
}

const char *driver_entry_name(MidiDevice *device) {
  if (device != nullptr && device != &null_midi_device) {
    return device->full_name;
  }
  return nullptr;
}

void open_driver_config(DeviceIdx device_idx) {
  DriverConfigMenuEntry entry = {nullptr, NULL_PAGE};
  if (driver_config_entry(device_idx, &entry) && entry.page != NULL_PAGE) {
    mcl.pushPage(entry.page);
  }
}

void open_primary_driver_config() {
  open_driver_config(DeviceIdx::Primary);
}

void open_secondary_driver_config() {
  open_driver_config(DeviceIdx::Secondary);
}

void new_project_from_menu() {
  proj.new_project_prompt();
}

} // namespace

void SystemMenuPage::prepare_menu_entries() {
  MidiDevice *primary_device = device_manager.primary_device();
  MidiDevice *secondary_device = device_manager.secondary_device();

  const char *primary_name = driver_entry_name(primary_device);
  if (primary_name != nullptr) {
    menu.set_entry_name(SYSTEM_MENU_DRIVER_PRIMARY, primary_name);
  }
  if (secondary_device != primary_device) {
    const char *secondary_name = driver_entry_name(secondary_device);
    if (secondary_name != nullptr) {
      menu.set_entry_name(SYSTEM_MENU_DRIVER_SECONDARY, secondary_name);
    }
  }

  DriverConfigMenuEntry entry = {nullptr, NULL_PAGE};
  bool primary =
      driver_config_entry(DeviceIdx::Primary, &entry) && entry.name != nullptr;
  menu.enable_entry(SYSTEM_MENU_DRIVER_PRIMARY, primary);
  if (primary) {
    menu.set_entry_name(SYSTEM_MENU_DRIVER_PRIMARY, entry.name);
  }

  entry = {nullptr, NULL_PAGE};
  bool secondary = secondary_device != primary_device &&
                   driver_config_entry(DeviceIdx::Secondary, &entry) &&
                   entry.name != nullptr;
  menu.enable_entry(SYSTEM_MENU_DRIVER_SECONDARY, secondary);
  if (secondary) {
    menu.set_entry_name(SYSTEM_MENU_DRIVER_SECONDARY, entry.name);
  }
}

void SystemMenuPage::init() {
  MenuPage<system_menu_page_N>::init();
  prepare_menu_entries();
  uint8_t max_item = menu.get_number_of_items() - 1;
  ((MCLEncoder *)encoders[1])->max = max_item;
  if (encoders[1]->cur > max_item) {
    encoders[1]->cur = max_item;
    cur_row = max_item < visible_rows ? max_item : visible_rows - 1;
  }
  selected_item = encoders[1]->cur;
  encoders[1]->old = encoders[1]->cur;
}

const uint8_t *const menu_target_param[] PROGMEM = {
    nullptr,

    // 1
    &mcl_cfg.ram_page_mode,

    // 2
    &mcl_cfg.uart1_turbo_speed, &mcl_cfg.uart2_turbo_speed, &mcl_cfg.uart2_device,
    &mcl_cfg.clock_rec, &mcl_cfg.clock_send, &mcl_cfg.midi_forward_1,

    // 8
    &mcl_cfg.auto_normalize, &mcl_cfg.uart2_ctrl_chan,

    // 10
    &mcl_cfg.load_mode, &mcl_cfg.uart_cc_fwd, &mcl_cfg.usb_mode,

    // 13
    &mcl_cfg.display_mirror,

    // 14
    &opt_trackid, &SeqPage::mask_type, &SeqPage::pianoroll_mode,
    &SeqPage::param_select, &SeqPage::slide, &seq_ptc_page.transpose,
    &SeqPage::velocity, &SeqPage::cond, &opt_speed, &opt_length, &opt_channel,
    &opt_copy, &opt_clear, &opt_paste, &opt_shift, &opt_reverse,

    // 30
    &mcl_cfg.rec_automation,

    // 31
    &grid_page.cur_grid, &mcl_cfg.load_mode, &slot.link.loops,
    &slot.link.row, &grid_page.slot_apply, &grid_page.slot_clear,
    &grid_page.slot_copy, &grid_page.slot_paste, &slot.link.length,

    // 40
#ifdef WAV_DESIGNER
    &WavDesignerPage::opt_mode, &WavDesignerPage::opt_shape,
#else
    nullptr, nullptr,
#endif
    // 42
    &mcl_cfg.rec_quant,
    // 43
    &opt_import_src, &opt_import_dest, &opt_import_count,
    // 46
    &mcl_cfg.uart2_poly_chan,
    // 47
    &mcl_cfg.uart2_prg_in,
    // 48
    &mcl_cfg.uart2_prg_out,
    // 49
    &mcl_cfg.uart2_prg_mode,
    // 50
    &mcl_cfg.seq_dev,
    // 51
    &mcl_cfg.midi_forward_2,
    // 52
    &mcl_cfg.midi_forward_usb,
    // 53
    &mcl_cfg.midi_transport_rec,
    // 54
    &mcl_cfg.midi_transport_send,
    // 55
    &mcl_cfg.usb_turbo_speed,
    // 56
    &mcl_cfg.midi_ctrl_port,
    // 57
    &mcl_cfg.md_trig_channel,
    // 58
    &perf_page.page_mode,
    // 59
    &perf_page.perf_id,
    // 60
    &mcl_cfg.uart2_cc_mute,
    // 61
    &mcl_cfg.uart1_device,
    // 62
    &mcl_cfg.grid_page_mode,
    // 63,
    &opt_transpose,
    // 64
    &mcl_cfg.uart_note_fwd,
    // 65
    &mcl_cfg.usb_device,
    // 66
    &mcl_cfg.grid_x_device,
    // 67
    &mcl_cfg.grid_x_port,
    // 68
    &mcl_cfg.grid_y_device,
    // 69
    &mcl_cfg.grid_y_port,
    // 70
    &opt_lfo_mult,
    // 71
    &grid_page.slot_load_sound,
    // 72
    &mcl_cfg.project_config,
    // 73
    &opt_swing
};

const menu_function_ptr_t menu_target_functions[] PROGMEM = {
    // 1 - mclsys_apply_config
    { .fn = mclsys_apply_config },
    // 2 - new_project_from_menu
    { .fn = new_project_from_menu },
    // 3 - opt_trackid_handler
    { .fn = opt_trackid_handler },
    // 4 - opt_mask_handler
    { .fn = opt_mask_handler },
    // 5 - opt_speed_handler
    { .fn = opt_speed_handler },
    // 6 - opt_length_handler
    { .fn = opt_length_handler },
    // 7 - opt_channel_handler
    { .fn = opt_channel_handler },
    // 8 - opt_copy_track_handler_cb
    { .fn = opt_copy_track_handler_cb },
    // 9 - opt_clear_track_handler
    { .fn = opt_clear_track_handler },
    // 10 - opt_clear_locks_handler
    { .fn = opt_clear_locks_handler },
    // 11 - opt_paste_track_handler
    { .fn = opt_paste_track_handler },
    // 12 - opt_shift_track_handler
    { .fn = opt_shift_track_handler },
    // 13 - opt_reverse_track_handler
    { .fn = opt_reverse_track_handler },
    // 14 - seq_menu_handler
    { .fn = seq_menu_handler },
    // 15 - rename_row
    { .fn = rename_row },
    // 16 - apply_slot_changes_cb
    { .fn = apply_slot_changes_cb },
    // 17 - wav_render
    { .fn = wav_render },
    // 18 - wavdesign_menu_handler
    { .fn = wavdesign_menu_handler },
    // 19 - mclsys_apply_config_midi
    { .fn = mclsys_apply_config_midi },
    // 20 - md_import
    { .fn = md_import },
    // 21 - usb_dfu_mode
    { .fn = usb_dfu_mode },
    // 22 - usb_os_update
    { .fn = usb_os_update },
    // 23 - usb_disk_mode
    { .fn = usb_disk_mode },
    // 24 - mcl_setup
    { .fn = mcl_setup },
    // 25 - rename_perf
    { .fn = rename_perf },
    // 26 - opt_transpose_track_handler
    { .fn = opt_transpose_track_handler },
    // 27 - open_primary_driver_config
    { .fn = open_primary_driver_config },
    // 28 - open_secondary_driver_config
    { .fn = open_secondary_driver_config },
    // 29 - opt_swing_handler
    { .fn = opt_swing_handler },
};
MenuPage<aux_config_page_N> aux_config_page(&menu_value_encoder,
                                            &menu_entry_encoder);

BootMenuPage<boot_menu_page_N> boot_menu_page(&menu_value_encoder,
                                              &menu_entry_encoder);
MenuPage<start_menu_page_N> start_menu_page(&menu_value_encoder,
                                            &menu_entry_encoder);
SystemMenuPage system_page(&menu_value_encoder, &menu_entry_encoder);
MenuPage<midi_config_page_N> midi_config_page(&menu_value_encoder,
                                              &menu_entry_encoder);

MenuPage<md_config_page_N> md_config_page(&menu_value_encoder,
                                          &menu_entry_encoder);
MenuPage<chain_config_page_N> chain_config_page(&menu_value_encoder,
                                                &menu_entry_encoder);
MenuPage<mcl_config_page_N> mcl_config_page(&menu_value_encoder,
                                            &menu_entry_encoder);
MenuPage<md_import_page_N> md_import_page(&menu_value_encoder,
                                          &menu_entry_encoder);

MenuPage<midiport_menu_page_N> midiport_menu_page(&menu_value_encoder,
                                                  &menu_entry_encoder);
MenuPage<mididevice_menu_page_N> mididevice_menu_page(&menu_value_encoder,
                                                      &menu_entry_encoder);
MenuPage<gridx_menu_page_N> gridx_menu_page(&menu_value_encoder,
                                            &menu_entry_encoder);
MenuPage<gridy_menu_page_N> gridy_menu_page(&menu_value_encoder,
                                            &menu_entry_encoder);
MenuPage<port1_menu_page_N> port1_menu_page(&menu_value_encoder,
                                            &menu_entry_encoder);
MenuPage<port2_menu_page_N> port2_menu_page(&menu_value_encoder,
                                            &menu_entry_encoder);
MenuPage<usbport_menu_page_N> usbport_menu_page(&menu_value_encoder,
                                                &menu_entry_encoder);
MenuPage<midiprogram_menu_page_N> midiprogram_menu_page(&menu_value_encoder,
                                                        &menu_entry_encoder);
MenuPage<midiclock_menu_page_N> midiclock_menu_page(&menu_value_encoder,
                                                    &menu_entry_encoder);
MenuPage<midiroute_menu_page_N> midiroute_menu_page(&menu_value_encoder,
                                                    &menu_entry_encoder);
MenuPage<midimachinedrum_menu_page_N> midimachinedrum_menu_page(
    &menu_value_encoder, &menu_entry_encoder);
MenuPage<midigeneric_menu_page_N> midigeneric_menu_page(&menu_value_encoder,
                                                        &menu_entry_encoder);
MenuPage<midicontrolinput_menu_page_N> midicontrolinput_menu_page(
    &menu_value_encoder, &menu_entry_encoder);
MenuPage<midicontroloutput_menu_page_N> midicontroloutput_menu_page(
    &menu_value_encoder, &menu_entry_encoder);

MCLEncoder input_encoder1(0, 127, ENCODER_RES_SYS);
MCLEncoder input_encoder2(0, 127, ENCODER_RES_SYS);

TextInputPage text_input_page(&input_encoder1, &input_encoder2);

MCLEncoder file_menu_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<file_menu_page_N> file_menu_page(&menu_value_encoder,
                                          &file_menu_encoder);

MCLEncoder seq_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder seq_menu_entry_encoder(0, 9, ENCODER_RES_PAT);
MenuPage<seq_menu_page_N> seq_menu_page(&seq_menu_value_encoder, &seq_menu_entry_encoder);
MenuPage<perf_menu_page_N> perf_menu_page(&seq_menu_value_encoder, &seq_menu_entry_encoder);

MCLEncoder grid_slot_param1(0, 7, ENCODER_RES_PAT);
MCLEncoder grid_slot_param2(0, 16, ENCODER_RES_PAT);
MenuPage<grid_slot_page_N> grid_slot_page(&grid_slot_param1, &grid_slot_param2);

MCLEncoder wavdesign_menu_value_encoder(0, 16, ENCODER_RES_PAT);
MCLEncoder wavdesign_menu_entry_encoder(0, 4, ENCODER_RES_PAT);
MenuPage<wavdesign_menu_page_N> wavdesign_menu_page(&wavdesign_menu_value_encoder, &wavdesign_menu_entry_encoder);
