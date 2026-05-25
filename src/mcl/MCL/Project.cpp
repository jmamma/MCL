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

class ATTR_PACKED() LegacyPerfTrackData {
public:
  PerfTrackEncoderData encs[4];
  PerfScene scenes[NUM_SCENES];
  MuteSet mute_sets[2];
  uint8_t perf_locks[4][4];
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
static_assert(sizeof(LegacyPerfTrackData) + 2 == sizeof(PerfTrackData),
              "legacy PerfTrack payload is not a prefix");
static_assert(sizeof(LegacyGridTrackHeader) + sizeof(LegacyPerfTrackData) ==
                  491,
              "origin/dev PerfTrack storage size changed");
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
static_assert(LEGACY_GRID_TRACK_HEADER_SIZE + sizeof(LegacyMDSeqTrackData) +
                      sizeof(MDMachine) ==
                  534,
              "origin/dev MDTrack storage size changed");
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

bool project_config_valid(const MCLSysConfigData &source) {
  return source.version == CONFIG_VERSION;
}

void copy_legacy_header(GridTrack &dst, const LegacyGridTrackHeader &src) {
  dst.version = 0;
  dst.reserved = 0;
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

void copy_legacy_perf_track_data(PerfTrack &dst,
                                 const LegacyPerfTrackData &src) {
  memcpy(static_cast<PerfTrackData *>(&dst), &src, sizeof(src));
  dst.convert_legacy_load_settings();
}

uint8_t project_seq_speed_value(const GridLink &link) {
  return link.speed_value() & 0x7F;
}

void convert_md_seq_unsigned_timing(MDSeqTrackData &data, uint8_t speed) {
  uint16_t timing_mid = SeqTrack::get_timing_mid(speed);
  uint8_t *legacy_timing = reinterpret_cast<uint8_t *>(data.microtiming);
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    uint8_t timing = legacy_timing[step];
    if (timing == 0) {
      timing = timing_mid;
    }
    data.microtiming[step] =
        SeqTrack::timing_to_microtiming(timing, timing_mid);
  }
}

void convert_ext_seq_unsigned_timing(ExtSeqTrackData &data, uint8_t speed) {
  uint16_t timing_mid = SeqTrack::get_timing_mid(speed);
  uint16_t used_events = data.event_count;
  if (used_events > NUM_EXT_EVENTS) {
    used_events = NUM_EXT_EVENTS;
  }
  for (uint16_t i = 0; i < used_events; i++) {
    uint8_t timing =
        *reinterpret_cast<uint8_t *>(&data.events[i].micro_timing);
    data.events[i].micro_timing =
        SeqTrack::timing_to_microtiming(timing, timing_mid);
  }
}

void copy_legacy_md_seq(MDSeqTrackData &dst, const LegacyMDSeqTrackData &src,
                        uint8_t speed) {
  // Every MDSeqTrackDataV1 field is populated here; the legacy lock-enable
  // bit occupies the current swing bit and is cleared after the bulk copy.
  memcpy(dst.locks, src.locks, sizeof(dst.locks));
  memcpy(dst.locks_params, src.locks_params, sizeof(dst.locks_params));
  memcpy(dst.microtiming, src.timing, sizeof(dst.microtiming));
  convert_md_seq_unsigned_timing(dst, speed);

  memcpy(dst.steps, src.steps, sizeof(dst.steps));
  for (uint8_t step = 0; step < NUM_MD_STEPS; step++) {
    dst.steps[step].swing = false;
  }
  dst.clean_params();
}

bool read_legacy_ext_seq(Grid &grid, ExtSeqTrackData &dst, uint8_t speed) {
  if (!grid.read(&dst, sizeof(LegacyExtSeqTrackData))) {
    return false;
  }

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
  return true;
}

bool write_migrated_track(Grid &grid, GridColumn column, GridRow row,
                          DeviceTrack &track, uint16_t write_size) {
  track.version = track.storage_version();
  track.reserved = 0;
  return grid.write(track._this(), write_size, column, row);
}

bool write_migrated_payload_track(Grid &grid, GridColumn column, GridRow row,
                                  LegacyGridTrackHeader &header,
                                  void *payload, size_t payload_size) {
  return grid.seek(column, row) &&
         grid.write(&header, sizeof(header)) &&
         grid.write(payload, payload_size);
}

void init_upgraded_md_track(MDTrack &dst, const GridLink &link,
                            const LegacyMDSeqTrackData &seq,
                            const MDMachine &machine) {
  dst.active = MD_TRACK_TYPE;
  dst.link = link;
  dst.mod_data.init();
  copy_legacy_md_seq(dst.seq_data, seq, project_seq_speed_value(link));
  dst.machine = machine;
}

bool read_upgraded_md_track(Grid &grid, GridColumn column, GridRow row,
                            MDTrack &dst) {
  LegacyGridTrackHeader legacy_header;
  if (!read_legacy_header(grid, column, row, MD_TRACK_TYPE,
                          &legacy_header)) {
    return false;
  }
  LegacyMDSeqTrackData legacy_seq;
  MDMachine legacy_machine;
  if (!grid.read(&legacy_seq, sizeof(legacy_seq)) ||
      !grid.read(&legacy_machine, sizeof(legacy_machine))) {
    return false;
  }
  init_upgraded_md_track(dst, legacy_header.link, legacy_seq, legacy_machine);
  copy_legacy_header(dst, legacy_header);
  return true;
}

bool migrate_md_track_native_swing(Grid &grid, GridColumn column, GridRow row) {
  MDTrack upgraded;
  if (!read_upgraded_md_track(grid, column, row, upgraded)) {
    return false;
  }
  return write_migrated_track(grid, column, row, upgraded,
                              upgraded.get_track_size());
}

bool migrate_ext_like_track_storage(Grid &grid, GridColumn column, GridRow row,
                                    uint8_t track_type,
                                    DeviceTrack &upgraded,
                                    ExtSeqTrackData &seq_data) {
  LegacyGridTrackHeader legacy_header;
  if (!read_legacy_header(grid, column, row, track_type, &legacy_header)) {
    return false;
  }
  upgraded.init_defaults();
  copy_legacy_header(upgraded, legacy_header);
  upgraded.active = track_type;
  if (!read_legacy_ext_seq(grid, seq_data, project_seq_speed_value(upgraded.link))) {
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
                                 GridColumn dst_column, uint8_t track_type,
                                 uint8_t payload_size) {
  static_assert(sizeof(MDFXData) >= sizeof(TempoData),
                "fixed payload scratch too small for tempo");
  static_assert(sizeof(MDFXData) >= sizeof(GridChain),
                "fixed payload scratch too small for grid chain");
  uint8_t payload[sizeof(MDFXData)];
  LegacyGridTrackHeader legacy_header;
  if (!read_legacy_header(grid, column, row, track_type, &legacy_header)) {
    return false;
  }
  if (!grid.read(payload, payload_size)) {
    return false;
  }
  LegacyGridTrackHeader header;
  init_migrated_header(header, legacy_header, track_type, 0);
  return write_migrated_payload_track(grid, dst_column, row, header, payload,
                                      payload_size);
}

bool migrate_perf_track_storage(Grid &grid, GridColumn column, GridRow row,
                                GridColumn dst_column) {
  LegacyGridTrackHeader legacy_header;
  if (!read_legacy_header(grid, column, row, PERF_TRACK_TYPE,
                          &legacy_header)) {
    return false;
  }

  LegacyPerfTrackData legacy_perf;
  if (!grid.read(&legacy_perf, sizeof(legacy_perf))) {
    return false;
  }

  PerfTrack upgraded;
  upgraded.init_defaults();
  copy_legacy_header(upgraded, legacy_header);
  upgraded.active = PERF_TRACK_TYPE;
  copy_legacy_perf_track_data(upgraded, legacy_perf);
  return write_migrated_track(grid, dst_column, row, upgraded,
                              upgraded.get_track_size());
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
  upgraded.init_defaults();
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

void normalize_project_config(MCLSysConfigData *data) {
  data->version = CONFIG_VERSION;
  data->project[0] = '\0';
  data->number_projects = 0;
  data->project_config = 0;
}

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
    gfx.alert("ERROR", "BAD NAME");
    return false;
  }

  char proj_filename[PRJ_NAME_LEN  + 5];
  if (!project_file_name(basename, proj_filename, sizeof(proj_filename))) {
    gfx.alert("ERROR", "BAD NAME");
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
    gfx.alert("ERROR", existing_project ? "PROJECT EXISTS" : "DIR EXISTS");
    return false;
  }
  if (!SD.mkdir(newprj, true) || !SD.chdir(newprj)) {
    gfx.alert("ERROR", "DIR");
    return false;
  }


  draw_wait_popup("CREATING PROJECT");

  DEBUG_PRINTLN(proj_filename);
  if (SD.exists(proj_filename)) {
    gfx.alert("ERROR", "PROJECT EXISTS");
    return false;
  }

  // Initialise Grid Files.
  //

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_filename[PRJ_NAME_LEN  + 5];
    if (!build_grid_filename(basename, i, grid_filename,
                             sizeof(grid_filename))) {
      gfx.alert("ERROR", "BAD NAME");
      return false;
    }
    if (!SD.exists(grid_filename)) {
      if (!grids[i].new_grid(grid_filename)) {
        gfx.alert("ERROR", "SD ERROR");
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
        gfx.alert("ERROR", "BAD PATH");
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
      gfx.alert("ERROR", "SD ERROR");
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

bool Project::convert_project(const char *projectname) { return true; }

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
  if (!ok || header_version < PROJ_MIN_READABLE_VERSION) {
    return false;
  }
  return true;
}

bool Project::load_project(const char *projectname) {
  return load_project_impl(projectname, 0, false);
}

bool Project::load_project_version(const char *projectname, uint8_t pair) {
  return load_project_impl(projectname, pair, true);
}

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

  uint8_t migration_flags = 0;
  if (version == PROJ_MIN_READABLE_VERSION) {
    migration_flags |= MIGRATE_TRACK_STORAGE;
    migration_flags |= MIGRATE_GRID_PAIRS;
    active_grid_pair = 0;
    migration_flags |= MIGRATE_PROJECT_CONFIG;
  }
  uint8_t pair = use_requested_pair ? requested_pair : active_grid_pair;
  if (migration_flags) {
    draw_wait_popup("UPGRADING PROJECT");
  }

  if (pair >= 128 || !project_pair_exists(pair, project_basename)) {
    if (use_requested_pair) {
      DEBUG_PRINTLN(F("requested grid pair missing"));
      return false;
    }
    pair = 0;
    active_grid_pair = 0;
    migration_flags |= MIGRATE_GRID_PAIRS;
    if (!project_pair_exists(pair, project_basename)) {
      DEBUG_PRINTLN(F("default grid pair missing"));
      return false;
    }
  }

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
      gfx.alert("ERROR", "OPEN GRID");
      return false;
    }
  }

  if ((migration_flags & MIGRATE_TRACK_STORAGE) &&
      !migrate_track_storage_versions()) {
    DEBUG_PRINTLN(F("Could not migrate project tracks"));
    return false;
  }
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
  if ((migration_flags || use_requested_pair) && !write_header()) {
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
  return version == PROJ_MIN_READABLE_VERSION || version >= PROJ_VERSION;
}

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
        SeqLFOData legacy_lfo;
        legacy_track.lfo_data.store_data(&legacy_lfo);

        MDTrack upgraded_md_track;
        if (grid_x_header->track_type[0] == MD_TRACK_TYPE) {
          if (!read_upgraded_md_track(grids[0], 0, row, upgraded_md_track)) {
            return false;
          }
        } else if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
          upgraded_md_track.init();
          upgraded_md_track.link.init(row);
          upgraded_md_track.machine.track = 0;
          upgraded_md_track.machine.lfo.init(0);
        }

        upgraded_md_track.mod_data.init();
        LFOSeqTrack::convert_legacy_data(legacy_lfo,
                                         &upgraded_md_track.mod_data.lfo);

        if (!write_migrated_track(grids[0], 0, row, upgraded_md_track,
                                  upgraded_md_track.get_track_size())) {
          return false;
        }
        if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
          grid_x_header->active = true;
          grid_x_header->update_model(0, upgraded_md_track.get_model(),
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
      MDRouteTrack new_route;
      copy_legacy_header(new_route, legacy_route.header);
      new_route.active = MD_ROUTE_TRACK_TYPE;
      memcpy(new_route.routing, legacy_route.route.routing,
             sizeof(new_route.routing));
      uint8_t ptc_group =
          cfg.uart2_poly_chan >= PTC_MIDI_GROUP_MIN &&
                  cfg.uart2_poly_chan <= PTC_MIDI_GROUP_MAX
              ? cfg.uart2_poly_chan
              : PTC_GROUP_LOCAL;
      uint16_t poly_mask = legacy_route.route.poly_mask;
      for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i, poly_mask >>= 1) {
        new_route.ptc_group[i] =
            (poly_mask & 1) ? ptc_group : PTC_GROUP_OFF;
      }

      if (!write_migrated_track(grids[1], MDROUTE_TRACK_NUM, row,
                                new_route, new_route.get_track_size())) {
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
      switch (row_header.track_type[column]) {
      case MD_TRACK_TYPE:
        if (grid == 0 &&
            !migrate_md_track_native_swing(grids[grid], column, row)) {
          return false;
        }
        break;
      case EXT_TRACK_TYPE:
      {
        ExtTrack upgraded;
        if (!migrate_ext_like_track_storage(grids[grid], column, row,
                                            EXT_TRACK_TYPE, upgraded,
                                            upgraded.seq_data)) {
          return false;
        }
        break;
      }
      case A4_TRACK_TYPE:
      {
        A4Track upgraded;
        if (!migrate_ext_like_track_storage(grids[grid], column, row,
                                            A4_TRACK_TYPE, upgraded,
                                            upgraded.seq_data)) {
          return false;
        }
        break;
      }
      case MNM_TRACK_TYPE:
      {
        MNMTrack upgraded;
        if (!migrate_ext_like_track_storage(grids[grid], column, row,
                                            MNM_TRACK_TYPE, upgraded,
                                            upgraded.seq_data)) {
          return false;
        }
        break;
      }
      case MDFX_TRACK_TYPE:
        if (!migrate_fixed_payload_track(grids[grid], column, row, column,
                                         MDFX_TRACK_TYPE, sizeof(MDFXData))) {
          return false;
        }
        break;
      case MDTEMPO_TRACK_TYPE:
        if (!migrate_fixed_payload_track(grids[grid], column, row, column,
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
        if (!migrate_fixed_payload_track(grids[grid], column, row, column,
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

    ok = dst_grid.write_header();

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
