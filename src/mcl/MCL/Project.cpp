#include "MCLDefines.h"
#include "MCLPlatformFeatures.h"
#include "Project.h"
#include "MCLSd.h"
#include "MCLGUI.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "Devices/MidiSetup.h"
#include "Sequencer/SeqTrackUtil.h"
#include "oled.h"
#include "Devices/DeviceManager.h"

#include "MDTrack.h"
#include "../Drivers/MD/MD.h"
#include "ExtTrack.h"
#include "A4Track.h"
#include "MNMTrack.h"
#include "MDFXTrack.h"
#include "MDRouteTrack.h"
#include "MDTempoTrack.h"
#include "EmptyTrack.h"
#include "Performance/PerfTrack.h"
#include "GridChainTrack.h"
#include "LFOSeqTrack.h"
#if MCL_FEATURE_HOST_ARRANGER
#include "SPSXTrack.h"
#include "Arrangement/MCLArrangement.h"
#include "Arrangement/MCLArrangementFormat.h"
#include "Host/SpsHostArrBridge.h"
#include "Host/SpsHostSeqBridge.h"
#endif
#include <stddef.h>

namespace {

bool copy_grid_row_slots_raw(Grid &src_grid, Grid &dst_grid, GridRow row) {
  uint8_t buf[256];
  static_assert((GRID_WIDTH * GRID_SLOT_BYTES) % sizeof(buf) == 0,
                "row slot copy buffer must divide row slot bytes");
  if (!src_grid.seek(0, row) || !dst_grid.seek(0, row)) {
    return false;
  }

  constexpr uint16_t chunks = (GRID_WIDTH * GRID_SLOT_BYTES) / sizeof(buf);
  for (uint16_t i = 0; i < chunks; i++) {
    if (!src_grid.read(buf, sizeof(buf)) || !dst_grid.write(buf, sizeof(buf))) {
      return false;
    }
  }
  return true;
}

#if MCL_FEATURE_HOST_ARRANGER
bool build_arrangement_leaf(uint8_t idx, bool private_source_file, char *out,
                            size_t out_len) {
  if (out == nullptr || out_len < 8) {
    return false;
  }
  out[0] = (char)('0' + idx / 100);
  out[1] = (char)('0' + (idx / 10) % 10);
  out[2] = (char)('0' + idx % 10);
  out[3] = '.';
  out[4] = private_source_file ? 'l' : 'a';
  out[5] = private_source_file ? 'o' : 'r';
  out[6] = private_source_file ? 'c' : 'r';
  out[7] = '\0';
  return true;
}

bool build_arrangement_relative_path(uint8_t idx, char *out, size_t out_len) {
  char leaf[8];
  return build_arrangement_leaf(idx, false, leaf, sizeof(leaf)) &&
         MCLSd::join_path(out, (uint8_t)out_len, mclarrfile::kDirName, leaf);
}

bool build_project_arrangement_path(const char *project, uint8_t idx,
                                    bool private_source_file, char *out,
                                    size_t out_len) {
  char leaf[8];
  char rel[16];
  return build_arrangement_leaf(idx, private_source_file, leaf, sizeof(leaf)) &&
         MCLSd::join_path(rel, sizeof(rel), mclarrfile::kDirName, leaf) &&
         MCLSd::join_path(out, (uint8_t)out_len, project, rel);
}

bool arrangement_exists_current_project(uint8_t idx) {
  char path[16];
  return build_arrangement_relative_path(idx, path, sizeof(path)) &&
         SD.exists(path);
}

bool ensure_arrangement_dir_current_project() {
  if (SD.exists(mclarrfile::kDirName)) {
    return true;
  }
  return SD.mkdir(mclarrfile::kDirName, true);
}

bool write_default_arrangement_current_project() {
  if (!ensure_arrangement_dir_current_project()) {
    return false;
  }
  char path[16];
  if (!build_arrangement_relative_path(0, path, sizeof(path))) {
    return false;
  }
  if (SD.exists(path)) {
    return true;
  }

  File arr_file;
  if (!arr_file.open(path, O_RDWR | O_CREAT | O_EXCL)) {
    return false;
  }
  mclarrfile::Header header;
  mclarrfile::initHeader(header, "main");
  bool ok = mcl_sd.write_data(&header, sizeof(header), &arr_file);
  ok = ok && arr_file.truncate(sizeof(header));
  ok = ok && arr_file.sync();
  arr_file.close();
  return ok;
}

bool ensure_arrangements_current_project(uint8_t *active_idx) {
  if (!write_default_arrangement_current_project()) {
    return false;
  }
  if (active_idx != nullptr && !arrangement_exists_current_project(*active_idx)) {
    *active_idx = 0;
  }
  return true;
}

bool copy_arrangement_files(const char *from_project, const char *to_project) {
  char to_dir[PRJ_PATH_LEN + 5];
  if (!MCLSd::join_path(to_dir, sizeof(to_dir), to_project,
                        mclarrfile::kDirName)) {
    return false;
  }
  if (!SD.exists(to_dir) && !SD.mkdir(to_dir, true)) {
    return false;
  }

  bool copied_arrangement = false;
  for (uint16_t idx = 0; idx <= mclarrfile::kMaxArrangementIndex; ++idx) {
    for (uint8_t private_source_file = 0; private_source_file < 2;
         ++private_source_file) {
      char src_path[PRJ_PATH_LEN + 16];
      char dst_path[PRJ_PATH_LEN + 16];
      if (!build_project_arrangement_path(
              from_project, (uint8_t)idx, private_source_file != 0, src_path,
              sizeof(src_path)) ||
          !build_project_arrangement_path(
              to_project, (uint8_t)idx, private_source_file != 0, dst_path,
              sizeof(dst_path))) {
        return false;
      }
      if (!SD.exists(src_path)) {
        continue;
      }
      if (!mcl_sd.copy_file(src_path, dst_path)) {
        return false;
      }
      if (!private_source_file) {
        copied_arrangement = true;
      }
    }
  }

  if (copied_arrangement) {
    return true;
  }

  char default_path[PRJ_PATH_LEN + 16];
  if (!build_project_arrangement_path(to_project, 0, false, default_path,
                                      sizeof(default_path))) {
    return false;
  }
  if (SD.exists(default_path)) {
    return true;
  }
  File arr_file;
  if (!arr_file.open(default_path, O_RDWR | O_CREAT | O_EXCL)) {
    return false;
  }
  mclarrfile::Header header;
  mclarrfile::initHeader(header, "main");
  bool ok = mcl_sd.write_data(&header, sizeof(header), &arr_file);
  ok = ok && arr_file.truncate(sizeof(header));
  ok = ok && arr_file.sync();
  arr_file.close();
  return ok;
}
#endif

constexpr size_t LEGACY_GRID_TRACK_HEADER_SIZE =
    sizeof(uint8_t) * 3 + sizeof(GridLink);

class ATTR_PACKED() LegacyGridTrackHeader {
public:
  uint8_t version[2];
  uint8_t active;
  GridLink link;
};

enum LegacyMigrationStatus : uint8_t {
  LEGACY_MIGRATE_OK,
  LEGACY_MIGRATE_TYPE_MISMATCH,
  LEGACY_MIGRATE_ERROR,
};

class ATTR_PACKED() LegacyMDLFOTrackStorage {
public:
  LegacyGridTrackHeader header;
  LegacyLFOSeqTrackData lfo_data;
};

class ATTR_PACKED() LegacyMDRouteTrackStorage {
public:
  LegacyGridTrackHeader header;
  LegacyMDRouteData route;
};

class ATTR_PACKED() MigratedMDRouteTrackStorage {
public:
  LegacyGridTrackHeader header;
  MDRouteData route;
};

class ATTR_PACKED() LegacyPerfTrackData {
public:
  PerfTrackEncoderData encs[4];
  PerfScene scenes[NUM_SCENES];
  MuteSet mute_sets[2];
  uint8_t perf_locks[4][4];
};

class ATTR_PACKED() LegacyPerfTrackStorage {
public:
  LegacyGridTrackHeader header;
  LegacyPerfTrackData data;
};

class ATTR_PACKED() MigratedPerfTrackStorage {
public:
  LegacyGridTrackHeader header;
  PerfTrackData data;
};

class ATTR_PACKED() LegacyFixedPayloadTrackStorage {
public:
  LegacyGridTrackHeader header;
  uint8_t payload[sizeof(MDFXData)];
};

class ATTR_PACKED() LegacyMDSeqStepDescriptor {
public:
  uint8_t locks;
  bool locks_enabled : 1;
  bool trig : 1;
  bool slide : 1;
  bool cond_plock : 1;
  uint8_t cond_id : 4;
};

class ATTR_PACKED() LegacyMDSeqTrackData {
public:
  uint8_t locks[NUM_MD_LOCK_SLOTS];
  uint8_t locks_params[NUM_LOCKS];
  uint8_t timing[NUM_MD_STEPS];
  LegacyMDSeqStepDescriptor steps[NUM_MD_STEPS];
};

class ATTR_PACKED() LegacyMDTrackStorage {
public:
  LegacyGridTrackHeader header;
  LegacyMDSeqTrackData seq;
  MDMachine machine;
};

class ATTR_PACKED() MigratedMDTrackStorage {
public:
  LegacyGridTrackHeader header;
  MDMachine machine;
  SeqTrackModData mod_data;
  MDSeqTrackData seq_data;
  TrackLoadFadeData load_fade;
};

struct ATTR_PACKED() LegacyExtEvent {
  bool is_lock : 1;
  uint8_t lock_idx : 3;
  uint8_t cond_id : 4;
  bool event_on : 1;
  uint8_t event_value : 7;
  uint8_t micro_timing;
};

class ATTR_PACKED() LegacyExtSeqTrackData {
public:
  NibbleArray<NUM_EXT_STEPS> event_buckets;
  LegacyExtEvent events[NUM_EXT_EVENTS];
  uint8_t locks_params[NUM_LOCKS];
  uint16_t event_count;
  uint8_t velocities[NUM_EXT_STEPS];
  uint8_t locks_params_orig[NUM_LOCKS];
  uint8_t channel;
};

class ATTR_PACKED() LegacyExtSeqFixedTail {
public:
  uint8_t locks_params[NUM_LOCKS];
  uint16_t event_count;
  uint8_t velocities[NUM_EXT_STEPS];
  uint8_t locks_params_orig[NUM_LOCKS];
  uint8_t channel;
};

static_assert(sizeof(LegacyGridTrackHeader) == LEGACY_GRID_TRACK_HEADER_SIZE,
              "origin/dev track header size changed");
static_assert(GridTrack::STORAGE_HEADER_SIZE == LEGACY_GRID_TRACK_HEADER_SIZE,
              "current track header prefix changed");
static_assert(sizeof(LegacyPerfTrackData) + 18 == sizeof(PerfTrackData),
              "legacy PerfTrack payload conversion size changed");
static_assert(sizeof(LegacyGridTrackHeader) + sizeof(LegacyPerfTrackData) ==
                  491,
              "origin/dev PerfTrack storage size changed");
static_assert(sizeof(LegacyPerfTrackStorage) == 491,
              "origin/dev PerfTrack storage size changed");
static_assert(sizeof(MigratedPerfTrackStorage) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(PerfTrackData),
              "migrated PerfTrack storage size changed");
static_assert(sizeof(MigratedPerfTrackStorage) ==
                  sizeof(PerfTrack) - sizeof(void *),
              "PerfTrack migration storage layout changed");
static_assert(sizeof(LegacyMDSeqStepDescriptor) ==
                  sizeof(MDSeqStepDescriptor),
              "Legacy MD step descriptor size changed");
static_assert(sizeof(LegacyMDSeqTrackData) == sizeof(MDSeqTrackDataV1),
              "Legacy MD seq data size changed");
static_assert(sizeof(LegacyExtEvent) == sizeof(ext_event_t),
              "Legacy Ext event size changed");
static_assert(sizeof(LegacyExtSeqTrackData) == sizeof(ExtSeqTrackData),
              "Legacy Ext seq data size changed");
static_assert(offsetof(LegacyExtSeqTrackData, locks_params) ==
                  sizeof(NibbleArray<NUM_EXT_STEPS>) +
                      sizeof(LegacyExtEvent) * NUM_EXT_EVENTS,
              "Legacy Ext fixed tail offset changed");
static_assert(sizeof(LegacyExtSeqFixedTail) ==
                  sizeof(LegacyExtSeqTrackData) -
                      offsetof(LegacyExtSeqTrackData, locks_params),
              "Legacy Ext fixed tail size changed");
static_assert(offsetof(ExtSeqTrackData, locks_params) ==
                  sizeof(NibbleArray<NUM_EXT_STEPS>),
              "Ext fixed tail offset changed");
static_assert(offsetof(ExtSeqTrackData, events) -
                      offsetof(ExtSeqTrackData, locks_params) ==
                  sizeof(LegacyExtSeqFixedTail),
              "Ext fixed tail size changed");
static_assert(offsetof(LegacyMDLFOTrackStorage, lfo_data) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE,
              "Legacy MD LFO storage prefix changed");
static_assert(sizeof(LegacyMDLFOTrackStorage) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(LegacyLFOSeqTrackData),
              "Legacy MD LFO storage size changed");
static_assert(offsetof(LegacyMDRouteTrackStorage, route) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE,
              "Legacy MD route storage prefix changed");
static_assert(sizeof(LegacyMDRouteTrackStorage) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(LegacyMDRouteData),
              "Legacy MD route storage size changed");
static_assert(sizeof(MigratedMDRouteTrackStorage) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(MDRouteData),
              "migrated MD route storage size changed");
static_assert(sizeof(MigratedMDRouteTrackStorage) ==
                  sizeof(MDRouteTrack) - sizeof(void *),
              "MD route migration storage layout changed");
static_assert(sizeof(LegacyFixedPayloadTrackStorage) ==
                  LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(MDFXData),
              "fixed payload migration scratch size changed");
static_assert(LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(LegacyMDSeqTrackData) +
                      sizeof(MDMachine) ==
                  534,
              "origin/dev MDTrack storage size changed");
static_assert(sizeof(LegacyMDTrackStorage) == 534,
              "origin/dev MDTrack storage size changed");
static_assert(sizeof(MigratedMDTrackStorage) ==
                  sizeof(MDTrack) - sizeof(void *),
              "MDTrack migration storage layout changed");
constexpr uint16_t PROJECT_RENAME_SUFFIXES =
    256;
constexpr size_t PROJECT_CONFIG_OFFSET =
    offsetof(MCLSysConfigData, uart1_turbo_speed);
constexpr size_t PROJECT_CONFIG_SIZE =
    offsetof(MCLSysConfigData, project_config) - PROJECT_CONFIG_OFFSET;
constexpr size_t PROJECT_CONFIG_SIZE_LEGACY =
    offsetof(MCLSysConfigData, md_sample_bank_capture);
constexpr size_t PROJECT_HEADER_SIZE_LEGACY_CONFIG =
    offsetof(ProjectHeader, cfg) + PROJECT_CONFIG_SIZE_LEGACY;
// Size of files saved before manual-step-mode fields existed (i.e. through
// active_arrangement_idx, the previous tail field).
constexpr size_t PROJECT_CONFIG_SIZE_PRE_MANUAL_STEP =
    offsetof(MCLSysConfigData, manual_step_enabled);
constexpr size_t PROJECT_HEADER_SIZE_PRE_MANUAL_STEP =
    offsetof(ProjectHeader, cfg) + PROJECT_CONFIG_SIZE_PRE_MANUAL_STEP;

#ifdef MCL_HAS_PROJECT_CONVERSION
#define PROJECT_VERSION_CAN_OPEN(v)                                           \
  ((v) == PROJ_MIN_READABLE_VERSION || (v) == PROJ_VERSION)
#define PROJECT_CONVERSION_ATTR() __attribute__((noinline, cold))
#else
#define PROJECT_VERSION_CAN_OPEN(v) ((v) == PROJ_VERSION)
#endif

bool project_config_version_valid(uint32_t version) {
  return version == CONFIG_VERSION;
}

bool project_config_valid(const MCLSysConfigData &source) {
  return project_config_version_valid(source.version);
}

size_t project_header_read_size(File &file) {
  uint32_t file_size = file.fileSize();
  if (file_size >= sizeof(ProjectHeader)) {
    return sizeof(ProjectHeader);
  }
  if (file_size >= PROJECT_HEADER_SIZE_PRE_MANUAL_STEP) {
    return PROJECT_HEADER_SIZE_PRE_MANUAL_STEP;
  }
  if (file_size >= PROJECT_HEADER_SIZE_LEGACY_CONFIG) {
    return PROJECT_HEADER_SIZE_LEGACY_CONFIG;
  }
  return sizeof(ProjectHeader);
}

bool read_project_header_body(ProjectHeader *self, File &file) NOINLINE();
bool read_project_header_body(ProjectHeader *self, File &file) {
  size_t header_size = project_header_read_size(file);
  memset((uint8_t *)self + sizeof(self->version), 0,
         sizeof(ProjectHeader) - sizeof(self->version));
  return mcl_sd.read_data((uint8_t *)self + sizeof(self->version),
                          header_size - sizeof(self->version), &file);
}

bool project_config_has_sample_bank(const MCLSysConfigData &source) {
  return source.version == CONFIG_VERSION;
}

uint8_t normalized_sample_bank_setting(uint8_t sample_bank) {
  return sample_bank <= MD_SAMPLE_BANK_FIXED_LAST ? sample_bank
                                                  : MD_SAMPLE_BANK_OFF;
}

uint8_t project_config_sample_bank_setting(const MCLSysConfigData &source) {
  if (!project_config_has_sample_bank(source)) {
    return MD_SAMPLE_BANK_OFF;
  }
  return normalized_sample_bank_setting(source.md_sample_bank);
}

uint8_t project_config_sample_bank_to_load(const MCLSysConfigData &source) {
  uint8_t setting = project_config_sample_bank_setting(source);
  if (setting >= MD_SAMPLE_BANK_FIXED_FIRST &&
      setting <= MD_SAMPLE_BANK_FIXED_LAST) {
    return setting;
  }
  return 0;
}

void clear_project_sample_bank(MCLSysConfigData *data) {
  if (data == nullptr) {
    return;
  }
  data->md_sample_bank = MD_SAMPLE_BANK_OFF;
  data->md_sample_bank_capture = 0;
}

bool project_config_has_manual_step(const MCLSysConfigData &source) {
  return source.version == CONFIG_VERSION;
}

// Always defaults manual-step mode OFF rather than trusting/inheriting it,
// since silently resurrecting it on an untrusted/older config could start
// stepping the MD sequencer off a stale CC binding.
void clear_project_manual_step(MCLSysConfigData *data) {
  if (data == nullptr) {
    return;
  }
  data->manual_step_enabled = 0;
  data->manual_step_cc = 0;
  data->manual_step_port = MANUAL_STEP_PORT_MIDI2;
}

#ifdef MCL_HAS_PROJECT_CONVERSION
void init_migrated_header(LegacyGridTrackHeader &dst,
                          const LegacyGridTrackHeader &src,
                          uint8_t track_type, uint8_t version) {
  dst.version[0] = version;
  dst.version[1] = 0;
  dst.active = track_type;
  dst.link = src.link;
}

void copy_legacy_perf_track_data(PerfTrackData &dst,
                                 const LegacyPerfTrackData &src) {
  memcpy(dst.encs, src.encs, sizeof(dst.encs));
  memcpy(dst.scenes, src.scenes, sizeof(dst.scenes));
  memcpy(dst.perf_locks, src.perf_locks, sizeof(dst.perf_locks));
  dst.load_perf_state = 255;
  dst.load_type_mask = 0;

  uint8_t bit = 1;
  uint8_t load_bit = 0x10;
  for (uint8_t n = 0; n < 4; n++, bit <<= 1, load_bit <<= 1) {
    dst.perf_states[n].mute_mask[0] = src.mute_sets[0].mutes[n];
    dst.perf_states[n].fill_mask[0] = 0;
    dst.perf_states[n].fill_mask[1] = 0;

    uint16_t mutes = src.mute_sets[1].mutes[n];
    if ((mutes & 0x8000) == 0) {
      dst.load_perf_state = n;
    }
    if (mutes & 0x2000) {
      dst.load_type_mask |= bit;
    }
    if (mutes & 0x4000) {
      dst.load_type_mask |= load_bit;
    }
    dst.perf_states[n].mute_mask[1] = mutes | 0xE000;
  }
}

uint8_t project_seq_speed_value(const GridLink &link) {
  return link.speed_value() & 0x7F;
}

void convert_md_seq_unsigned_timing(MDSeqTrackData &data,
                                    const LegacyMDSeqTrackData &src,
                                    uint8_t speed) {
  uint16_t ticks_per_step = SeqTrack::get_ticks_per_step(speed);
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    uint8_t timing = src.timing[step];
    if (timing == 0) {
      timing = ticks_per_step;
    }
    data.microtiming[step] =
        SeqTrack::timing_to_microtiming(timing, ticks_per_step);
  }
}

void convert_ext_seq_unsigned_timing(ExtSeqTrackData &data, uint8_t speed) {
  uint16_t ticks_per_step = SeqTrack::get_ticks_per_step(speed);
  uint16_t used_events = data.event_count;
  if (used_events > NUM_EXT_EVENTS) {
    used_events = NUM_EXT_EVENTS;
  }
  for (uint16_t i = 0; i < used_events; i++) {
    uint8_t timing =
        *reinterpret_cast<uint8_t *>(&data.events[i].micro_timing);
    if (timing == 0) {
      timing = ticks_per_step;
    }
    data.events[i].micro_timing =
        SeqTrack::timing_to_microtiming(timing, ticks_per_step);
  }
}

void copy_legacy_md_locks(MDSeqTrackData &dst,
                          const LegacyMDSeqTrackData &src) {
  memset(dst.locks, 0, sizeof(dst.locks));
  memcpy(dst.locks_params, src.locks_params, sizeof(dst.locks_params));
  memset(dst.steps, 0, sizeof(dst.steps));
  dst.slide_mask = 0;

  uint16_t src_lock = 0;
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    uint8_t legacy_locks = src.steps[step].locks;
    bool locks_enabled = src.steps[step].locks_enabled;
    dst.steps[step].trig = src.steps[step].trig;
    dst.steps[step].cond_plock = src.steps[step].cond_plock;
    dst.steps[step].cond_id = seq_legacy_cond_to_stepseq(src.steps[step].cond_id);
    if (src.steps[step].slide) {
      SET_BIT64(dst.slide_mask, step);
    }
    for (uint8_t lock = 0; lock < NUM_LOCKS; lock++) {
      uint8_t lock_mask = 1 << lock;
      if (!(legacy_locks & lock_mask)) {
        continue;
      }

      if (src_lock < NUM_MD_LOCK_SLOTS && locks_enabled &&
          src.locks_params[lock]) {
        dst.set_track_locks_i(step, lock, src.locks[src_lock]);
      }
      src_lock++;
    }
  }
}

void copy_legacy_md_seq(MDSeqTrackData &dst, const LegacyMDSeqTrackData &src,
                        uint8_t speed) {
  // Every MDSeqTrackData field is populated here; legacy lock-enable is folded
  // into the migrated packed lock data below.
  convert_md_seq_unsigned_timing(dst, src, speed);

  copy_legacy_md_locks(dst, src);
  // Keep stale legacy lock params. Legacy playback used nonzero params with
  // no step lock bit to send kit-value resets on trig steps.
  dst.swing_amount = 0;
  dst.set_default_swing();
  dst.mute_mask = 0;
}

void convert_legacy_ext_seq(ExtSeqTrackData &dst, uint8_t speed) {
  uint8_t *raw = reinterpret_cast<uint8_t *>(&dst);
  LegacyExtSeqFixedTail fixed_tail;
  memcpy(&fixed_tail, raw + offsetof(LegacyExtSeqTrackData, locks_params),
         sizeof(fixed_tail));

  uint16_t used_events = fixed_tail.event_count;
  if (used_events > NUM_EXT_EVENTS) {
    used_events = NUM_EXT_EVENTS;
  }
  memmove(dst.events, raw + offsetof(LegacyExtSeqTrackData, events),
          used_events * sizeof(LegacyExtEvent));

  memcpy(dst.locks_params, &fixed_tail, sizeof(fixed_tail));
  dst.event_count = used_events;
  for (uint16_t i = 0; i < used_events; i++) {
    if (!dst.events[i].is_lock) {
      uint8_t condition = seq_legacy_cond_to_stepseq(dst.events[i].cond_id);
      ext_event_set_condition(dst.events[i], condition);
    }
  }
  convert_ext_seq_unsigned_timing(dst, speed);
}

LegacyMigrationStatus read_legacy_ext_track(Grid &grid, GridColumn column,
                                            GridRow row, DeviceTrack &upgraded,
                                            ExtSeqTrackData &seq_data) {
  uint8_t track_type = upgraded.active;
  if (!grid.read(upgraded._this(),
                 LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(LegacyExtSeqTrackData),
                 column, row)) {
    return LEGACY_MIGRATE_ERROR;
  }
  if (upgraded.active != track_type) {
    return LEGACY_MIGRATE_TYPE_MISMATCH;
  }
  upgraded.active = track_type;
  convert_legacy_ext_seq(seq_data, project_seq_speed_value(upgraded.link));
  return LEGACY_MIGRATE_OK;
}

bool write_migrated_track(Grid &grid, GridColumn column, GridRow row,
                          DeviceTrack &track, uint16_t write_size) {
  track.version = track.storage_version();
  track.reserved = 0;
  return grid.write(track._this(), write_size, column, row);
}

bool migrate_empty_track_header(Grid &src_grid, Grid &dst_grid,
                                GridColumn column, GridRow row) {
  LegacyGridTrackHeader header;
  if (!src_grid.read(&header, sizeof(header), column, row)) {
    return false;
  }
  header.version[0] = 0;
  header.version[1] = 0;
  header.active = EMPTY_TRACK_TYPE;
  return dst_grid.write(&header, sizeof(header), column, row);
}

void init_upgraded_md_track(MigratedMDTrackStorage &dst, const GridLink &link,
                            const LegacyMDSeqTrackData &seq,
                            const MDMachine &machine) {
  dst.header.version[0] = SEQ_TRACK_LOAD_FADE_STORAGE_VERSION;
  dst.header.version[1] = 0;
  dst.header.active = MD_TRACK_TYPE;
  dst.header.link = link;
  dst.mod_data.init();
  dst.load_fade.init();
  dst.machine = machine;
  copy_legacy_md_seq(dst.seq_data, seq, project_seq_speed_value(link));
}

LegacyMigrationStatus read_upgraded_md_track(Grid &grid, GridColumn column,
                                             GridRow row,
                                             MigratedMDTrackStorage &dst) {
  LegacyMDTrackStorage legacy_track;
  if (!grid.read(&legacy_track, sizeof(legacy_track), column, row)) {
    return LEGACY_MIGRATE_ERROR;
  }
  if (legacy_track.header.active != MD_TRACK_TYPE) {
    return LEGACY_MIGRATE_TYPE_MISMATCH;
  }
  init_upgraded_md_track(dst, legacy_track.header.link, legacy_track.seq,
                         legacy_track.machine);
  return LEGACY_MIGRATE_OK;
}

LegacyMigrationStatus migrate_md_track_native_swing(Grid &src_grid,
                                                    Grid &dst_grid,
                                                    GridColumn column,
                                                    GridRow row) {
  MigratedMDTrackStorage upgraded;
  LegacyMigrationStatus status =
      read_upgraded_md_track(src_grid, column, row, upgraded);
  if (status != LEGACY_MIGRATE_OK) {
    return status;
  }
  return dst_grid.write(&upgraded, sizeof(upgraded), column, row)
             ? LEGACY_MIGRATE_OK
             : LEGACY_MIGRATE_ERROR;
}

LegacyMigrationStatus migrate_ext_like_track_storage(Grid &src_grid,
                                                     Grid &dst_grid,
                                                     GridColumn column,
                                                     GridRow row,
                                                     DeviceTrack &upgraded,
                                                     ExtSeqTrackData &seq_data) {
  upgraded.init_defaults();
  LegacyMigrationStatus status =
      read_legacy_ext_track(src_grid, column, row, upgraded, seq_data);
  if (status != LEGACY_MIGRATE_OK) {
    return status;
  }
  uint16_t payload_size = upgraded.get_sound_data_size();
  if (payload_size != 0 &&
      !src_grid.read(upgraded.get_sound_data_ptr(), payload_size)) {
    return LEGACY_MIGRATE_ERROR;
  }
  return write_migrated_track(dst_grid, column, row, upgraded,
                              upgraded.get_track_size())
             ? LEGACY_MIGRATE_OK
             : LEGACY_MIGRATE_ERROR;
}

bool handle_legacy_migration_status(LegacyMigrationStatus status,
                                    GridRowHeader &row_header, Grid &dst_grid,
                                    GridColumn column, GridRow row) {
  if (status == LEGACY_MIGRATE_OK) {
    return true;
  }
  if (status == LEGACY_MIGRATE_TYPE_MISMATCH) {
    row_header.update_model(column, 0, EMPTY_TRACK_TYPE);
    return dst_grid.clear_slot(column, row, false);
  }
  return false;
}

uint8_t fixed_payload_track_size(uint8_t track_type) {
  switch (track_type) {
  case MDTEMPO_TRACK_TYPE:
    return sizeof(TempoData);
  case GRIDCHAIN_TRACK_TYPE:
    return sizeof(GridChain);
  default:
    return sizeof(MDFXData);
  }
}

bool migrate_fixed_payload_track(Grid &src_grid, Grid &dst_grid,
                                 GridColumn column, GridRow row,
                                 uint8_t track_type) {
  static_assert(sizeof(MDFXData) >= sizeof(TempoData),
                "fixed payload scratch too small for tempo");
  static_assert(sizeof(MDFXData) >= sizeof(GridChain),
                "fixed payload scratch too small for grid chain");
  LegacyFixedPayloadTrackStorage storage;
  uint8_t storage_size =
      sizeof(storage.header) + fixed_payload_track_size(track_type);
  if (!src_grid.read(&storage, storage_size, column, row) ||
      storage.header.active != track_type) {
    return false;
  }
  init_migrated_header(storage.header, storage.header, track_type, 0);
  return dst_grid.write(&storage, storage_size, column, row);
}

bool migrate_perf_track_storage(Grid &src_grid, Grid &dst_grid,
                                GridColumn column, GridRow row,
                                GridColumn dst_column) {
  LegacyPerfTrackStorage legacy_track;
  if (!src_grid.read(&legacy_track, sizeof(legacy_track), column, row) ||
      legacy_track.header.active != PERF_TRACK_TYPE) {
    return false;
  }

  MigratedPerfTrackStorage upgraded;
  init_migrated_header(upgraded.header, legacy_track.header, PERF_TRACK_TYPE,
                       PERF_TRACK_STORAGE_VERSION_PERF_STATES);
  copy_legacy_perf_track_data(upgraded.data, legacy_track.data);
  if (!dst_grid.write(&upgraded, sizeof(upgraded), dst_column, row)) {
    return false;
  }
  if (dst_column != column) {
    legacy_track.header.version[0] = 0;
    legacy_track.header.version[1] = 0;
    legacy_track.header.active = EMPTY_TRACK_TYPE;
    return dst_grid.write(&legacy_track.header, sizeof(legacy_track.header),
                          column, row);
  }
  return true;
}

bool PROJECT_CONVERSION_ATTR() migrate_md_route_track_storage(
    Grid &src_grid, Grid &dst_grid, GridRowHeader &row_header, GridRow row,
    uint8_t poly_channel) {
  LegacyMDRouteTrackStorage legacy_route;
  if (!src_grid.read(&legacy_route, sizeof(legacy_route), MDROUTE_TRACK_NUM,
                     row)) {
    return false;
  }
  if (legacy_route.header.active != MDROUTE_TRACK_TYPE) {
    return dst_grid.clear_slot(MDROUTE_TRACK_NUM, row, false);
  }

  MigratedMDRouteTrackStorage new_route;
  init_migrated_header(new_route.header, legacy_route.header,
                       MD_ROUTE_TRACK_TYPE, 0);
  memcpy(new_route.route.routing, legacy_route.route.routing,
         sizeof(new_route.route.routing));
  if (poly_channel < PTC_MIDI_GROUP_MIN || poly_channel > PTC_MIDI_GROUP_MAX) {
    poly_channel = PTC_MIDI_GROUP_MIN;
  }
  uint16_t poly_mask = legacy_route.route.poly_mask;
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i, poly_mask >>= 1) {
    new_route.route.ptc_group[i] =
        (poly_mask & 1) ? poly_channel : PTC_GROUP_OFF;
  }

  if (!dst_grid.write(&new_route, sizeof(new_route), MDROUTE_TRACK_NUM, row)) {
    return false;
  }
  row_header.update_model(MDROUTE_TRACK_NUM, MD_ROUTE_TRACK_TYPE,
                          MD_ROUTE_TRACK_TYPE);
  return true;
}
#endif

void copy_project_config(MCLSysConfigData *dst,
                         const MCLSysConfigData &source) {
  dst->version = CONFIG_VERSION;
  dst->project[0] = '\0';
  dst->number_projects = 0;
  memcpy((uint8_t *)dst + PROJECT_CONFIG_OFFSET,
         (const uint8_t *)&source + PROJECT_CONFIG_OFFSET,
         PROJECT_CONFIG_SIZE);
  dst->project_config = 0;
  dst->md_sample_bank = normalized_sample_bank_setting(source.md_sample_bank);
  dst->md_sample_bank_capture = 0;
  dst->active_arrangement_idx = source.active_arrangement_idx;
  clear_project_manual_step(dst);
}

#ifdef MCL_HAS_PROJECT_CONVERSION
void normalize_project_config(MCLSysConfigData *data) {
  uint8_t sample_bank = project_config_sample_bank_setting(*data);
  data->version = CONFIG_VERSION;
  data->project[0] = '\0';
  data->number_projects = 0;
  data->project_config = 0;
  data->md_sample_bank = sample_bank;
  data->md_sample_bank_capture = 0;
  data->active_arrangement_idx = 0;
  clear_project_manual_step(data);
}
#endif

void apply_project_config(MCLSysConfigData *dst,
                          const MCLSysConfigData &source) {
  if (!project_config_valid(source)) {
    return;
  }
  memcpy((uint8_t *)dst + PROJECT_CONFIG_OFFSET,
         (const uint8_t *)&source + PROJECT_CONFIG_OFFSET,
         PROJECT_CONFIG_SIZE);
  dst->md_sample_bank = project_config_sample_bank_setting(source);
  dst->md_sample_bank_capture = 0;
  dst->active_arrangement_idx = source.active_arrangement_idx;
}

void load_project_sample_bank(uint8_t sample_bank) {
  if (sample_bank == 0) {
    return;
  }
  MidiDevice *primary = device_manager.primary_device();
  if (primary != &MD && !SeqTrackUtil::is_md_device(primary)) {
    return;
  }
  MD.loadSampleBank(sample_bank - 1);
}

} // namespace

void Project::draw_wait_popup(const char *message) {
  mcl_gui.draw_infobox("PLEASE WAIT", message);
  oled_display.display();
}

void Project::draw_upgrade_progress(GridIndex grid, GridRow row) {
#ifdef OLED_DISPLAY
  uint8_t progress = grid * (GRID_LENGTH / NUM_GRIDS) + row / NUM_GRIDS;
  mcl_gui.draw_progress_bar(progress, GRID_LENGTH, false, MCLGUI::s_progress_x,
                            21);
#endif
}

void Project::setup() {}

bool Project::new_project(const char *newprj) {
  const char *basename = nullptr;
  if (!split_project_path(newprj, &basename)) {
    gfx.alert_error("BAD NAME");
    return false;
  }

  char proj_filename[PRJ_NAME_LEN  + 5];
  if (!project_file_name(basename, proj_filename, sizeof(proj_filename))) {
    gfx.alert_error("BAD NAME");
    return false;
  }

  // Create parent project directory
  //
  chdir_projects();
  DEBUG_PRINTLN(newprj);
  DEBUG_PRINTLN(strlen(newprj));
  // Create project directory
  if (SD.exists(newprj)) {
    gfx.alert_error("PROJECT EXISTS");
    return false;
  }
  if (!SD.mkdir(newprj, true) || !SD.chdir(newprj)) {
    gfx.alert_error("DIR");
    return false;
  }

  draw_wait_popup("CREATING PROJECT");

#if MCL_FEATURE_HOST_ARRANGER
  uint8_t new_active_arrangement_idx = 0;
  if (!ensure_arrangements_current_project(&new_active_arrangement_idx)) {
    gfx.alert_error("SD ERROR");
    return false;
  }
#endif

  DEBUG_PRINTLN(proj_filename);
  if (SD.exists(proj_filename)) {
    gfx.alert_error("PROJECT EXISTS");
    return false;
  }

  // Initialise Grid Files.
  //

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_filename[PRJ_NAME_LEN  + 5];
    if (!build_grid_filename(basename, i, grid_filename,
                             sizeof(grid_filename))) {
      gfx.alert_error("BAD NAME");
      return false;
    }
    if (!SD.exists(grid_filename)) {
      if (!grids[i].new_grid(grid_filename, PROJ_VERSION, i, true)) {
        gfx.alert_error("SD ERROR");
        return false;
      }
    }
  }
  // Initialiase Project Master File.
  //
#if MCL_FEATURE_HOST_ARRANGER
  uint8_t previous_active_arrangement_idx = mcl_cfg.active_arrangement_idx;
  mcl_cfg.active_arrangement_idx = new_active_arrangement_idx;
#endif
  bool ret = proj.new_project_master_file(proj_filename);
#if MCL_FEATURE_HOST_ARRANGER
  mcl_cfg.active_arrangement_idx = previous_active_arrangement_idx;
#endif
  return ret;
}

bool Project::new_project_prompt(const char *parent) {
  char newprj[PRJ_NAME_LEN] = "project___";

  uint8_t project_number = mcl_cfg.number_projects;
  newprj[7] = project_number / 100 + '0';
  project_number %= 100;
  newprj[7 + 1] = project_number / 10 + '0';
  newprj[7 + 2] = project_number % 10 + '0';
again:
  if (mcl_gui.wait_for_input(newprj, "New Project:", PRJ_NAME_LEN - 1)) {
    char project_path[PRJ_PATH_LEN];
    if (parent != nullptr && parent[0] != '\0') {
      if (!MCLSd::join_path(project_path, sizeof(project_path), parent,
                            newprj)) {
        gfx.alert_error("BAD PATH");
        goto again;
      }
    } else {
      strcpy(project_path, newprj);
    }

    if (!new_project(project_path)) {
      goto again;
    }
    if (proj.load_project(project_path)) {
      grid_page.reload_slot_models = false;
      DEBUG_PRINTLN("project loaded, setting page to grid");
      mcl.setPage(GRID_PAGE);
      return true;
    } else {
      gfx.alert_error("SD ERROR");
      goto again;
    }
  }
  if (proj.project_loaded) {
    mcl.setPage(GRID_PAGE);
    return true;
  }
  return false;
}

void Project::chdir_projects() {
  char path[64];
  const char *c_project_root = mcl_sd.full_path(PRJ_DIR, path, sizeof(path));
  SD.mkdir(c_project_root, true);
  SD.chdir(c_project_root);
}

#ifdef MCL_HAS_PROJECT_CONVERSION
bool Project::convert_project(const char *projectname) { return true; }
#endif

bool Project::split_project_path(const char *projectname,
                                 const char **basename) const {
  size_t path_len = strlen(projectname);
  if (path_len == 0 || path_len >= PRJ_PATH_LEN) {
    DEBUG_PRINTLN("bad path len");
    return false;
  }

  const char *project_basename = strrchr(projectname, '/');
  project_basename = project_basename == nullptr ? projectname
                                                 : project_basename + 1;
  size_t name_len = strlen(project_basename);
  if (name_len == 0 || name_len > PRJ_NAME_LEN) {
    DEBUG_PRINTLN("bad name len");
    return false;
  }
  if (basename != nullptr) {
    *basename = project_basename;
  }
  return true;
}

bool Project::project_file_name(const char *basename, char *out,
                                size_t out_len) const {
  size_t name_len = strlen(basename);
  if (name_len == 0 || name_len > PRJ_NAME_LEN || name_len + 5 > out_len) {
    return false;
  }
  strcpy(out, basename);
  strcat(out, ".mcl");
  return true;
}

bool Project::build_grid_filename(const char *basename, uint8_t suffix,
                                  char *out, size_t out_len) const {
  size_t name_len = strlen(basename);
  if (name_len == 0 || name_len > PRJ_NAME_LEN || name_len + 5 > out_len) {
    return false;
  }
  memcpy(out, basename, name_len);
  out[name_len] = '.';
  mcl_gui.put_value_at(suffix, out + name_len + 1);
  return true;
}

uint8_t Project::project_pair_file_mask(uint8_t pair, const char *basename) {
  uint8_t mask = 0;
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_name[PRJ_NAME_LEN + 5];
    if (!build_grid_filename(basename, pair * NUM_GRIDS + i, grid_name,
                             sizeof(grid_name))) {
      return 0xFF;
    }
    if (SD.exists(grid_name)) {
      mask |= 1 << i;
    }
  }
  return mask;
}

bool Project::project_pair_exists(uint8_t pair, const char *basename) {
  return project_pair_file_mask(pair, basename) == ((1 << NUM_GRIDS) - 1);
}

bool Project::grid_pair_exists(const char *projectname, uint8_t pair) {
  const char *basename = nullptr;
  if (!split_project_path(projectname, &basename)) {
    return false;
  }
  chdir_projects();
  if (!SD.chdir(projectname)) {
    return false;
  }
  return project_pair_exists(pair, basename);
}

bool Project::read_active_grid_pair(const char *projectname, uint8_t *pair) {
  const char *basename = nullptr;
  if (pair == nullptr || !split_project_path(projectname, &basename)) {
    return false;
  }
  if (project_loaded && strcmp(projectname, mcl_cfg.project) == 0) {
    *pair = active_grid_pair;
    return true;
  }

  char proj_filename[PRJ_NAME_LEN + 5];
  if (!project_file_name(basename, proj_filename, sizeof(proj_filename))) {
    return false;
  }

  File header_file;
  chdir_projects();
  if (!SD.chdir(projectname) || !header_file.open(proj_filename, O_READ)) {
    header_file.close();
    return false;
  }

  uint32_t header_version = 0;
  bool ok = mcl_sd.read_data((uint8_t *)&header_version,
                             sizeof(header_version), &header_file);
  if (ok && header_version == PROJ_VERSION) {
    ok = mcl_sd.read_data(pair, sizeof(*pair), &header_file);
  } else {
    *pair = 0;
  }
  header_file.close();
  if (!ok || !PROJECT_VERSION_CAN_OPEN(header_version)) {
    return false;
  }
  return true;
}

bool Project::load_project(const char *projectname) {
  return load_project_impl(projectname, 0, false);
}

#ifdef MCL_HAS_PROJECT_BACKUP
bool Project::load_project_version(const char *projectname, uint8_t pair) {
  const bool restore_current = project_loaded;
  const uint8_t previous_pair = active_grid_pair;
  char previous_project[PRJ_PATH_LEN];
  memcpy(previous_project, mcl_cfg.project, sizeof(previous_project));
  previous_project[sizeof(previous_project) - 1] = '\0';
  if (load_project_impl(projectname, pair, true)) {
    return true;
  }

  if (restore_current) {
    load_project_impl(previous_project, previous_pair, true);
  }
  return false;
}
#endif

bool Project::load_project_impl(const char *projectname, uint8_t requested_pair,
                                bool use_requested_pair) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Loading project"));
  DEBUG_PRINTLN(projectname);
  file.close();
  project_loaded = false;

  const char *project_basename = nullptr;
  if (!split_project_path(projectname, &project_basename)) {
    return false;
  }

  char proj_filename[PRJ_NAME_LEN  + 5];
  if (!project_file_name(project_basename, proj_filename,
                         sizeof(proj_filename))) {
    DEBUG_PRINTLN("bad project filename");
    return false;
  }

  // Open project parent
  chdir_projects();

  if (!SD.chdir(projectname)) {
    DEBUG_PRINTLN("could not enter project dir");
    return false;
  }

  ret = file.open(proj_filename, O_RDWR);
  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not open project file"));
    return false;
  }
  ret = check_project_version();

  if (!ret) {
    DEBUG_PRINTLN(F("Project version incompatible"));
    file.close();
    return false;
  }

  uint32_t project_version = version;
#ifdef MCL_HAS_PROJECT_CONVERSION
  bool migrate_track_storage = project_version == PROJ_MIN_READABLE_VERSION;
  bool project_needs_update = migrate_track_storage;
#endif

  uint8_t pair = use_requested_pair ? requested_pair : active_grid_pair;

  if (pair >= 128 || !project_pair_exists(pair, project_basename)) {
    if (use_requested_pair) {
      DEBUG_PRINTLN(F("requested grid pair missing"));
      return false;
    }
    pair = 0;
    active_grid_pair = 0;
#ifdef MCL_HAS_PROJECT_CONVERSION
    project_needs_update = true;
#endif
    if (!project_pair_exists(pair, project_basename)) {
      DEBUG_PRINTLN(F("default grid pair missing"));
      return false;
    }
  }

#ifdef MCL_HAS_PROJECT_CONVERSION
  bool write_grid_headers = project_version == PROJ_MIN_READABLE_VERSION;
#endif
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_name[PRJ_NAME_LEN  + 5];
    grids[i].close_file();

    if (!build_grid_filename(project_basename, pair * NUM_GRIDS + i,
                             grid_name, sizeof(grid_name))) {
      DEBUG_PRINTLN(F("bad grid filename"));
      return false;
    }
    DEBUG_PRINTLN(F("opening grid"));
    DEBUG_PRINTLN(grid_name);
    if (!grids[i].open_file(grid_name)) {
      DEBUG_PRINTLN(F("could not open grid"));
      gfx.alert_error("OPEN GRID");
      return false;
    }

    uint32_t grid_version = project_version;
    if (grids[i].read_header()) {
      grid_version = grids[i].version;
    } else {
#ifdef MCL_HAS_PROJECT_CONVERSION
      if (project_version == PROJ_MIN_READABLE_VERSION) {
        DEBUG_PRINTLN(F("Legacy grid header missing"));
      } else
#endif
      {
        DEBUG_PRINTLN(F("Grid header missing"));
        return false;
      }
    }
    if (!PROJECT_VERSION_CAN_OPEN(grid_version)) {
      DEBUG_PRINTLN(F("Grid version incompatible"));
      return false;
    }
#ifdef MCL_HAS_PROJECT_CONVERSION
    if (grid_version == PROJ_MIN_READABLE_VERSION) {
      migrate_track_storage = true;
    }
    if (grid_version < PROJ_VERSION) {
      write_grid_headers = true;
    }
#endif
  }

#ifdef MCL_HAS_PROJECT_CONVERSION
  bool rebuilt_grid_pair = false;
  if (migrate_track_storage || project_needs_update || write_grid_headers) {
    draw_wait_popup("UPGRADING PROJECT");
  }

  if (migrate_track_storage) {
    if (!migrate_track_storage_versions(project_basename, &pair)) {
      DEBUG_PRINTLN(F("Could not migrate project tracks"));
      return false;
    }
    active_grid_pair = pair;
    rebuilt_grid_pair = true;
    write_grid_headers = false;
  }
  if (write_grid_headers) {
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      uint8_t grid_id = pair * NUM_GRIDS + i;
      if (!grids[i].write_header(PROJ_VERSION, grid_id) ||
          !grids[i].sync()) {
        DEBUG_PRINTLN(F("Could not write grid headers"));
        return false;
      }
    }
  }
#endif
  if (use_requested_pair
#ifdef MCL_HAS_PROJECT_CONVERSION
      && !rebuilt_grid_pair
#endif
      ) {
    active_grid_pair = requested_pair;
  }
  bool applied_project_config = false;
  bool update_header = use_requested_pair;
  uint8_t project_sample_bank = project_config_sample_bank_to_load(cfg);
  if (mcl_cfg.project_config) {
    apply_project_config(&mcl_cfg, cfg);
    ptc_groups.load(mcl_cfg.ptc_group);
    mclsys_normalize_midi_config();
    copy_project_config(&cfg, mcl_cfg);
    project_sample_bank = project_config_sample_bank_to_load(mcl_cfg);
    applied_project_config = true;
  } else {
    mcl_cfg.md_sample_bank = project_config_sample_bank_setting(cfg);
    mcl_cfg.md_sample_bank_capture = 0;
  }

  // Manual-step mode always follows the loaded project, regardless of the
  // PROJ CFG toggle above — it's sequencer playback behavior specific to
  // each project, not a general system setting like MIDI routing/tempo.
  bool manual_step_changed =
      mcl_cfg.manual_step_enabled != cfg.manual_step_enabled ||
      mcl_cfg.manual_step_cc != cfg.manual_step_cc ||
      mcl_cfg.manual_step_port != cfg.manual_step_port;
  if (project_config_valid(cfg)) {
    mcl_cfg.manual_step_enabled = cfg.manual_step_enabled;
    mcl_cfg.manual_step_cc = cfg.manual_step_cc;
    mcl_cfg.manual_step_port = cfg.manual_step_port;
  } else {
    mcl_cfg.manual_step_enabled = 0;
    mcl_cfg.manual_step_cc = 0;
    mcl_cfg.manual_step_port = MANUAL_STEP_PORT_MIDI2;
  }

#if MCL_FEATURE_HOST_ARRANGER
  uint8_t active_arrangement_idx = cfg.active_arrangement_idx;
  if (!ensure_arrangements_current_project(&active_arrangement_idx)) {
    DEBUG_PRINTLN(F("Could not initialise arrangements"));
    active_arrangement_idx = 0;
  }
  mcl_cfg.active_arrangement_idx = active_arrangement_idx;
  if (cfg.active_arrangement_idx != active_arrangement_idx) {
    cfg.active_arrangement_idx = active_arrangement_idx;
    update_header = true;
  }
#else
  mcl_cfg.active_arrangement_idx = cfg.active_arrangement_idx;
#endif

#ifdef MCL_HAS_PROJECT_CONVERSION
  update_header = update_header || project_needs_update || write_grid_headers ||
                  project_version < PROJ_VERSION;
  bool created_project_header = false;
  if (project_version < PROJ_VERSION) {
    char old_filename[PRJ_NAME_LEN + 5];
    strcpy(old_filename, proj_filename);
    uint8_t len = strlen(old_filename);
    old_filename[len - 3] = 'o';
    old_filename[len - 2] = 'l';
    old_filename[len - 1] = 'd';
    if (!SD.exists(old_filename)) {
#ifdef __AVR__
      if (!file.rename(old_filename)) {
        return false;
      }
      file.close();
#else
      file.close();
      if (!SD.rename(proj_filename, old_filename)) {
        return false;
      }
#endif
      if (!file.open(proj_filename, O_RDWR | O_CREAT)) {
        return false;
      }
      created_project_header = true;
    }
  }
#endif
  if (update_header && !write_header()) {
    return false;
  }
#ifdef MCL_HAS_PROJECT_CONVERSION
  if (created_project_header && !file.sync()) {
    return false;
  }
#endif

  strncpy(mcl_cfg.project, projectname, sizeof(mcl_cfg.project) - 1);
  mcl_cfg.project[sizeof(mcl_cfg.project) - 1] = '\0';
  if (!mcl_cfg.number_projects) { mcl_cfg.number_projects++; }

  ret = mcl_cfg.write_cfg();

  if (!ret) {
    DEBUG_PRINTLN(F("could not write cfg"));
    return false;
  }
  if (applied_project_config || manual_step_changed) {
    midi_setup.cfg_ports();
  }
  load_project_sample_bank(project_sample_bank);
  grid_page.row_scan = GRID_LENGTH;
  project_loaded = true;
#if MCL_FEATURE_HOST_ARRANGER
  mcl_arrangement.seekLoadCurrentPosition(true, true);
  sps_host_arr_bridge.notifyDirty(
      0xFF, (uint8_t)(spsarr::DIRTY_CELLS | spsarr::DIRTY_ACTIVE |
                      spsarr::DIRTY_ARRANGEMENT));
  sps_host_seq_bridge.notifyDirty(
      0xFF, (uint8_t)(spsseq::DIRTY_SUMMARY | spsseq::DIRTY_DETAIL |
                      spsseq::DIRTY_LOCKS | spsseq::DIRTY_META));
  sps_host_seq_bridge.notifyActive();
#endif
  return true;
}

bool Project::read_header() {
  if (!file.seekSet(0)) {
    DEBUG_PRINTLN(F("Seek failed"));
    return false;
  }

  uint32_t header_version = 0;
  if (!mcl_sd.read_data((uint8_t *)&header_version, sizeof(header_version),
                        &file)) {
    DEBUG_PRINTLN(F("Could not read project version"));
    return false;
  }

#ifndef MCL_HAS_PROJECT_CONVERSION
  if (header_version != PROJ_VERSION) {
    version = header_version;
    return true;
  }
#endif

#ifdef MCL_HAS_PROJECT_CONVERSION
  if (header_version == PROJ_MIN_READABLE_VERSION) {
    version = header_version;
    if (!read_project_header_body(this, file)) {
      DEBUG_PRINTLN(F("Could not read legacy project header"));
      return false;
    }

    active_grid_pair = 0;
    memset(reserved, 0, sizeof(reserved));
    if (project_config_valid(cfg)) {
      normalize_project_config(&cfg);
    } else {
      copy_project_config(&cfg, mcl_cfg);
      clear_project_sample_bank(&cfg);
      clear_project_manual_step(&cfg);
    }
    return true;
  }
#endif

  version = header_version;
  if (!read_project_header_body(this, file)) {
    DEBUG_PRINTLN(F("Could not read project header"));
    return false;
  }
  if (!project_config_valid(cfg)) {
    copy_project_config(&cfg, mcl_cfg);
    clear_project_sample_bank(&cfg);
    clear_project_manual_step(&cfg);
  }
  return true;
}

bool Project::check_project_version() {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Check project version"));

  ret = read_header();

  if (!ret) {
    DEBUG_PRINTLN(F("Could not read project header"));
    return false;
  }
  return PROJECT_VERSION_CAN_OPEN(version);
}

#ifdef MCL_HAS_PROJECT_CONVERSION
bool PROJECT_CONVERSION_ATTR() Project::migrate_legacy_md_aux_slots(
    GridRow row, GridRowHeader *grid_x_header, Grid *dst_grids,
    bool *skip_grid_x0) {
  GridRowHeader grid_y_header;
  if (!grids[1].read_row_header(&grid_y_header, row)) {
    return false;
  }
  if (grid_x_header->name[0] != '\0') {
    memcpy(grid_y_header.name, grid_x_header->name, sizeof(grid_y_header.name));
  }

  if (grid_y_header.track_type[MDLFO_TRACK_NUM] == MDLFO_TRACK_TYPE) {
    LegacyMDLFOTrackStorage legacy_track;
    if (!grids[1].read(&legacy_track, sizeof(legacy_track), MDLFO_TRACK_NUM,
                       row)) {
      return false;
    }

    if (legacy_track.header.active == MDLFO_TRACK_TYPE &&
        (grid_x_header->track_type[0] == MD_TRACK_TYPE ||
         grid_x_header->track_type[0] == EMPTY_TRACK_TYPE)) {
      MigratedMDTrackStorage upgraded_md_track;
      if (grid_x_header->track_type[0] == MD_TRACK_TYPE) {
        if (read_upgraded_md_track(grids[0], 0, row, upgraded_md_track) !=
            LEGACY_MIGRATE_OK) {
          return false;
        }
      } else if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
        LegacyGridTrackHeader legacy_header;
        if (!grids[0].read(&legacy_header, sizeof(legacy_header), 0, row)) {
          return false;
        }
        upgraded_md_track.header.version[0] =
            SEQ_TRACK_LOAD_FADE_STORAGE_VERSION;
        upgraded_md_track.header.version[1] = 0;
        upgraded_md_track.header.active = MD_TRACK_TYPE;
        upgraded_md_track.header.link = legacy_header.link;
        upgraded_md_track.machine.track = 0;
        upgraded_md_track.machine.init();
        upgraded_md_track.mod_data.init();
        upgraded_md_track.seq_data.init();
        upgraded_md_track.load_fade.init();
      }

      LFOSeqTrack::convert_legacy_data(legacy_track.lfo_data,
                                       &upgraded_md_track.mod_data.lfo);

      if (!dst_grids[0].write(&upgraded_md_track,
                              sizeof(upgraded_md_track), 0, row)) {
        return false;
      }
      if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
        grid_x_header->active = true;
        grid_x_header->update_model(0, upgraded_md_track.machine.get_model(),
                                    MD_TRACK_TYPE);
      }
      *skip_grid_x0 = true;
    }

    grid_y_header.update_model(MDLFO_TRACK_NUM, 0, EMPTY_TRACK_TYPE);
  }

  return dst_grids[1].write_row_header(&grid_y_header, row);
}

bool PROJECT_CONVERSION_ATTR()
Project::migrate_grid_track_storage_versions(GridIndex grid, Grid *dst_grids) {
  for (GridRow row = 0; row < GRID_LENGTH; row++) {
    draw_upgrade_progress(grid, row);

    GridRowHeader row_header;
    bool ok = grid == 0 ? grids[0].read_row_header(&row_header, row)
                        : dst_grids[1].read_row_header(&row_header, row);
    if (!ok) {
      return false;
    }
    bool skip_grid_x0 = false;
    if (grid == 0 &&
        !migrate_legacy_md_aux_slots(row, &row_header, dst_grids,
                                     &skip_grid_x0)) {
      return false;
    }
    bool skip_grid_y_perf = false;

    for (GridColumn column = 0; column < GRID_WIDTH; column++) {
      if (grid == 0 && column == 0 && skip_grid_x0) {
        continue;
      }

      uint8_t track_type = row_header.track_type[column];
      switch (track_type) {
      case EMPTY_TRACK_TYPE:
        if (!migrate_empty_track_header(grids[grid], dst_grids[grid],
                                        column, row)) {
          return false;
        }
        break;
      case MD_TRACK_TYPE:
        if (grid != 0) {
          if (!dst_grids[grid].clear_slot(column, row, false)) {
            return false;
          }
        } else if (!handle_legacy_migration_status(
                       migrate_md_track_native_swing(grids[grid],
                                                     dst_grids[grid], column,
                                                     row),
                       row_header, dst_grids[grid], column, row)) {
          return false;
        }
        break;
      case EXT_TRACK_TYPE:
      {
        ExtTrack upgraded;
        if (!handle_legacy_migration_status(
                migrate_ext_like_track_storage(grids[grid], dst_grids[grid],
                                               column, row, upgraded,
                                               upgraded.seq_data),
                row_header, dst_grids[grid], column, row)) {
          return false;
        }
        break;
      }
      case A4_TRACK_TYPE:
      {
        A4Track upgraded;
        if (!handle_legacy_migration_status(
                migrate_ext_like_track_storage(grids[grid], dst_grids[grid],
                                               column, row, upgraded,
                                               upgraded.seq_data),
                row_header, dst_grids[grid], column, row)) {
          return false;
        }
        break;
      }
      case MNM_TRACK_TYPE:
      {
        MNMTrack upgraded;
        if (!handle_legacy_migration_status(
                migrate_ext_like_track_storage(grids[grid], dst_grids[grid],
                                               column, row, upgraded,
                                               upgraded.seq_data),
                row_header, dst_grids[grid], column, row)) {
          return false;
        }
        break;
      }
      case MDFX_TRACK_TYPE:
      case MDTEMPO_TRACK_TYPE:
      case GRIDCHAIN_TRACK_TYPE:
        if (!migrate_fixed_payload_track(grids[grid], dst_grids[grid],
                                         column, row, track_type)) {
          return false;
        }
        break;
      case PERF_TRACK_TYPE:
        if (grid == 1) {
          if (column == LEGACY_PERF_TRACK_NUM) {
            if (!migrate_perf_track_storage(grids[grid], dst_grids[grid],
                                            column, row, PERF_TRACK_NUM)) {
              return false;
            }
            row_header.update_model(PERF_TRACK_NUM, PERF_TRACK_TYPE,
                                    PERF_TRACK_TYPE);
            row_header.update_model(LEGACY_PERF_TRACK_NUM, 0, EMPTY_TRACK_TYPE);
            skip_grid_y_perf = true;
            break;
          }
          if (column == PERF_TRACK_NUM) {
            if (!skip_grid_y_perf &&
                !dst_grids[grid].clear_slot(column, row, false)) {
              return false;
            }
            break;
          }
        }
        if (!migrate_perf_track_storage(grids[grid], dst_grids[grid],
                                        column, row, column)) {
          return false;
        }
        break;
      case MDROUTE_TRACK_TYPE:
        if (grid == 1 && column == MDROUTE_TRACK_NUM) {
          if (!migrate_md_route_track_storage(grids[grid], dst_grids[grid],
                                              row_header, row,
                                              cfg.uart2_poly_chan)) {
            return false;
          }
        } else if (!dst_grids[grid].clear_slot(column, row, false)) {
          return false;
        }
        break;
      default:
        if (!dst_grids[grid].clear_slot(column, row, false)) {
          return false;
        }
        break;
      }
    }
    if (!dst_grids[grid].write_row_header(&row_header, row)) {
      return false;
    }
  }
  return true;
}

bool PROJECT_CONVERSION_ATTR()
Project::migrate_track_storage_versions(const char *basename,
                                        uint8_t *active_pair) {
  uint8_t dest_pair = 1;
  for (; dest_pair < 128; dest_pair++) {
    if (project_pair_file_mask(dest_pair, basename) == 0) {
      break;
    }
  }
  if (dest_pair >= 128) {
    return false;
  }

  Grid dst_grids[NUM_GRIDS];
  bool ok = true;
  for (GridIndex grid = 0; ok && grid < NUM_GRIDS; grid++) {
    char grid_name[PRJ_NAME_LEN + 5];
    uint8_t grid_id = dest_pair * NUM_GRIDS + grid;
    build_grid_filename(basename, grid_id, grid_name, sizeof(grid_name));
    ok = dst_grids[grid].new_file(grid_name) &&
         dst_grids[grid].write_header(PROJ_VERSION, grid_id);
  }

  for (GridIndex grid = 0; ok && grid < NUM_GRIDS; grid++) {
    ok = migrate_grid_track_storage_versions(grid, dst_grids);
  }

  for (GridIndex grid = 0; ok && grid < NUM_GRIDS; grid++) {
    draw_upgrade_progress(grid, GRID_LENGTH);
    ok = dst_grids[grid].sync();
  }

  for (GridIndex grid = 0; grid < NUM_GRIDS; grid++) {
    dst_grids[grid].close_file();
  }

  if (ok) {
    for (GridIndex grid = 0; grid < NUM_GRIDS; grid++) {
      grids[grid].close_file();
    }
    for (GridIndex grid = 0; ok && grid < NUM_GRIDS; grid++) {
      char grid_name[PRJ_NAME_LEN + 5];
      uint8_t grid_id = dest_pair * NUM_GRIDS + grid;
      build_grid_filename(basename, grid_id, grid_name, sizeof(grid_name));
      ok = grids[grid].open_file(grid_name);
    }
  }

  if (!ok) {
    for (GridIndex grid = 0; grid < NUM_GRIDS; grid++) {
      grids[grid].close_file();
    }
    for (GridIndex grid = 0; grid < NUM_GRIDS; grid++) {
      char grid_name[PRJ_NAME_LEN + 5];
      uint8_t grid_id = dest_pair * NUM_GRIDS + grid;
      build_grid_filename(basename, grid_id, grid_name, sizeof(grid_name));
      SD.remove(grid_name);
    }
    return false;
  }

  *active_pair = dest_pair;
  return true;
}
#endif

bool Project::write_header() {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Writing project header"));

  version = PROJ_VERSION;
  //  Config mcl_cfg.
  //  uint8_t reserved[16];
  hash = 0;

  ret = file.seekSet(0);

  if (!ret) {

    DEBUG_PRINTLN(F("Seek failed"));

    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(ProjectHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN(F("Write header failed"));
    return false;
  }
  DEBUG_PRINTLN(F("Write header success"));
  return true;
}

bool Project::store_config_from_system() {
  if (!project_loaded) {
    return true;
  }
  copy_project_config(&cfg, mcl_cfg);
  return write_header();
}

bool Project::copy_grid_pair(const char *from_project,
                             const char *from_basename,
                             const char *to_project,
                             const char *to_basename,
                             uint8_t source_pair, uint8_t dest_pair) {
  chdir_projects();

  bool ok = true;
  for (uint8_t grid_idx = 0; ok && grid_idx < NUM_GRIDS; grid_idx++) {
    char src_name[PRJ_NAME_LEN + 5];
    char dst_name[PRJ_NAME_LEN + 5];
    char src_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6];
    char dst_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6];
    Grid src_grid;
    Grid dst_grid;

    ok = build_grid_filename(from_basename, source_pair * NUM_GRIDS + grid_idx,
                             src_name, sizeof(src_name)) &&
         build_grid_filename(to_basename, dest_pair * NUM_GRIDS + grid_idx,
                             dst_name, sizeof(dst_name)) &&
         MCLSd::join_path(src_path, sizeof(src_path), from_project,
                          src_name) &&
         MCLSd::join_path(dst_path, sizeof(dst_path), to_project, dst_name) &&
         !SD.exists(dst_path) && src_grid.open_file(src_path) &&
         dst_grid.new_file(dst_path);
    if (!ok) {
      src_grid.close_file();
      dst_grid.close_file();
      break;
    }

    uint32_t grid_version = PROJ_MIN_READABLE_VERSION;
    if (src_grid.read_header()) {
      grid_version = src_grid.version;
    }
    ok = PROJECT_VERSION_CAN_OPEN(grid_version) &&
         dst_grid.write_header(grid_version,
                               dest_pair * NUM_GRIDS + grid_idx);

    for (GridRow row = 0; ok && row < GRID_LENGTH; row++) {
      draw_upgrade_progress(grid_idx, row);

      GridRowHeader row_header;
      ok = src_grid.read_row_header(&row_header, row) &&
           dst_grid.write_row_header(&row_header, row);
      if (!ok) {
        break;
      }

      ok = copy_grid_row_slots_raw(src_grid, dst_grid, row);
    }

    if (ok) {
      draw_upgrade_progress(grid_idx, GRID_LENGTH);
      ok = dst_grid.sync();
    }

    src_grid.close_file();
    dst_grid.close_file();
  }

  if (!ok) {
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      char dst_name[PRJ_NAME_LEN + 5];
      char dst_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6];
      if (build_grid_filename(to_basename, dest_pair * NUM_GRIDS + i,
                              dst_name, sizeof(dst_name)) &&
          MCLSd::join_path(dst_path, sizeof(dst_path), to_project,
                           dst_name)) {
        SD.remove(dst_path);
      }
    }
  }
  return ok;
}

#ifdef MCL_HAS_PROJECT_BACKUP
bool Project::create_backup(const char *projectname, uint8_t *created_pair) {
  const char *basename = nullptr;
  if (!split_project_path(projectname, &basename)) {
    return false;
  }

  uint8_t source_pair = 0;
  if (project_loaded && strcmp(projectname, mcl_cfg.project) == 0) {
    if (!sync_grid() || !store_config_from_system()) {
      return false;
    }
    source_pair = active_grid_pair;
  } else if (!read_active_grid_pair(projectname, &source_pair)) {
    return false;
  }

  chdir_projects();
  if (!SD.chdir(projectname) || !project_pair_exists(source_pair, basename)) {
    return false;
  }

  uint8_t dest_pair = 1;
  for (; dest_pair < 128; dest_pair++) {
    uint8_t mask = project_pair_file_mask(dest_pair, basename);
    if (mask == 0xFF) {
      return false;
    }
    if (mask == 0) {
      break;
    }
  }
  if (dest_pair >= 128) {
    return false;
  }

  draw_wait_popup("CREATING BACKUP");
  if (!copy_grid_pair(projectname, basename, projectname, basename,
                      source_pair, dest_pair)) {
    return false;
  }
  if (created_pair != nullptr) {
    *created_pair = dest_pair;
  }
  return true;
}

bool Project::delete_backup(const char *projectname, uint8_t pair) {
  if (pair == 0) {
    return false;
  }

  const char *basename = nullptr;
  if (!split_project_path(projectname, &basename)) {
    return false;
  }

  uint8_t active_pair = 0;
  if (!read_active_grid_pair(projectname, &active_pair) || pair == active_pair) {
    return false;
  }

  chdir_projects();
  if (!SD.chdir(projectname) || !project_pair_exists(pair, basename)) {
    return false;
  }

  bool ok = true;
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_name[PRJ_NAME_LEN + 5];
    if (!build_grid_filename(basename, pair * NUM_GRIDS + i, grid_name,
                             sizeof(grid_name)) ||
        !SD.remove(grid_name)) {
      ok = false;
    }
  }
  return ok;
}
#endif

bool Project::rename_project_files(const char *from_basename,
                                   const char *to_basename) {
  char from_name[PRJ_NAME_LEN + 5];
  char to_name[PRJ_NAME_LEN + 5];

  for (uint16_t suffix = 0; suffix < PROJECT_RENAME_SUFFIXES; suffix++) {
    if (!build_grid_filename(from_basename, suffix, from_name,
                             sizeof(from_name)) ||
        !build_grid_filename(to_basename, suffix, to_name, sizeof(to_name))) {
      return false;
    }
    if (SD.exists(from_name) && !SD.rename(from_name, to_name)) {
      return false;
    }
  }

  if (!project_file_name(from_basename, from_name, sizeof(from_name)) ||
      !project_file_name(to_basename, to_name, sizeof(to_name))) {
    return false;
  }
  return !SD.exists(from_name) || SD.rename(from_name, to_name);
}

bool Project::copy_project(const char *from_project, const char *to_project) {
  const char *from_basename = nullptr;
  const char *to_basename = nullptr;
  if (!split_project_path(from_project, &from_basename) ||
      !split_project_path(to_project, &to_basename)) {
    return false;
  }

  if (project_loaded && strcmp(from_project, mcl_cfg.project) == 0) {
#if MCL_FEATURE_HOST_ARRANGER
    if (!mcl_arrangement.flushRuntimePrivateSourceEdits()) {
      return false;
    }
#endif
    if (!sync_grid() || !store_config_from_system()) {
      return false;
    }
  }

  chdir_projects();
  if (SD.exists(to_project) || !SD.mkdir(to_project, true)) {
    return false;
  }

  draw_wait_popup("CLONING PROJECT");

  char from_name[PRJ_NAME_LEN + 5];
  char to_name[PRJ_NAME_LEN + 5];
  char from_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6];
  char to_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6];
  bool ok = project_file_name(from_basename, from_name, sizeof(from_name)) &&
            project_file_name(to_basename, to_name, sizeof(to_name)) &&
            MCLSd::join_path(from_path, sizeof(from_path), from_project,
                             from_name) &&
            MCLSd::join_path(to_path, sizeof(to_path), to_project, to_name) &&
            mcl_sd.copy_file(from_path, to_path);

  bool copied_pair = false;
  for (uint8_t pair = 0; ok && pair < 128; pair++) {
    chdir_projects();
    bool exists = SD.chdir(from_project) &&
                  project_pair_exists(pair, from_basename);
    if (!exists) {
      continue;
    }
    ok = copy_grid_pair(from_project, from_basename, to_project, to_basename,
                        pair, pair);
    copied_pair = copied_pair || ok;
  }

#if MCL_FEATURE_HOST_ARRANGER
  if (ok) {
    chdir_projects();
    ok = copy_arrangement_files(from_project, to_project);
  }
#endif

  if (!copied_pair) {
    ok = false;
  }

  if (!ok) {
    chdir_projects();
    mcl_sd.remove_dir(to_project);
    return false;
  }
  chdir_projects();
  return true;
}

#ifdef MCL_HAS_FILE_MOVE
bool Project::move_project(const char *from_project, const char *to_project) {
  if (strcmp(from_project, to_project) == 0) {
    return false;
  }

  bool reload_current = project_loaded &&
                        strcmp(from_project, mcl_cfg.project) == 0;
  if (!copy_project(from_project, to_project)) {
    return false;
  }

  if (reload_current) {
    close_project();
    project_loaded = false;
  }

  chdir_projects();
  if (!mcl_sd.remove_dir(from_project)) {
    if (reload_current) {
      if (!load_project(to_project)) {
        load_project(from_project);
      }
    }
    return false;
  }

  if (reload_current && !load_project(to_project)) {
    return false;
  }
  return true;
}
#endif

bool Project::new_project_master_file(const char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Creating new project master file"));

  file.close();
  active_grid_pair = 0;
  copy_project_config(&cfg, mcl_cfg);

  DEBUG_PRINTLN(F("Attempting to extend project file"));
  DEBUG_PRINTLN(projectname);

  ret = file.open(projectname, O_RDWR | O_CREAT);
  if (!ret) {
    DEBUG_PRINTLN(F("Could not open file"));
    gfx.alert_error("OPEN ERR");
    return false;
  }

  ret = file.preAllocate(GRID_SLOT_BYTES);
  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not extend file"));
    gfx.alert_error("PREALLOC ERR");
    return false;
  }

  if (!write_header()) {
    gfx.alert_error("HEADER ERR");
    return false;
  }

  // m_strncpy(mcl_cfg.project, projectname, 16);
  file.close();

  mcl_cfg.number_projects++;
  mcl_cfg.write_cfg();

  DEBUG_PRINTLN(projectname);
  DEBUG_PRINTLN(F("project created"));
  // if (!ret) {
  // return false;
  // }

  return true;
}
Project proj;
