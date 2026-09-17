#include "Drivers/MD/MD.h"
#include "Drivers/MD/GridTracks/MDFXTrack.h"
#include "Drivers/MD/GridTracks/SPSXTrack.h"
#include "ElektronDataEncoder.h"
#include "Host/SpsHostSeqBridge.h"
#include "Host/SpsSeqProtocol.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "MidiUart.h"
#include "Project.h"
#include "Sequencer/MCLSeq.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>

namespace {

constexpr uint16_t kTestSysexCapacity = 60000;

struct SysexFixture {
    std::array<uint8_t, kTestSysexCapacity> storage{};
    RingBuffer<> ring;
    MidiUartClass uart;
    MidiSysexClass sysex;
    MidiClass midi;

    SysexFixture()
        : ring(storage.data(), (uint16_t)storage.size()),
          sysex(&uart, &ring), midi(&uart, &sysex) {}
};

bool publishToMidiSysex(MidiSysexClass& sysex, const uint8_t* message,
                        uint16_t length) {
    if (!message || length < 2 || message[0] != 0xF0 ||
        message[length - 1] != 0xF7 ||
        length - 2 >= sysex.rb->len) {
        return false;
    }
    const uint8_t record = sysex.msg_wr;
    sysex.startRecord();
    for (uint16_t i = 1; i + 1 < length; ++i)
        sysex.recordByte(message[i]);
    sysex.stopRecord();
    sysex.rd_cur = record;
    return true;
}

bool checkKitWireCapability(uint32_t caps, uint8_t expectedVersion,
                            uint16_t expectedSize) {
    std::array<uint8_t, 2048> message{};
    MD.is_spsx = true;
    MD.fw_caps = FW_CAP_SPSX | caps;
    for (uint8_t track = 0; track < 16; ++track) {
        MD.kit.params[track][MODEL_ENVDEST] = track % 3;
        MD.kit.outputRouting[track] = mdKitRouteFromSelection(track % 8);
    }
    ElektronDataToSysexEncoder encoder(message.data());
    const uint16_t length = MD.kit.toSysex(&encoder);
    auto fixture = std::make_unique<SysexFixture>();
    if (length != expectedSize || message[7] != expectedVersion ||
        !publishToMidiSysex(fixture->sysex, message.data(), length)) {
        return false;
    }
    MDKit decoded;
    if (!decoded.fromSysex(&fixture->midi)) return false;
    for (uint8_t track = 0; track < 16; ++track) {
        if (decoded.params[track][MODEL_ENVDEST] != track % 3 ||
            decoded.outputRouting[track] != MD.kit.outputRouting[track])
            return false;
    }
    std::array<uint8_t, 2048> roundtrip{};
    ElektronDataToSysexEncoder again(roundtrip.data());
    if (decoded.toSysex(&again) != length ||
        std::memcmp(message.data(), roundtrip.data(), length) != 0)
        return false;
    for (uint8_t param = MODEL_ENVATT; param < SPS_PARAMS_PER_TRACK; ++param) {
        const char* label = model_param_name(0, param);
        if (!label || std::strlen(label) != 3) return false;
    }
    return true;
}

bool checkGlobalVersions() {
    std::array<uint8_t, 512> message{}, roundtrip{};
    static MDGlobal source;
    source.seqTempoMode = 1;
    source.channelMode = 1;
    for (bool native : {false, true}) {
        MD.is_spsx = native;
        ElektronDataToSysexEncoder encoder(message.data());
        const uint16_t length = source.toSysex(&encoder);
        if (message[7] != (native ? SPSX_GLOBAL_VERSION : MDX_GLOBAL_VERSION)) return false;
        auto fixture = std::make_unique<SysexFixture>();
        if (!publishToMidiSysex(fixture->sysex, message.data(), length)) return false;
        MDGlobal decoded;
        if (!decoded.fromSysex(&fixture->midi) ||
            decoded.seqTempoMode != unsigned(native) ||
            decoded.channelMode != unsigned(native)) return false;
        ElektronDataToSysexEncoder again(roundtrip.data());
        if (decoded.toSysex(&again) != length ||
            std::memcmp(message.data(), roundtrip.data(), length)) return false;
    }
    return true;
}

bool checkMachineRoutePackets() {
    MidiUartClass output;
    auto* savedUart = MD.uart;
    const bool wasConnected = MD.connected;
    MD.uart = &output;
    MD.connected = true;
    MD.is_spsx = true;
    bool ok = true;
    SPSMachine machine{};
    machine.init();
    machine.params[MODEL_ENVDEST] = 2;
    machine.outputRouting = 6;
    for (uint8_t level : {uint8_t(255), uint8_t(59)}) {
        output.init();
        MD.assignMachineBulk(3, &machine, level, 255, true);
        std::array<uint8_t, 128> bytes{};
        const size_t size = output.desktop_egress(bytes.data(), bytes.size());
        ok &= size == size_t(64 + (level != 255)) && bytes[0] == 0xf0 &&
              bytes[size - 1] == 0xf7 && (bytes[11] & 4) &&
              bytes[12 + MODEL_ENVDEST] == 2 && bytes[size - 2] == 6;
        if (level != 255) ok &= bytes[size - 3] == level;
    }
    MD.save_kit_params();
    const uint8_t route = MD.kit.outputRouting[3];
    MD.kit.outputRouting[3] = 0;
    MD.restore_kit_params();
    ok &= MD.kit.outputRouting[3] == route;
    MD.uart = savedUart;
    MD.connected = wasConnected;
    return ok;
}

// Cross-codec fixture entry point: accepts a firmware-emitted full kit and
// writes the actual MCL decoder/encoder result for byte comparison.
int roundtripKitFile(const char* input, const char* output) {
    std::array<uint8_t, 2048> message{}, roundtrip{};
    FILE* file = std::fopen(input, "rb");
    if (!file) return 2;
    const size_t length = std::fread(message.data(), 1, message.size(), file);
    std::fclose(file);
    auto fixture = std::make_unique<SysexFixture>();
    MDKit decoded;
    MD.is_spsx = true;
    if (!publishToMidiSysex(fixture->sysex, message.data(), length) ||
        !decoded.fromSysex(&fixture->midi)) return 3;
    ElektronDataToSysexEncoder encoder(roundtrip.data());
    const uint16_t size = decoded.toSysex(&encoder);
    file = std::fopen(output, "wb");
    if (!file) return 4;
    const bool ok = std::fwrite(roundtrip.data(), 1, size, file) == size;
    std::fclose(file);
    return ok ? 0 : 5;
}

bool checkPatternWireCapability(uint32_t caps, uint8_t expectedVersion,
                                bool expectBusLock) {
    std::array<uint8_t, 16384> message{};
    MD.is_spsx = true;
    MD.fw_caps = FW_CAP_SPSX | caps;
    MD.pattern.clearPattern();
    MD.pattern.patternLength = 64;

    // A stock-range lock must survive both the stock fallback and compact
    // format. BUS1-3 exercise the compact-only parameter range.
    if (!MD.pattern.addLock(0, 0, MODEL_REV, 55))
        return false;
    MD.pattern.ext_locks_params[0][MODEL_REV] = MODEL_REV + 1;
    constexpr uint8_t tracks[] = {0, 3, 4, 15};
    constexpr uint8_t params[] = {MODEL_BUS1, MODEL_BUS2, MODEL_BUS3};
    for (uint8_t track : tracks) {
        MD.pattern.trigPatterns[track] =
            (uint64_t(1) << 5) | (uint64_t(1) << 45);
        for (uint8_t param : params) {
            MD.pattern.ext_locks_params[track][param] = param + 1;
            if (!MD.pattern.addLock(track, 5, param,
                                    (uint8_t)((20 + track + param) &
                                              0x7f)) ||
                !MD.pattern.addLock(track, 45, param,
                                    (uint8_t)((80 + track + param) &
                                              0x7f))) {
                return false;
            }
        }
    }

    ElektronDataToSysexEncoder encoder(message.data());
    const uint16_t length = MD.pattern.toSysex(&encoder);
    auto fixture = std::make_unique<SysexFixture>();
    if (length == 0 || length > message.size() ||
        message[7] != expectedVersion ||
        !publishToMidiSysex(fixture->sysex, message.data(), length)) {
        std::fprintf(stderr,
                     "pattern capability envelope: len=%u ver=%02x "
                     "expected=%02x\n",
                     length, message[7], expectedVersion);
        return false;
    }

    MDPattern decoded;
    if (!decoded.fromSysex(&fixture->midi) ||
        decoded.version != expectedVersion) {
        std::fprintf(stderr,
                     "pattern capability decode: len=%u decoded-ver=%02x "
                     "expected=%02x\n",
                     length, decoded.version, expectedVersion);
        return false;
    }
    const int16_t legacyRow = decoded.paramLocks[0][MODEL_REV];
    const bool hasLegacyLock =
        legacyRow >= 0 && decoded.lock_row((uint16_t)legacyRow)[0] == 55;
    const uint8_t decodedParameterCount =
        decoded.version == MD_PATTERN_VERSION_SPSX
            ? MD_PATTERN_LOCK_SLOTS
            : 24;
    if (!hasLegacyLock ||
        decodedParameterCount !=
            (expectBusLock ? MD_PATTERN_LOCK_SLOTS : 24)) {
        std::fprintf(stderr,
                     "pattern capability base lock: row=%d value=%d "
                     "expect-bus=%d\n",
                     legacyRow,
                     legacyRow >= 0
                         ? decoded.lock_row((uint16_t)legacyRow)[0]
                         : -2,
                     expectBusLock);
        return false;
    }
    for (uint8_t track : tracks) {
        for (uint8_t param : params) {
            const int16_t row = decoded.paramLocks[track][param];
            const bool present = row >= 0;
            if (present != expectBusLock) {
                std::fprintf(stderr,
                             "pattern capability bus lock: track=%u "
                             "param=%u row=%d expected=%d\n",
                             track, param, row, expectBusLock);
                return false;
            }
            if (present &&
                (decoded.lock_row((uint16_t)row)[5] !=
                     (int8_t)((20 + track + param) & 0x7f) ||
                 decoded.lock_row((uint16_t)row)[45] !=
                     (int8_t)((80 + track + param) & 0x7f))) {
                std::fprintf(stderr,
                             "pattern capability value: track=%u param=%u "
                             "row=%d got=%d/%d expected=%u/%u\n",
                             track, param, row,
                             decoded.lock_row((uint16_t)row)[5],
                             decoded.lock_row((uint16_t)row)[45],
                             (uint8_t)((20 + track + param) & 0x7f),
                             (uint8_t)((80 + track + param) & 0x7f));
                return false;
            }
        }
    }
    return true;
}

uint8_t stressLockValue(uint8_t track, uint8_t param, uint8_t step) {
    return (uint8_t)((track * 53u + param * 29u + step * 17u +
                      step * step + (param ^ step) * 7u) & 0x7Fu);
}

bool hasValidElektronEnvelope(
    const uint8_t* bytes, uint16_t length) {
    if (bytes == nullptr || length < 15 ||
        bytes[0] != 0xf0 || bytes[length - 1] != 0xf7) {
        return false;
    }
    uint16_t checksum = 0;
    for (uint16_t index = 9; index < length - 5; ++index) {
        if (bytes[index] > 0x7f)
            return false;
        checksum = (checksum + bytes[index]) & 0x3fff;
    }
    const uint16_t storedChecksum =
        ((uint16_t)bytes[length - 5] << 7) |
        bytes[length - 4];
    const uint16_t storedLength =
        ((uint16_t)bytes[length - 3] << 7) |
        bytes[length - 2];
    return checksum == storedChecksum &&
           storedLength == (uint16_t)(length - 10);
}

bool checkCanonicalPatternEnvelope(uint16_t& encodedLength) {
    std::array<uint8_t, 8192> message{};
    MD.is_spsx = true;
    MD.fw_caps = FW_CAP_SPSX | FW_CAP_SPSX_PARAMS_37;
    MD.pattern.clearPattern();
    MD.pattern.origPosition = 37;
    MD.pattern.patternLength = 64;
    MD.pattern.scale = 2;
    MD.pattern.doubleTempo = 1;
    MD.pattern.kit = 9;
    MD.pattern.patternTempo = 0x1234;
    MD.pattern.accentAmount = (13u >> 1) | (13u << 3);
    MD.pattern.swingAmount = 7u * 16384u / 30u;
    MD.pattern.accentPattern = UINT64_C(0x0000000000000004);
    MD.pattern.slidePattern = UINT64_C(0x0000000000000008);
    MD.pattern.swingPattern = UINT64_C(0x00000000aaaaaaaa);
    MD.pattern.accentEditAll = 1;
    MD.pattern.slideEditAll = 1;
    MD.pattern.swingEditAll = 1;
    MD.pattern.ext_scale_mode = 1;
    MD.pattern.ext_tempo_multiplier = 0;
    MD.pattern.ext_chain_row = 52;
    MD.pattern.ext_chain_loops = 3;
    MD.pattern.chain_change = 7;
    MD.pattern.trigPatterns[1] =
        (UINT64_C(1) << 3) | (UINT64_C(1) << 40);
    MD.pattern.ext_track_lengths[1] = 48;
    MD.pattern.ext_track_speeds[1] = 2;
    if (!MD.pattern.addLock(1, 3, 2, 11) ||
        !MD.pattern.addLock(1, 40, 33, 99)) {
        return false;
    }
    MD.pattern.ext_microtiming[1][40] = -7;
    MD.pattern.ext_step_flags[1][3] = 1;
    MD.pattern.ext_step_flags[1][40] = (5u << 2) | 2u;

    ElektronDataToSysexEncoder encoder(message.data());
    encodedLength = MD.pattern.toSysex(&encoder);
    if (encodedLength == 0 ||
        message[7] != MD_PATTERN_VERSION_SPSX) {
        return false;
    }
    return hasValidElektronEnvelope(
        message.data(), encodedLength);
}

bool checkMaximumPatternRoundTrip(uint16_t& encodedLength) {
    std::array<uint8_t, kTestSysexCapacity> message{};
    MD.is_spsx = true;
    MD.fw_caps = FW_CAP_SPSX | FW_CAP_SPSX_PARAMS_37;
    MD.pattern.clearPattern();
    MD.pattern.patternLength = 64;
    ElektronPattern* editingApi = &MD.pattern;

    for (uint8_t track = 0; track < 16; ++track) {
        MD.pattern.trigPatterns[track] = UINT64_MAX;
        for (uint8_t param = 0; param < SPS_PARAMS_PER_TRACK; ++param) {
            MD.pattern.ext_locks_params[track][param] = param + 1;
            for (uint8_t step = 0; step < 64; ++step) {
                if (!editingApi->addLock(
                        track, step, param,
                        stressLockValue(track, param, step))) {
                    return false;
                }
            }
        }
    }

    if (MD.pattern.getLockIdx(15, MODEL_ENVDEST) != MAX_LOCK_ROWS - 1 ||
        editingApi->getNextEmptyLock() != -1 ||
        editingApi->addLock(16, 0, 0, 1)) {
        return false;
    }

    // Exercise clearing/reallocation through the generic API on the final
    // extended row, not just direct wire-array access.
    for (uint8_t step = 0; step < 64; ++step)
        editingApi->clearLock(15, step, MODEL_ENVDEST);
    if (MD.pattern.getLockIdx(15, MODEL_ENVDEST) != -1)
        return false;
    for (uint8_t step = 0; step < 64; ++step) {
        if (!editingApi->addLock(
                15, step, MODEL_ENVDEST,
                stressLockValue(15, MODEL_ENVDEST, step))) {
            return false;
        }
    }

    // A complete 608-lane desktop pattern is a valid hosted live record, but
    // exceeds the MKII hardware event pool. The wire encoder must reject it.
    ElektronDataToSysexEncoder oversizedEncoder(message.data());
    if (MD.pattern.toSysex(&oversizedEncoder) != 0)
        return false;

    // Two fully populated 38-lane tracks fit the MKII event pool and exercise
    // the same dense canonical lane representation.
    MD.pattern.clearPattern();
    MD.pattern.patternLength = 64;
    constexpr uint8_t kWireTracks = 2;
    for (uint8_t track = 0; track < kWireTracks; ++track) {
        MD.pattern.trigPatterns[track] = UINT64_MAX;
        for (uint8_t step = 0; step < 64; ++step)
            MD.pattern.ext_step_flags[track][step] = 1;
        for (uint8_t param = 0;
             param < SPS_PARAMS_PER_TRACK; ++param) {
            MD.pattern.ext_locks_params[track][param] = param + 1;
            for (uint8_t step = 0; step < 64; ++step) {
                if (!editingApi->addLock(
                        track, step, param,
                        stressLockValue(track, param, step))) {
                    return false;
                }
            }
        }
    }

    ElektronDataToSysexEncoder encoder(message.data());
    encodedLength = MD.pattern.toSysex(&encoder);
    if (encodedLength == 0 || encodedLength > 16393 ||
        encodedLength >= message.size() ||
        message[7] != MD_PATTERN_VERSION_SPSX) {
        return false;
    }

    auto fixture = std::make_unique<SysexFixture>();
    if (!publishToMidiSysex(fixture->sysex, message.data(), encodedLength) ||
        fixture->sysex.get_recordLen() != encodedLength - 2) {
        std::fprintf(stderr,
                     "maximum pattern publish: len=%u record=%u\n",
                     encodedLength, fixture->sysex.get_recordLen());
        return false;
    }

    auto decoded = std::make_unique<MDPattern>();
    if (!decoded->fromSysex(&fixture->midi) ||
        decoded->version != MD_PATTERN_VERSION_SPSX ||
        decoded->numRows !=
            kWireTracks * SPS_PARAMS_PER_TRACK) {
        std::fprintf(stderr,
                     "maximum pattern decode: version=%02x rows=%u "
                     "expected=%u\n",
                     decoded->version, decoded->numRows,
                     kWireTracks * SPS_PARAMS_PER_TRACK);
        return false;
    }
    for (uint8_t track = 0; track < kWireTracks; ++track) {
        for (uint8_t param = 0; param < SPS_PARAMS_PER_TRACK; ++param) {
            const int16_t row = decoded->paramLocks[track][param];
            if (row < 0 || row >= MAX_LOCK_ROWS)
                return false;
            const int8_t* values = decoded->lock_row((uint16_t)row);
            for (uint8_t step = 0; step < 64; ++step) {
                if (values[step] !=
                    (int8_t)stressLockValue(track, param, step)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool checkMalformedCanonicalPatternRejected() {
    std::array<uint8_t, 4096> message{};
    std::array<uint8_t, 4096> body{};
    MD.fw_caps = FW_CAP_SPSX | FW_CAP_SPSX_PARAMS_37;
    MD.pattern.clearPattern();
    MD.pattern.patternLength = 32;
    if (!MD.pattern.addLock(0, 3, 2, 77))
        return false;

    ElektronDataToSysexEncoder encoder(message.data());
    const uint16_t length = MD.pattern.toSysex(&encoder);
    if (length < 15 || message[7] != MD_PATTERN_VERSION_SPSX)
        return false;

    const uint16_t wireBytes = length - 15;
    const uint16_t bodyBytes = ElektronHelper::ElektronSysexToData(
        message.data() + 10, body.data(), wireBytes);
    constexpr uint16_t kFirstEvent =
        16 + 40 + 16 * 40;
    if (bodyBytes <= kFirstEvent + 1)
        return false;

    // Set a reserved descriptor bit, then rebuild a checksum-valid message.
    // The failure therefore exercises the body parser, not the envelope.
    body[kFirstEvent] |= 0x10;
    if (ElektronHelper::ElektronDataToSysex(
            body.data(), message.data() + 10, bodyBytes) != wireBytes) {
        return false;
    }
    ElektronHelper::calculateSysexChecksum(
        message.data(), (uint16_t)(length - 5));

    auto fixture = std::make_unique<SysexFixture>();
    if (!publishToMidiSysex(fixture->sysex, message.data(), length))
        return false;

    MDPattern decoded;
    decoded.clearPattern();
    decoded.patternLength = 64;
    if (!decoded.addLock(1, 5, 3, 99) ||
        decoded.fromSysex(&fixture->midi)) {
        return false;
    }
    return decoded.patternLength == 64 &&
           decoded.paramLocks[1][3] >= 0 &&
           decoded.getLock(1, 5, 3) == 99;
}

#pragma pack(push, 1)
template<uint8_t Params>
struct LegacySpsMachine {
    uint8_t params[Params];
    uint8_t track;
    uint8_t level;
    uint32_t model;
    MDLFO lfos[2];
    uint8_t trigGroup;
    uint8_t muteGroup;
};

template<uint8_t Params>
struct LegacySpsxTrackPayload {
    LegacySpsMachine<Params> machine;
    uint8_t seqVersion;
    BasicStepSeqTrackData<Params> seqData;
    SeqTrackModStorage mod;
    TrackLoadFadeData fade;
};
#pragma pack(pop)

template<uint8_t Params>
bool checkTrackRowMigration(uint8_t version, bool legacySequence) {
    static_assert(sizeof(LegacySpsMachine<Params>) +
                      SPS_PARAMS_PER_TRACK - Params + 1 == sizeof(SPSMachine));
    static_assert(sizeof(LegacySpsxTrackPayload<Params>) +
                      2 * (SPS_PARAMS_PER_TRACK - Params) + 1 ==
                  sizeof(SPSMachine) + sizeof(SPSXTrackStorage) +
                      sizeof(TrackLoadFadeData));
    proj.version = PROJ_VERSION;
    LegacySpsxTrackPayload<Params> old{};
    for (uint8_t param = 0; param < Params; ++param)
        old.machine.params[param] = (uint8_t)(param + 11);
    old.machine.track = 7;
    old.machine.level = 93;
    old.machine.model = 0x20041;
    old.machine.lfos[0].destinationParam = 21;
    old.machine.lfos[1].destinationParam = 28;
    old.machine.trigGroup = 3;
    old.machine.muteGroup = 9;
    old.seqVersion = legacySequence ? SPSX_SEQ_VERSION_LEGACY : SPSX_SEQ_VERSION_SPSX;
    old.seqData.init();
    old.seqData.swing_amount = 19;
    old.seqData.locks_params[Params - 1] = Params;
    old.seqData.steps[7].locks = uint64_t{1} << (Params - 1);
    old.seqData.locks[0] = 88;
    MDSeqTrackData legacy;
    legacy.init();
    legacy.swing_amount = 23;
    legacy.locks_params[0] = 7;
    legacy.steps[3].locks = 1;
    legacy.locks[0] = 49;
    if (legacySequence) std::memcpy(&old.seqData, &legacy, sizeof(legacy));
    std::memset(&old.mod, 0x35, sizeof(old.mod));
    std::memset(&old.fade, 0x59, sizeof(old.fade));

    SPSXTrack migrated;
    migrated.version = version;
    std::memcpy(&migrated.machine, &old, sizeof(old));
    migrated.on_storage_loaded();
    if (migrated.version != SPSX_TRACK_STORAGE_VERSION ||
        std::memcmp(migrated.machine.params, old.machine.params, Params) ||
        std::memcmp(&migrated.machine.track, &old.machine.track,
                    sizeof(old.machine) - Params) ||
        migrated.machine.outputRouting != MD_KIT_ROUTE_GLOBAL ||
        migrated.seq_storage.seq_version != old.seqVersion ||
        std::memcmp(static_cast<SeqTrackModStorage*>(&migrated.seq_storage),
                    &old.mod, sizeof(old.mod)) ||
        std::memcmp(&migrated.load_fade, &old.fade, sizeof(old.fade))) return false;
    for (uint8_t param = Params; param < SPS_PARAMS_PER_TRACK; ++param) {
        if (migrated.machine.params[param] != 0) return false;
    }
    if (legacySequence) {
        if (std::memcmp(&migrated.seq_storage.seq_data.legacy, &legacy,
                        sizeof(legacy))) return false;
    } else {
        const auto& seq = migrated.seq_storage.seq_data.spsx;
        if (seq.swing_amount != 19 || seq.locks_params[Params - 1] != Params ||
            seq.steps[7].locks != (uint64_t{1} << (Params - 1)) ||
            seq.locks[0] != 88) return false;
        for (uint8_t param = Params; param < SPS_PARAMS_PER_TRACK; ++param) {
            if (seq.locks_params[param] != 0) return false;
        }
    }
    // Loading an already migrated record must not change it again.
    std::array<uint8_t, sizeof(SPSXTrack)> before{};
    std::memcpy(before.data(), static_cast<const void*>(&migrated), sizeof(migrated));
    migrated.on_storage_loaded();
    return std::memcmp(before.data(), static_cast<const void*>(&migrated), sizeof(migrated)) == 0;
}

bool checkTrackRowMigrations() {
    for (bool legacy : {false, true}) {
        if (!checkTrackRowMigration<34>(5, legacy) ||
            !checkTrackRowMigration<37>(6, legacy)) return false;
    }
    MDFXTrack oldFx;
    oldFx.version = 0;
    std::memset(oldFx.userBusFx, 1, sizeof(oldFx.userBusFx));
    std::memset(oldFx.userPostFx, 1, sizeof(oldFx.userPostFx));
    oldFx.on_storage_loaded();
    if (oldFx.version != MDFX_TRACK_STORAGE_VERSION_ROUTED_FX) {
        std::fprintf(stderr, "MDFX migration version=%u\n", oldFx.version);
        return false;
    }
    for (const auto& bus : oldFx.userBusFx) {
        for (uint8_t value : bus) {
            if (value != SPS_USER_FX_DEFAULT_PARAM) {
                std::fprintf(stderr, "MDFX bus default=%u\n", value);
                return false;
            }
        }
    }
    for (uint8_t value : oldFx.userPostFx) {
        if (value != SPS_USER_FX_DEFAULT_PARAM) {
            std::fprintf(stderr, "MDFX post default=%u\n", value);
            return false;
        }
    }

    MDFXTrack currentFx;
    currentFx.version = MDFX_TRACK_STORAGE_VERSION_ROUTED_FX;
    std::memset(currentFx.userBusFx, 23, sizeof(currentFx.userBusFx));
    std::memset(currentFx.userPostFx, 47, sizeof(currentFx.userPostFx));
    currentFx.on_storage_loaded();
    const bool preserved = currentFx.userBusFx[0][0] == 23 &&
                           currentFx.userPostFx[0] == 47;
    if (!preserved) {
        std::fprintf(stderr, "MDFX current changed=%u/%u\n",
                     currentFx.userBusFx[0][0], currentFx.userPostFx[0]);
    }
    return preserved;
}

bool checkNegotiatedExtendedCcMapping() {
    MidiUartClass output;
    const bool wasConnected = MD.connected;
    MD.connected = true;
    MD.is_spsx = true;
    MD.global.channelMode = 1;

    for (uint32_t caps : {uint32_t(0), uint32_t(FW_CAP_SPSX_PARAMS_37)}) {
        MD.fw_caps = FW_CAP_SPSX | caps;
        const uint8_t wireParams = SPS_PARAMS_PER_TRACK;
        for (uint8_t base : {uint8_t(0), uint8_t(8)}) {
            MD.global.baseChannel = base;
            for (uint8_t track = 0; track < 16; ++track) {
                for (uint8_t param = MD_PARAMS_PER_TRACK;
                     param < SPS_PARAMS_PER_TRACK; ++param) {
                    SpsExtendedCcAddress address{};
                    if (!spsExtendedParamToCc(base, track, param, address)) {
                        MD.connected = wasConnected;
                        return false;
                    }

                    uint8_t decodedTrack = 0;
                    uint8_t decodedParam = 0;
                    MD.parseCC(address.channel, address.cc, &decodedTrack,
                               &decodedParam);
                    const bool supported = param < wireParams;
                    if (supported ? (decodedTrack != track ||
                                     decodedParam != param)
                                  : decodedTrack != 255) {
                        MD.connected = wasConnected;
                        return false;
                    }

                    output.init();
                    MD.setTrackParam(track, param, 73, &output, true);
                    uint8_t bytes[4] = {};
                    const size_t count =
                        output.desktop_egress(bytes, sizeof(bytes));
                    if (supported) {
                        if (count != 3 ||
                            bytes[0] != (uint8_t)(MIDI_CONTROL_CHANGE |
                                                  address.channel) ||
                            bytes[1] != address.cc || bytes[2] != 73) {
                            MD.connected = wasConnected;
                            return false;
                        }
                    } else if (count != 0) {
                        MD.connected = wasConnected;
                        return false;
                    }
                }
            }
        }
    }

    MD.global.baseChannel = 9;
    output.init();
    MD.setTrackParam(0, MODEL_BUS1, 73, &output, true);
    uint8_t byte = 0;
    const bool invalidBaseRejected = output.desktop_egress(&byte, 1) == 0;
    MD.connected = wasConnected;
    return invalidBaseRejected;
}

bool checkEncoderInterfacePacketWidths() {
    struct Case {
        uint32_t caps;
        uint8_t requestedParams;
        uint8_t expectedParams;
    };
    constexpr Case cases[] = {
        {0, MD_PARAMS_PER_TRACK, MD_PARAMS_PER_TRACK},
        {0, SPS_PARAMS_V1_PER_TRACK, SPS_PARAMS_V1_PER_TRACK},
        {0, SPS_PARAMS_PER_TRACK, SPS_PARAMS_PER_TRACK},
        {FW_CAP_SPSX_PARAMS_37, MD_PARAMS_PER_TRACK, MD_PARAMS_PER_TRACK},
        {FW_CAP_SPSX_PARAMS_37, SPS_PARAMS_V1_PER_TRACK,
         SPS_PARAMS_V1_PER_TRACK},
        {FW_CAP_SPSX_PARAMS_37, SPS_PARAMS_PER_TRACK,
         SPS_PARAMS_PER_TRACK},
    };

    std::array<uint8_t, SPS_PARAMS_PER_TRACK> params{};
    for (uint8_t i = 0; i < params.size(); ++i)
        params[i] = (i % 5 == 2) ? 255 : (uint8_t)(i + 40);

    MidiUartClass output;
    const bool wasConnected = MD.connected;
    const bool wasSpsx = MD.is_spsx;
    const uint32_t wasCaps = MD.fw_caps;
    MidiUartClass* const wasUart = MD.uart;
    bool ok = true;

    MD.connected = true;
    MD.is_spsx = true;
    MD.uart = &output;
    for (const Case& test : cases) {
        MD.fw_caps = FW_CAP_SPSX | test.caps;
        output.init();
        MD.activate_encoder_interface(params.data(), test.requestedParams);

        std::array<uint8_t, 64> packet{};
        const size_t packetBytes =
            output.desktop_egress(packet.data(), packet.size());
        const uint8_t maskBytes =
            (uint8_t)((test.expectedParams + 6) / 7);
        const size_t expectedBytes =
            (size_t)(10 + maskBytes + test.expectedParams);
        if (packetBytes != expectedBytes || packet[0] != 0xF0 ||
            packet[1] != 0x00 || packet[2] != 0x20 ||
            packet[3] != 0x3C || packet[4] != 0x02 ||
            packet[5] != 0x00 || packet[6] != 0x70 ||
            packet[7] != 0x36 || packet[8] != 0x01 ||
            packet[packetBytes - 1] != 0xF7) {
            ok = false;
            break;
        }

        for (uint8_t i = 0; i < test.expectedParams; ++i) {
            const bool expectedEnabled = params[i] != 255;
            const bool enabled =
                (packet[9 + i / 7] & (uint8_t)(1u << (i % 7))) != 0;
            const uint8_t value = packet[9 + maskBytes + i];
            if (enabled != expectedEnabled ||
                (expectedEnabled && value != params[i])) {
                ok = false;
                break;
            }
        }
        if (!ok)
            break;
    }

    MD.uart = wasUart;
    MD.fw_caps = wasCaps;
    MD.is_spsx = wasSpsx;
    MD.connected = wasConnected;
    return ok;
}

bool deliverSequencerFrame(uint8_t command, uint8_t tag,
                           const uint8_t* body, uint16_t bodyBytes) {
    std::array<uint8_t, 128> frame{};
    const uint16_t frameBytes = spsseq::spsSeqBuildFrame(
        command, tag, body, bodyBytes, frame.data(), frame.size());
    if (frameBytes == 0 ||
        MidiUart.desktop_ingress(frame.data(), frameBytes) != frameBytes) {
        return false;
    }
    Midi.processSysex();
    return true;
}

bool readHelloAck(uint8_t expectedVersion, uint8_t expectedParams) {
    std::array<uint8_t, 128> frame{};
    const size_t frameBytes =
        MidiUart.desktop_egress(frame.data(), frame.size());
    if (frameBytes < 2 || frame[0] != 0xF0 ||
        frame[frameBytes - 1] != 0xF7) {
        return false;
    }
    spsseq::Parsed parsed{};
    if (!spsseq::spsSeqParseFrame(frame.data() + 1,
                                  (uint16_t)(frameBytes - 2), parsed) ||
        parsed.cmd != spsseq::CMD_HELLO_ACK) {
        return false;
    }
    std::array<uint8_t, 16> body{};
    const uint16_t bodyBytes =
        spsseq::spsSeqDecodeBody(parsed, body.data(), body.size());
    return bodyBytes >= 6 && body[0] == expectedVersion &&
           body[5] == expectedParams;
}

bool readSequencerResponse(uint8_t expectedCommand,
                           std::array<uint8_t, spsseq::kMaxBodyRaw>& body,
                           uint16_t& bodyBytes) {
    std::array<uint8_t, 1200> frame{};
    const size_t frameBytes =
        MidiUart.desktop_egress(frame.data(), frame.size());
    if (frameBytes < 2 || frame[0] != 0xF0 ||
        frame[frameBytes - 1] != 0xF7) {
        return false;
    }
    spsseq::Parsed parsed{};
    if (!spsseq::spsSeqParseFrame(frame.data() + 1,
                                  (uint16_t)(frameBytes - 2), parsed) ||
        parsed.cmd != expectedCommand) {
        return false;
    }
    bodyBytes = spsseq::spsSeqDecodeBody(
        parsed, body.data(), (uint16_t)body.size());
    return bodyBytes <= body.size();
}

bool readSingleBusLockResponse(uint8_t expectedParam, uint8_t expectedValue) {
    std::array<uint8_t, spsseq::kMaxBodyRaw> body{};
    uint16_t bodyBytes = 0;
    if (!readSequencerResponse(spsseq::CMD_TRACK_LOCKS, body, bodyBytes) ||
        bodyBytes < 9 || body[0] != 0 || body[1] != 0x41 ||
        body[2] != 0 || body[3] != 1 || body[4] != 1 ||
        body[5] != expectedParam || body[6] != 1 || body[7] != 0 ||
        body[8] != 1 || bodyBytes != 10) {
        return false;
    }
    return body[9] == expectedValue;
}

bool checkSequencerNegotiation(uint8_t param, uint8_t value) {
    MidiUart.init();
    MidiSysex.rb->init();
    std::memset(MidiSysex.ledger, 0, sizeof(MidiSysex.ledger));
    MidiSysex.msg_wr = 0;
    MidiSysex.msg_rd = 0;
    MidiSysex.rd_cur = 0;
    MidiSysex.initSysexListeners();
    sps_host_seq_bridge.setup();

    SPSXSeqTrack& track = mcl_seq.spsx_tracks[0];
    track.clear_locks();
    const uint8_t capabilities[2] = {
        (uint8_t)(spsseq::CAP_SPSX & 0xFF),
        (uint8_t)(spsseq::CAP_SPSX >> 8),
    };

    const uint8_t helloV1[3] = {
        spsseq::kProtoVersionV1, capabilities[0], capabilities[1]};
    if (!deliverSequencerFrame(spsseq::CMD_HELLO, 1, helloV1,
                               sizeof(helloV1)) ||
        sps_host_seq_bridge.negotiatedProtoVersion() !=
            spsseq::kProtoVersionV1 ||
        sps_host_seq_bridge.negotiatedLockParams() !=
            spsseq::kNumLockParamsV1 ||
        !readHelloAck(spsseq::kProtoVersionV1,
                      spsseq::kNumLockParamsV1)) {
        return false;
    }

    const uint8_t v1BusLock[4] = {0, 0, param, value};
    if (!deliverSequencerFrame(spsseq::CMD_SET_LOCK, 2, v1BusLock,
                               sizeof(v1BusLock)) ||
        track.find_param(param) != 255) {
        return false;
    }
    uint8_t responseByte = 0;
    if (MidiUart.desktop_egress(&responseByte, 1) != 0)
        return false;

    const uint8_t helloV2[3] = {
        spsseq::kProtoVersion, capabilities[0], capabilities[1]};
    if (!deliverSequencerFrame(spsseq::CMD_HELLO, 3, helloV2,
                               sizeof(helloV2)) ||
        sps_host_seq_bridge.negotiatedProtoVersion() !=
            spsseq::kProtoVersion ||
        sps_host_seq_bridge.negotiatedLockParams() !=
            spsseq::kNumLockParams ||
        !readHelloAck(spsseq::kProtoVersion,
                      spsseq::kNumLockParams)) {
        return false;
    }

    // Exercise the exact host-facing transport and bridge surface used by
    // both an external SPS-X peer and the hosted WASM module: set BUS3 through
    // a framed command, then query TRACK_LOCKS and decode param 36 back from
    // the framed response.
    const uint8_t v2BusLock[4] = {0, 0, param, value};
    if (!deliverSequencerFrame(spsseq::CMD_SET_LOCK, 4, v2BusLock,
                               sizeof(v2BusLock))) {
        return false;
    }
    std::array<uint8_t, spsseq::kMaxBodyRaw> notification{};
    uint16_t notificationBytes = 0;
    if (!readSequencerResponse(spsseq::CMD_NOTIFY_DIRTY, notification,
                               notificationBytes) ||
        notificationBytes < 2 || notification[0] != 0 ||
        (notification[1] & (spsseq::DIRTY_LOCKS |
                            spsseq::DIRTY_SUMMARY)) == 0) {
        return false;
    }

    std::array<uint8_t, SPS_PARAMS_PER_TRACK> locks{};
    locks.fill(255);
    track.get_step_locks(0, locks.data(), false);
    const bool acceptedByV2 =
        track.find_param(param) != 255 &&
        locks[param] == value;
    const uint8_t trackRequest = 0;
    const bool roundTripped = acceptedByV2 &&
        deliverSequencerFrame(spsseq::CMD_REQ_TRACK_LOCKS, 5,
                              &trackRequest, 1) &&
        readSingleBusLockResponse(param, value);
    track.clear_locks();
    return roundTripped;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::strcmp(argv[1], "--roundtrip-kit") == 0)
        return roundtripKitFile(argv[2], argv[3]);
    const bool globalVersions = checkGlobalVersions();
    const bool machineRoutes = checkMachineRoutePackets();
    const bool kitV1 = checkKitWireCapability(
        0, SPSX_KIT_VERSION,
        mdKitWireSizeForVersion(SPSX_KIT_VERSION));
    const bool kitV2 = checkKitWireCapability(
        FW_CAP_SPSX_PARAMS_37, SPSX_KIT_VERSION,
        mdKitWireSizeForVersion(SPSX_KIT_VERSION));
    const bool patternStockFallback = checkPatternWireCapability(
        0, 0x03, false);
    const bool patternCompact = checkPatternWireCapability(
        FW_CAP_SPSX_PARAMS_37, MD_PATTERN_VERSION_SPSX, true);
    uint16_t maximumPatternBytes = 0;
    const bool maximumPattern =
        checkMaximumPatternRoundTrip(maximumPatternBytes);
    const bool malformedPattern =
        checkMalformedCanonicalPatternRejected();
    uint16_t envelopePatternBytes = 0;
    const bool envelopePattern =
        checkCanonicalPatternEnvelope(envelopePatternBytes);
    const bool trackRows = checkTrackRowMigrations();
    const bool extendedCc = checkNegotiatedExtendedCcMapping();
    const bool encoderPackets = checkEncoderInterfacePacketWidths();
    const bool seqNegotiation =
        checkSequencerNegotiation(MODEL_BUS3, 91) &&
        checkSequencerNegotiation(MODEL_ENVDEST, 2);
    if (!globalVersions || !machineRoutes || !kitV1 || !kitV2 || !patternStockFallback || !patternCompact ||
        !maximumPattern ||
        !malformedPattern || !envelopePattern || !trackRows ||
        !extendedCc || !encoderPackets || !seqNegotiation) {
        std::fprintf(stderr,
                     "MCL wire smoke failed: globals=%d machine-routes=%d kit65=%d kit65-caps=%d stock03=%d "
                     "compact04=%d max608=%d malformed=%d envelope=%d rows=%d cc=%d "
                     "encoder=%d seq=%d bytes=%u envelope-bytes=%u\n",
                     globalVersions, machineRoutes, kitV1, kitV2, patternStockFallback, patternCompact,
                     maximumPattern,
                     malformedPattern, envelopePattern, trackRows, extendedCc,
                     encoderPackets,
                     seqNegotiation, maximumPatternBytes,
                     envelopePatternBytes);
        return 1;
    }
    std::fprintf(stderr,
                 "MCL wire compatibility passed: kit/pattern, 24/34/38 "
                 "packets, row migrations, v1/v2 negotiation, BUS3/DST bridge "
                 "round-trip, malformed-body rejection, and 608 rows "
                 "(%u-byte dense MKII fixture)\n",
                 maximumPatternBytes);
    return 0;
}
