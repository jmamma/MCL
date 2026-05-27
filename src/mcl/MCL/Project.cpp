#include "MCLDefines.h"
#include "Project.h"
#include "MCLSd.h"
#include "MCLGUI.h"
#include "GridPages.h"
#include "MidiSetup.h"
#include "oled.h"

#include "MDTrack.h"
#include "ExtTrack.h"
#include "A4Track.h"
#include "MNMTrack.h"
#include "MDFXTrack.h"
#include "MDRouteTrack.h"
#include "MDTempoTrack.h"
#include "EmptyTrack.h"
#include "PerfTrack.h"
#include "GridChainTrack.h"
#include "LFOSeqTrack.h"
#if !defined(__AVR__)
#include "SPSXTrack.h"
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

constexpr size_t LEGACY_GRID_TRACK_HEADER_SIZE =
    sizeof(uint8_t) * 3 + sizeof(GridLink);

class ATTR_PACKED() LegacyGridTrackHeader {
public:
  uint8_t version[2];
  uint8_t active;
  GridLink link;
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
static_assert(sizeof(GridTrack) - sizeof(void *) == LEGACY_GRID_TRACK_HEADER_SIZE,
              "current track header prefix changed");
static_assert(sizeof(LegacyPerfTrackData) + 2 == sizeof(PerfTrackData),
              "legacy PerfTrack payload is not a prefix");
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
constexpr uint8_t MIGRATE_TRACK_STORAGE = 1 << 0;
constexpr uint8_t MIGRATE_SIGNED_MICROTIMING = 1 << 1;
constexpr uint8_t MIGRATE_PERF_TRACK_LAYOUT = 1 << 2;
constexpr uint8_t MIGRATE_GRID_PAIRS = 1 << 3;
constexpr uint8_t MIGRATE_PROJECT_CONFIG = 1 << 4;

#ifdef MCL_HAS_PROJECT_CONVERSION
#define PROJECT_VERSION_CAN_OPEN(v)                                           \
  ((v) == PROJ_MIN_READABLE_VERSION ||                                        \
   ((v) >= PROJ_VERSION_PERF_TRACK_LAYOUT && (v) <= PROJ_VERSION))
#else
#define PROJECT_VERSION_CAN_OPEN(v) ((v) == PROJ_VERSION)
#endif

bool project_config_valid(const MCLSysConfigData &source) {
  return source.version == CONFIG_VERSION;
}

#ifdef MCL_HAS_PROJECT_CONVERSION
void copy_legacy_header(GridTrack &dst, const LegacyGridTrackHeader &src) {
  dst.active = src.active;
  dst.link = src.link;
}

bool read_legacy_header(Grid &grid, GridColumn column, GridRow row,
                        uint8_t expected_type,
                        LegacyGridTrackHeader *header) {
  if (header == nullptr || !grid.seek(column, row) ||
      !grid.read(header, sizeof(*header))) {
    return false;
  }
  return header->active == expected_type;
}

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
  memcpy(&dst, &src, sizeof(src));
  dst.load_mute_set = 255;
  dst.load_type_mask = 0;

  uint8_t bit = 1;
  uint8_t load_bit = 0x10;
  for (uint8_t n = 0; n < 4; n++, bit <<= 1, load_bit <<= 1) {
    uint16_t mutes = dst.mute_sets[1].mutes[n];
    if ((mutes & 0x8000) == 0) {
      dst.load_mute_set = n;
    }
    if (mutes & 0x2000) {
      dst.load_type_mask |= bit;
    }
    if (mutes & 0x4000) {
      dst.load_type_mask |= load_bit;
    }
    dst.mute_sets[1].mutes[n] = mutes | 0xE000;
  }
}

uint8_t project_seq_speed_value(const GridLink &link) {
  return link.speed_value() & 0x7F;
}

void convert_md_seq_unsigned_timing(MDSeqTrackData &data, uint8_t speed) {
  uint16_t ticks_per_step = SeqTrack::get_ticks_per_step(speed);
  uint8_t *legacy_timing = reinterpret_cast<uint8_t *>(data.microtiming);
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    uint8_t timing = legacy_timing[step];
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
  memcpy(dst.steps, src.steps, sizeof(dst.steps));

  uint16_t src_lock = 0;
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    uint8_t legacy_locks = src.steps[step].locks;
    bool locks_enabled = src.steps[step].locks_enabled;
    dst.steps[step].locks = 0;
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
  // Every MDSeqTrackData field is populated here; legacy lock-enable shares the
  // current swing bit and is overwritten after copying the step descriptors.
  memcpy(dst.microtiming, src.timing, sizeof(dst.microtiming));
  convert_md_seq_unsigned_timing(dst, speed);

  copy_legacy_md_locks(dst, src);
  // Keep stale legacy lock params. Legacy playback used nonzero params with
  // no step lock bit to send kit-value resets on trig steps.
  dst.swing_amount = 0;
  dst.set_swing_from_mask(MDSEQ_DEFAULT_SWING_MASK);
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
  convert_ext_seq_unsigned_timing(dst, speed);
}

bool read_legacy_ext_track(Grid &grid, GridColumn column, GridRow row,
                           DeviceTrack &upgraded, ExtSeqTrackData &seq_data) {
  uint8_t track_type = upgraded.active;
  if (!grid.read(upgraded._this(),
                 LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(LegacyExtSeqTrackData),
                 column, row) ||
      upgraded.active != track_type) {
    return false;
  }
  upgraded.active = track_type;
  convert_legacy_ext_seq(seq_data, project_seq_speed_value(upgraded.link));
  return true;
}

bool write_migrated_track(Grid &grid, GridColumn column, GridRow row,
                          DeviceTrack &track, uint16_t write_size) {
  track.version = track.storage_version();
  track.reserved = 0;
  return grid.write(track._this(), write_size, column, row);
}

void init_upgraded_md_track(MigratedMDTrackStorage &dst, const GridLink &link,
                            const LegacyMDSeqTrackData &seq,
                            const MDMachine &machine) {
  dst.header.version[0] = SEQ_TRACK_MICROTIMING_STORAGE_VERSION;
  dst.header.version[1] = 0;
  dst.header.active = MD_TRACK_TYPE;
  dst.header.link = link;
  dst.mod_data.init();
  dst.machine = machine;
  copy_legacy_md_seq(dst.seq_data, seq, project_seq_speed_value(link));
}

bool read_upgraded_md_track(Grid &grid, GridColumn column, GridRow row,
                            MigratedMDTrackStorage &dst) {
  LegacyMDTrackStorage legacy_track;
  if (!grid.read(&legacy_track, sizeof(legacy_track), column, row) ||
      legacy_track.header.active != MD_TRACK_TYPE) {
    return false;
  }
  init_upgraded_md_track(dst, legacy_track.header.link, legacy_track.seq,
                         legacy_track.machine);
  return true;
}

bool migrate_md_track_native_swing(Grid &grid, GridColumn column, GridRow row) {
  MigratedMDTrackStorage upgraded;
  if (!read_upgraded_md_track(grid, column, row, upgraded)) {
    return false;
  }
  return grid.write(&upgraded, sizeof(upgraded), column, row);
}

bool migrate_ext_like_track_storage(Grid &grid, GridColumn column, GridRow row,
                                    DeviceTrack &upgraded,
                                    ExtSeqTrackData &seq_data) {
  upgraded.init_defaults();
  if (!read_legacy_ext_track(grid, column, row, upgraded, seq_data)) {
    return false;
  }
  uint16_t payload_size = upgraded.get_sound_data_size();
  if (payload_size != 0 &&
      !grid.read(upgraded.get_sound_data_ptr(), payload_size)) {
    return false;
  }
  return write_migrated_track(grid, column, row, upgraded,
                              upgraded.get_track_size());
}

bool migrate_fixed_payload_track(Grid &grid, GridColumn column, GridRow row,
                                 uint8_t track_type, uint8_t payload_size) {
  static_assert(sizeof(MDFXData) >= sizeof(TempoData),
                "fixed payload scratch too small for tempo");
  static_assert(sizeof(MDFXData) >= sizeof(GridChain),
                "fixed payload scratch too small for grid chain");
  LegacyFixedPayloadTrackStorage storage;
  uint8_t storage_size = sizeof(storage.header) + payload_size;
  if (!grid.read(&storage, storage_size, column, row) ||
      storage.header.active != track_type) {
    return false;
  }
  init_migrated_header(storage.header, storage.header, track_type, 0);
  return grid.write(&storage, storage_size, column, row);
}

bool migrate_perf_track_storage(Grid &grid, GridColumn column, GridRow row,
                                GridColumn dst_column) {
  LegacyPerfTrackStorage legacy_track;
  if (!grid.read(&legacy_track, sizeof(legacy_track), column, row) ||
      legacy_track.header.active != PERF_TRACK_TYPE) {
    return false;
  }

  MigratedPerfTrackStorage upgraded;
  init_migrated_header(upgraded.header, legacy_track.header, PERF_TRACK_TYPE,
                       PERF_TRACK_STORAGE_VERSION_CLEAN_LAYOUT);
  copy_legacy_perf_track_data(upgraded.data, legacy_track.data);
  return grid.write(&upgraded, sizeof(upgraded), dst_column, row);
}

bool migrate_perf_track_clean_layout(Grid &grid, GridColumn column,
                                     GridRow row) {
  LegacyGridTrackHeader header;
  if (!read_legacy_header(grid, column, row, PERF_TRACK_TYPE, &header)) {
    return false;
  }
  if (header.version[0] >= PERF_TRACK_STORAGE_VERSION_CLEAN_LAYOUT) {
    return true;
  }

  LegacyPerfTrackData legacy_perf;
  if (!grid.read(&legacy_perf, sizeof(legacy_perf))) {
    return false;
  }

  PerfTrack upgraded;
  copy_legacy_header(upgraded, header);
  copy_legacy_perf_track_data(upgraded, legacy_perf);
  return write_migrated_track(grid, column, row, upgraded,
                              upgraded.get_track_size());
}

#if !defined(__AVR__)
bool migrate_spsx_track_signed_microtiming(Grid &grid, GridColumn column,
                                           GridRow row) {
  SPSXTrack track;
  if (!grid.read(track._this(), track.get_track_size(), column, row)) {
    return false;
  }
  if (track.storage_version_at_least(SEQ_TRACK_MICROTIMING_STORAGE_VERSION)) {
    return true;
  }
  if (track.seq_storage.seq_version == SPSX_SEQ_VERSION_LEGACY) {
    convert_md_seq_unsigned_timing(track.seq_storage.seq_data.legacy,
                                   project_seq_speed_value(track.link));
  }
  return write_migrated_track(grid, column, row, track,
                              track.get_track_size());
}
#endif

bool migrate_md_track_signed_microtiming(Grid &grid, GridColumn column,
                                         GridRow row) {
  MDTrack track;
  if (!grid.read(track._this(), track.get_track_size(), column, row)) {
    return false;
  }
  if (track.storage_version_at_least(SEQ_TRACK_MICROTIMING_STORAGE_VERSION)) {
    return true;
  }
  convert_md_seq_unsigned_timing(track.seq_data,
                                 project_seq_speed_value(track.link));
  return write_migrated_track(grid, column, row, track, track.get_track_size());
}

bool migrate_ext_like_signed_microtiming(Grid &grid, GridColumn column,
                                         GridRow row, DeviceTrack &track,
                                         ExtSeqTrackData &seq_data) {
  if (!grid.read(track._this(), track.get_track_size(), column, row)) {
    return false;
  }
  if (track.storage_version_at_least(SEQ_TRACK_MICROTIMING_STORAGE_VERSION)) {
    return true;
  }
  convert_ext_seq_unsigned_timing(seq_data, project_seq_speed_value(track.link));
  return write_migrated_track(grid, column, row, track,
                              track.get_track_size());
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
}

#ifdef MCL_HAS_PROJECT_CONVERSION
void normalize_project_config(MCLSysConfigData *data) {
  data->version = CONFIG_VERSION;
  data->project[0] = '\0';
  data->number_projects = 0;
  data->project_config = 0;
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
    bool existing_project = SD.chdir(newprj) && SD.exists(proj_filename);
    chdir_projects();
    gfx.alert_error(existing_project ? "PROJECT EXISTS" : "DIR EXISTS");
    return false;
  }
  if (!SD.mkdir(newprj, true) || !SD.chdir(newprj)) {
    gfx.alert_error("DIR");
    return false;
  }


  draw_wait_popup("CREATING PROJECT");

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
      if (!grids[i].new_grid(grid_filename, PROJ_VERSION, i)) {
        gfx.alert_error("SD ERROR");
        return false;
      }
    }
  }
  // Initialiase Project Master File.
  //
  bool ret = proj.new_project_master_file(proj_filename);
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
  if (mcl_gui.wait_for_input(newprj, "New Project:", PRJ_NAME_LEN)) {
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

#define OLD_PROJ_VERSION 2025

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
  if (ok && header_version >= PROJ_VERSION_GRID_PAIRS) {
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
  return load_project_impl(projectname, pair, true);
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
  uint8_t migration_flags = 0;
  if (project_version == PROJ_MIN_READABLE_VERSION) {
    migration_flags = MIGRATE_TRACK_STORAGE | MIGRATE_GRID_PAIRS |
                      MIGRATE_PROJECT_CONFIG;
  }

  if (project_version < PROJ_VERSION_GRID_HEADERS &&
      !stamp_existing_grid_headers(project_basename, project_version)) {
    DEBUG_PRINTLN(F("Could not stamp grid headers"));
    return false;
  }
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
    migration_flags |= MIGRATE_GRID_PAIRS;
#endif
    if (!project_pair_exists(pair, project_basename)) {
      DEBUG_PRINTLN(F("default grid pair missing"));
      return false;
    }
  }

#ifdef MCL_HAS_PROJECT_CONVERSION
  bool write_grid_headers = project_version < PROJ_VERSION_GRID_HEADERS;
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
      write_grid_headers = true;
      if (project_version >= PROJ_VERSION_GRID_HEADERS) {
        // Older backup pairs can be headerless even after the master project
        // file has been upgraded. Treat the selected grid pair as supported
        // legacy storage so loading that backup upgrades it in place.
        grid_version = PROJ_MIN_READABLE_VERSION;
      }
#else
      DEBUG_PRINTLN(F("Grid header missing"));
      return false;
#endif
    }
    if (!PROJECT_VERSION_CAN_OPEN(grid_version)) {
      DEBUG_PRINTLN(F("Grid version incompatible"));
      return false;
    }
#ifdef MCL_HAS_PROJECT_CONVERSION
    if (grid_version == PROJ_MIN_READABLE_VERSION) {
      migration_flags |= MIGRATE_TRACK_STORAGE;
    }
    if (grid_version < PROJ_VERSION) {
      write_grid_headers = true;
    }
#endif
  }

#ifdef MCL_HAS_PROJECT_CONVERSION
  if (migration_flags) {
    draw_wait_popup("UPGRADING PROJECT");
  }

  if ((migration_flags & MIGRATE_TRACK_STORAGE) &&
      !migrate_track_storage_versions()) {
    DEBUG_PRINTLN(F("Could not migrate project tracks"));
    return false;
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
  if (use_requested_pair) {
    active_grid_pair = requested_pair;
  }
  bool applied_project_config = false;
  if (mcl_cfg.project_config) {
    apply_project_config(&mcl_cfg, cfg);
    ptc_groups.load(mcl_cfg.ptc_group);
    mclsys_normalize_midi_config();
    copy_project_config(&cfg, mcl_cfg);
    applied_project_config = true;
  }
  bool update_header = use_requested_pair;
#ifdef MCL_HAS_PROJECT_CONVERSION
  update_header = update_header || migration_flags || write_grid_headers ||
                  project_version < PROJ_VERSION;
#endif
  if (update_header && !write_header()) {
    return false;
  }

  strncpy(mcl_cfg.project, projectname, sizeof(mcl_cfg.project) - 1);
  mcl_cfg.project[sizeof(mcl_cfg.project) - 1] = '\0';
  if (!mcl_cfg.number_projects) { mcl_cfg.number_projects++; }

  ret = mcl_cfg.write_cfg();

  if (!ret) {
    DEBUG_PRINTLN(F("could not write cfg"));
    return false;
  }
  if (applied_project_config) {
    midi_setup.cfg_ports();
  }
  grid_page.row_scan = GRID_LENGTH;
  project_loaded = true;
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
    if (!mcl_sd.read_data((uint8_t *)(ProjectHeader *)this +
                              sizeof(header_version),
                          sizeof(ProjectHeader) - sizeof(header_version),
                          &file)) {
      DEBUG_PRINTLN(F("Could not read legacy project header"));
      return false;
    }

    active_grid_pair = 0;
    memset(reserved, 0, sizeof(reserved));
    if (project_config_valid(cfg)) {
      normalize_project_config(&cfg);
    } else {
      copy_project_config(&cfg, mcl_cfg);
    }
    return true;
  }
#endif

  version = header_version;
  if (!mcl_sd.read_data((uint8_t *)(ProjectHeader *)this +
                            sizeof(header_version),
                        sizeof(ProjectHeader) - sizeof(header_version),
                        &file)) {
    DEBUG_PRINTLN(F("Could not read project header"));
    return false;
  }
  if (!project_config_valid(cfg)) {
    copy_project_config(&cfg, mcl_cfg);
  }
  return true;
}

bool Project::check_project_version(uint16_t min_version) {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Check project version"));

  ret = read_header();

  if (!ret) {
    DEBUG_PRINTLN(F("Could not read project header"));
    return false;
  }
  (void)min_version;
  return PROJECT_VERSION_CAN_OPEN(version);
}

#ifdef MCL_HAS_PROJECT_CONVERSION
bool NOINLINE() Project::migrate_legacy_md_aux_slots(
    GridRow row, GridRowHeader *grid_x_header) {
  if (grid_x_header == nullptr) {
    return false;
  }

  GridRowHeader grid_y_header;
  if (!grids[1].read_row_header(&grid_y_header, row)) {
    return false;
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
          if (!read_upgraded_md_track(grids[0], 0, row, upgraded_md_track)) {
            return false;
          }
        } else if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
          upgraded_md_track.header.version[0] =
              SEQ_TRACK_MICROTIMING_STORAGE_VERSION;
          upgraded_md_track.header.version[1] = 0;
          upgraded_md_track.header.active = MD_TRACK_TYPE;
          upgraded_md_track.header.link.init(row);
          upgraded_md_track.machine.track = 0;
          upgraded_md_track.machine.init();
          upgraded_md_track.mod_data.init();
          upgraded_md_track.seq_data.init();
        }

        LFOSeqTrack::convert_legacy_data(legacy_track.lfo_data,
                                         &upgraded_md_track.mod_data.lfo);

        if (!grids[0].write(&upgraded_md_track, sizeof(upgraded_md_track), 0,
                            row)) {
          return false;
        }
        if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
          grid_x_header->active = true;
          grid_x_header->update_model(0, upgraded_md_track.machine.get_model(),
                                      MD_TRACK_TYPE);
          if (!grids[0].write_row_header(grid_x_header, row)) {
            return false;
          }
        }
        grid_x_header->track_type[0] = EMPTY_TRACK_TYPE;
      }

      grid_y_header.update_model(MDLFO_TRACK_NUM, 0, EMPTY_TRACK_TYPE);
      if (!grids[1].clear_slot(MDLFO_TRACK_NUM, row, false)) {
        return false;
      }
  }

  if (grid_y_header.track_type[LEGACY_PERF_TRACK_NUM] == PERF_TRACK_TYPE) {
    if (!migrate_perf_track_storage(grids[1], LEGACY_PERF_TRACK_NUM, row,
                                    PERF_TRACK_NUM)) {
      return false;
    }
    grid_y_header.update_model(PERF_TRACK_NUM, PERF_TRACK_TYPE,
                               PERF_TRACK_TYPE);
    grid_y_header.update_model(LEGACY_PERF_TRACK_NUM, 0, EMPTY_TRACK_TYPE);
    if (!grids[1].clear_slot(LEGACY_PERF_TRACK_NUM, row, false)) {
      return false;
    }
  }

  if (grid_y_header.track_type[MDROUTE_TRACK_NUM] == MDROUTE_TRACK_TYPE) {
    LegacyMDRouteTrackStorage legacy_route;
    if (!grids[1].read(&legacy_route, sizeof(legacy_route), MDROUTE_TRACK_NUM,
                       row)) {
      return false;
    }

    if (legacy_route.header.active == MDROUTE_TRACK_TYPE) {
      MigratedMDRouteTrackStorage new_route;
      init_migrated_header(new_route.header, legacy_route.header,
                           MD_ROUTE_TRACK_TYPE, 0);
      memcpy(new_route.route.routing, legacy_route.route.routing,
             sizeof(new_route.route.routing));
      uint8_t ptc_group =
          cfg.uart2_poly_chan >= PTC_MIDI_GROUP_MIN &&
                  cfg.uart2_poly_chan <= PTC_MIDI_GROUP_MAX
              ? cfg.uart2_poly_chan
              : PTC_GROUP_LOCAL;
      uint16_t poly_mask = legacy_route.route.poly_mask;
      for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i, poly_mask >>= 1) {
        new_route.route.ptc_group[i] =
            (poly_mask & 1) ? ptc_group : PTC_GROUP_OFF;
      }

      if (!grids[1].write(&new_route, sizeof(new_route), MDROUTE_TRACK_NUM,
                          row)) {
        return false;
      }
      grid_y_header.update_model(MDROUTE_TRACK_NUM, MD_ROUTE_TRACK_TYPE,
                                 MD_ROUTE_TRACK_TYPE);
    }
  }

  if (!grids[1].write_row_header(&grid_y_header, row)) {
    return false;
  }
  return true;
}

bool NOINLINE() Project::migrate_track_storage_versions() {
  for (GridIndex grid = 0; grid < NUM_GRIDS; grid++) {
    if (!migrate_grid_track_storage_versions(grid)) {
      return false;
    }
  }
  return true;
}

bool NOINLINE() Project::migrate_post_storage_tracks(uint8_t migration_flags) {
  for (GridIndex grid = 0; grid < NUM_GRIDS; grid++) {
    if (!migrate_grid_post_storage_tracks(grid, migration_flags)) {
      return false;
    }
  }
  return true;
}

bool NOINLINE() Project::migrate_grid_track_storage_versions(GridIndex grid) {
  for (GridRow row = 0; row < GRID_LENGTH; row++) {
    draw_upgrade_progress(grid, row);

    GridRowHeader row_header;
    if (!grids[grid].read_row_header(&row_header, row)) {
      return false;
    }

    if (grid == 0 && !migrate_legacy_md_aux_slots(row, &row_header)) {
      return false;
    }

    for (GridColumn column = 0; column < GRID_WIDTH; column++) {
      uint8_t track_type = row_header.track_type[column];
      if ((track_type >= MD_TRACK_TYPE && track_type <= EXT_TRACK_TYPE) ||
          track_type == MNM_TRACK_TYPE) {
        LegacyGridTrackHeader header;
        header.active = track_type;
        if (!read_legacy_header(grids[grid], column, row, track_type,
                                &header)) {
          if (header.active != track_type &&
              grids[grid].clear_slot(column, row, true)) {
            continue;
          }
          return false;
        }
      }

      switch (track_type) {
      case MD_TRACK_TYPE:
        if (grid == 0 &&
            !migrate_md_track_native_swing(grids[grid], column, row)) {
          return false;
        }
        break;
      case EXT_TRACK_TYPE:
      {
        ExtTrack upgraded;
        if (!migrate_ext_like_track_storage(grids[grid], column, row, upgraded,
                                            upgraded.seq_data)) {
          return false;
        }
        break;
      }
      case A4_TRACK_TYPE:
      {
        A4Track upgraded;
        if (!migrate_ext_like_track_storage(grids[grid], column, row, upgraded,
                                            upgraded.seq_data)) {
          return false;
        }
        break;
      }
      case MNM_TRACK_TYPE:
      {
        MNMTrack upgraded;
        if (!migrate_ext_like_track_storage(grids[grid], column, row, upgraded,
                                            upgraded.seq_data)) {
          return false;
        }
        break;
      }
      case MDFX_TRACK_TYPE:
        if (!migrate_fixed_payload_track(grids[grid], column, row,
                                         MDFX_TRACK_TYPE, sizeof(MDFXData))) {
          return false;
        }
        break;
      case MDTEMPO_TRACK_TYPE:
        if (!migrate_fixed_payload_track(grids[grid], column, row,
                                         MDTEMPO_TRACK_TYPE,
                                         sizeof(TempoData))) {
          return false;
        }
        break;
      case PERF_TRACK_TYPE:
        if (grid == 1 && column == PERF_TRACK_NUM) {
          break;
        }
        if (!migrate_perf_track_storage(grids[grid], column, row, column)) {
          return false;
        }
        break;
      case GRIDCHAIN_TRACK_TYPE:
        if (!migrate_fixed_payload_track(grids[grid], column, row,
                                         GRIDCHAIN_TRACK_TYPE,
                                         sizeof(GridChain))) {
          return false;
        }
        break;
      case MD_ROUTE_TRACK_TYPE:
        break;
      default:
        break;
      }
    }
  }

  draw_upgrade_progress(grid, GRID_LENGTH);
  return grids[grid].sync();
}

bool NOINLINE() Project::migrate_grid_post_storage_tracks(
    GridIndex grid, uint8_t migration_flags) {
  bool migrate_perf_layout = migration_flags & MIGRATE_PERF_TRACK_LAYOUT;
  bool migrate_signed_timing = migration_flags & MIGRATE_SIGNED_MICROTIMING;
  for (GridRow row = 0; row < GRID_LENGTH; row++) {
    draw_upgrade_progress(grid, row);

    GridRowHeader row_header;
    if (!grids[grid].read_row_header(&row_header, row)) {
      return false;
    }

    for (GridColumn column = 0; column < GRID_WIDTH; column++) {
      uint8_t track_type = row_header.track_type[column];
      if (migrate_perf_layout && track_type == PERF_TRACK_TYPE &&
          !migrate_perf_track_clean_layout(grids[grid], column, row)) {
        return false;
      }
      if (!migrate_signed_timing) {
        continue;
      }
      switch (track_type) {
      case MD_TRACK_TYPE:
        if (grid == 0 &&
            !migrate_md_track_signed_microtiming(grids[grid], column, row)) {
          return false;
        }
        break;
#if !defined(__AVR__)
      case MDSPSX_TRACK_TYPE:
        if (grid == 0 &&
            !migrate_spsx_track_signed_microtiming(grids[grid], column, row)) {
          return false;
        }
        break;
#endif
      case EXT_TRACK_TYPE:
      {
        ExtTrack track;
        if (!migrate_ext_like_signed_microtiming(grids[grid], column, row,
                                                track, track.seq_data)) {
          return false;
        }
        break;
      }
      case A4_TRACK_TYPE:
      {
        A4Track track;
        if (!migrate_ext_like_signed_microtiming(grids[grid], column, row,
                                                track, track.seq_data)) {
          return false;
        }
        break;
      }
      case MNM_TRACK_TYPE:
      {
        MNMTrack track;
        if (!migrate_ext_like_signed_microtiming(grids[grid], column, row,
                                                track, track.seq_data)) {
          return false;
        }
        break;
      }
      default:
        break;
      }
    }
  }

  draw_upgrade_progress(grid, GRID_LENGTH);
  return grids[grid].sync();
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
  if (!project_loaded || !mcl_cfg.project_config) {
    return true;
  }
  copy_project_config(&cfg, mcl_cfg);
  return write_header();
}

#ifdef MCL_HAS_PROJECT_CONVERSION
bool Project::stamp_existing_grid_headers(const char *basename,
                                          uint32_t grid_version) {
  for (uint8_t pair = 0; pair < 128; pair++) {
    uint8_t mask = project_pair_file_mask(pair, basename);
    if (mask == 0xFF) {
      return false;
    }
    if (mask != ((1 << NUM_GRIDS) - 1)) {
      continue;
    }

    for (uint8_t grid_idx = 0; grid_idx < NUM_GRIDS; grid_idx++) {
      char grid_name[PRJ_NAME_LEN + 5];
      Grid grid;
      uint8_t grid_id = pair * NUM_GRIDS + grid_idx;
      if (!build_grid_filename(basename, grid_id, grid_name,
                               sizeof(grid_name)) ||
          !grid.open_file(grid_name)) {
        grid.close_file();
        return false;
      }

      bool ok = true;
      if (!grid.read_header()) {
        ok = grid.write_header(grid_version, grid_id);
      }
      grid.close_file();
      if (!ok) {
        return false;
      }
    }
  }
  return true;
}
#endif

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
    return false;
  }

  ret = file.preAllocate(GRID_SLOT_BYTES);
  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not extend file"));
    return false;
  }

  if (!write_header()) {
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
