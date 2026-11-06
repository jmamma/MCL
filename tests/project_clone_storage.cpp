#include "Project.h"
#include "MDTrack.h"

#include <array>
#include <cstdio>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

static void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

static std::string gridPath(const char* project, uint8_t id) {
    char leaf[PRJ_NAME_LEN + 5];
    require(proj.build_grid_filename(project, id, leaf, sizeof(leaf)), "grid name");
    return std::string("/Projects/") + project + "/" + leaf;
}

static void populatePair(const char* project, uint8_t pair, uint32_t version) {
    for (uint8_t g = 0; g < NUM_GRIDS; ++g) {
        Grid grid;
        const uint8_t id = pair * NUM_GRIDS + g;
        const auto path = gridPath(project, id);
        require(SD.exists(path.c_str()) ? grid.open_file(path.c_str())
                                      : grid.new_file(path.c_str()), "create source grid");
        require(grid.write_header(version, id), "source grid header");
        for (GridRow row = 0; row < GRID_LENGTH; ++row) {
            GridRowHeader header;
            header.init(); header.active = true;
            std::snprintf(header.name, sizeof(header.name), "pair%u-row%u", pair, row);
            for (GridColumn col = 0; col < GRID_WIDTH; ++col) {
                header.update_model(col, 16, MD_TRACK_TYPE);
                MDTrack track;
                track.init_defaults(); track.link.init(row);
                track.link.length = 2 + (row + col) % 63;
                track.machine.track = col;
                track.machine.model = 16;
                track.seq_data.steps[0].trig = true;
                track.seq_data.steps[track.link.length - 1].trig = true;
                track.seq_data.steps[0].locks = 1;
                track.seq_data.locks_params[0] = 4;
                track.seq_data.locks[0] = 20 + col;
                std::array<uint8_t, GRID_SLOT_BYTES> bytes{};
                for (size_t i = 0; i < bytes.size(); ++i)
                    bytes[i] = uint8_t(1 + (i + (i >> 8) + row + col + id) % 255);
                std::memcpy(bytes.data(), track._this(), track._sizeof());
                require(grid.write(bytes.data(), bytes.size(), col, row), "source track");
            }
            require(grid.write_row_header(&header, row), "source row header");
        }
        require(grid.sync(), "sync source grid");
        grid.close_file();
    }
}

static void verifyPair(const char* from, const char* to, uint8_t pair) {
    for (uint8_t g = 0; g < NUM_GRIDS; ++g) {
        Grid source, clone;
        require(source.open_file(gridPath(from, pair * 2 + g).c_str()) &&
                clone.open_file(gridPath(to, pair * 2 + g).c_str()), "open copied grids");
        require(source.read_header() && clone.read_header(), "read grid headers");
        require(std::memcmp(static_cast<GridHeader*>(&source),
                            static_cast<GridHeader*>(&clone), sizeof(GridHeader)) == 0,
                "grid header changed");
        std::array<uint8_t, GRID_SLOT_BYTES> expected{}, actual{};
        for (GridRow row = 0; row < GRID_LENGTH; ++row) {
            GridRowHeader a, b;
            require(source.read_row_header(&a, row) && clone.read_row_header(&b, row) &&
                    std::memcmp(&a, &b, sizeof(a)) == 0, "row header changed");
            for (GridColumn col = 0; col < GRID_WIDTH; ++col) {
                require(source.read(expected.data(), expected.size(), col, row) &&
                        clone.read(actual.data(), actual.size(), col, row), "read copied slot");
                require(expected == actual, "cloned track data differs");
                if (clone.version == PROJ_VERSION) {
                    MDTrack decoded;
                    std::memcpy(decoded._this(), actual.data(), decoded._sizeof());
                    require(decoded.link.length == 2 + (row + col) % 63 &&
                            decoded.seq_data.steps[0].trig &&
                            decoded.seq_data.steps[decoded.link.length - 1].trig &&
                            decoded.seq_data.steps[0].locks == 1 &&
                            decoded.seq_data.locks_params[0] == 4 &&
                            decoded.seq_data.locks[0] == 20 + col,
                            "cloned length, trigs or locks lost");
                }
            }
        }
        source.close_file(); clone.close_file();
    }
}

static void cloneCase(const char* from, const char* to, bool upgraded) {
    require(proj.new_project(from), "create fresh project");
    populatePair(from, 0, upgraded ? PROJ_MIN_READABLE_VERSION : PROJ_VERSION);
    if (upgraded) {
        // An upgraded project retains its original grids and selects a new pair.
        // A nonconsecutive saved version must survive cloning too.
        populatePair(from, 1, PROJ_VERSION);
        populatePair(from, 3, PROJ_VERSION);
        File header;
        const std::string path = std::string("/Projects/") + from + "/" + from + ".mcl";
        require(header.open(path.c_str(), O_RDWR), "open source project header");
        const uint8_t active = 1;
        require(header.seekSet(offsetof(ProjectHeader, active_grid_pair)) &&
                header.write(&active, sizeof(active)) == sizeof(active) && header.sync(),
                "select upgraded pair");
        header.close();
    }
    require(proj.copy_project(from, to), "clone project");
    uint8_t active = 255;
    require(proj.read_active_grid_pair(to, &active) && active == unsigned(upgraded),
            "clone selected wrong grid pair");
    verifyPair(from, to, 0);
    if (upgraded) { verifyPair(from, to, 1); verifyPair(from, to, 3); }
    // Reusing an existing destination must not damage it or the source.
    require(!proj.copy_project(from, to), "clone overwrote existing project");
    verifyPair(from, to, upgraded ? 1 : 0);
}

int main() {
    const fs::path root = fs::temp_directory_path() /
        ("mcl-clone-storage-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!fs::create_directory(root)) return 1;
    int result = 0;
    try {
        mcl_desktop_set_sd_root(root.string().c_str());
        mcl_sd.mcl_root[0] = '\0';
        cloneCase("Fresh", "FreshClone", false);
        cloneCase("Upgraded", "UpgradedClone", true);
        // A partial source cannot produce a successful or retained destination.
        const auto truncated = root / fs::path(gridPath("Fresh", 0)).relative_path();
        fs::resize_file(truncated, 8192);
        require(!proj.copy_project("Fresh", "TruncatedClone"), "accepted truncated grid");
        require(!fs::exists(root / "Projects/TruncatedClone"), "failed clone left behind");
        require(fs::file_size(truncated) == 8192,
                "source modified by failed clone");
        std::puts("Project cloning passed: fresh/upgraded pairs, lengths, trigs, locks, full slots and failure cleanup");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Project cloning failed: %s\n", error.what());
        result = 1;
    }
    proj.close_project();
    fs::remove_all(root);
    return result;
}
