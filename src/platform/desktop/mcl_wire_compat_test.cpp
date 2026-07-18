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
    ElektronDataToSysexEncoder encoder(message.data());
    const uint16_t length = MD.kit.toSysex(&encoder);
    auto fixture = std::make_unique<SysexFixture>();
    if (length != expectedSize || message[7] != expectedVersion ||
        !publishToMidiSysex(fixture->sysex, message.data(), length)) {
        return false;
    }
    MDKit decoded;
    return decoded.fromSysex(&fixture->midi);
}

bool checkPatternWireCapability(uint32_t caps, uint8_t expectedVersion,
                                bool expectBusLock) {
    std::array<uint8_t, 16384> message{};
    MD.is_spsx = true;
    MD.fw_caps = FW_CAP_SPSX | caps;
    MD.pattern.clearPattern();
    MD.pattern.patternLength = 64;

    // Param 33 must survive both SPS-X wire versions. BUS1-3 exercise four
    // channel groups and both halves of a 64-step pattern.
    if (!MD.pattern.addLock(0, 0, MODEL_RENV, 55))
        return false;
    MD.pattern.ext_locks_params[0][MODEL_RENV] = MODEL_RENV + 1;
    constexpr uint8_t tracks[] = {0, 3, 4, 15};
    constexpr uint8_t params[] = {MODEL_BUS1, MODEL_BUS2, MODEL_BUS3};
    for (uint8_t track : tracks) {
        MD.pattern.trigPatterns[track] =
            (uint64_t(1) << 5) | (uint64_t(1) << 45);
        for (uint8_t param : params) {
            MD.pattern.ext_locks_params[track][param] = param + 1;
            if (!MD.pattern.addLock(track, 5, param,
                                    (uint8_t)(20 + track + param)) ||
                !MD.pattern.addLock(track, 45, param,
                                    (uint8_t)(80 + track + param))) {
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
        return false;
    }

    MDPattern decoded;
    if (!decoded.fromSysex(&fixture->midi) ||
        decoded.version != expectedVersion)
        return false;
    const int16_t legacyRow = decoded.paramLocks[0][MODEL_RENV];
    const bool hasLegacyLock =
        legacyRow >= 0 && decoded.lock_row((uint16_t)legacyRow)[0] == 55;
    if (!hasLegacyLock ||
        mdPatternLockSlotCountForVersion(decoded.version) !=
            (expectBusLock ? MD_PATTERN_LOCK_SLOTS
                           : MD_PATTERN_LOCK_SLOTS_V1)) {
        return false;
    }
    for (uint8_t track : tracks) {
        for (uint8_t param : params) {
            const int16_t row = decoded.paramLocks[track][param];
            const bool present = row >= 0;
            if (present != expectBusLock)
                return false;
            if (present &&
                (decoded.lock_row((uint16_t)row)[5] !=
                     (int8_t)(20 + track + param) ||
                 decoded.lock_row((uint16_t)row)[45] !=
                     (int8_t)(80 + track + param))) {
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

    if (MD.pattern.getLockIdx(15, MODEL_BUS3) != MAX_LOCK_ROWS - 1 ||
        editingApi->getNextEmptyLock() != -1 ||
        editingApi->addLock(16, 0, 0, 1)) {
        return false;
    }

    // Exercise clearing/reallocation through the generic API on the final
    // extended row, not just direct wire-array access.
    for (uint8_t step = 0; step < 64; ++step)
        editingApi->clearLock(15, step, MODEL_BUS3);
    if (MD.pattern.getLockIdx(15, MODEL_BUS3) != -1)
        return false;
    for (uint8_t step = 0; step < 64; ++step) {
        if (!editingApi->addLock(
                15, step, MODEL_BUS3,
                stressLockValue(15, MODEL_BUS3, step))) {
            return false;
        }
    }

    ElektronDataToSysexEncoder encoder(message.data());
    encodedLength = MD.pattern.toSysex(&encoder);
    if (encodedLength <= 16383 || encodedLength >= message.size() ||
        message[7] != MD_PATTERN_VERSION_SPSX) {
        return false;
    }

    auto fixture = std::make_unique<SysexFixture>();
    if (!publishToMidiSysex(fixture->sysex, message.data(), encodedLength) ||
        fixture->sysex.get_recordLen() != encodedLength - 2) {
        return false;
    }

    auto decoded = std::make_unique<MDPattern>();
    if (!decoded->fromSysex(&fixture->midi) ||
        decoded->version != MD_PATTERN_VERSION_SPSX ||
        decoded->numRows != MAX_LOCK_ROWS) {
        return false;
    }
    for (uint8_t track = 0; track < 16; ++track) {
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

#pragma pack(push, 1)
struct LegacySpsMachine34 {
    uint8_t params[SPS_PARAMS_V1_PER_TRACK];
    uint8_t track;
    uint8_t level;
    uint32_t model;
    MDLFO lfos[2];
    uint8_t trigGroup;
    uint8_t muteGroup;
};

struct LegacySpsxTrackV5Payload {
    LegacySpsMachine34 machine;
    uint8_t seqVersion;
    StepSeqTrackData seqData;
    SeqTrackModStorage mod;
    TrackLoadFadeData fade;
};
#pragma pack(pop)

bool checkTrackRowMigrations() {
    static_assert(sizeof(LegacySpsMachine34) + 3 == sizeof(SPSMachine));
    static_assert(sizeof(StepSeqTrackData) + 3 == sizeof(SPSXSeqTrackData));
    static_assert(sizeof(LegacySpsxTrackV5Payload) + 6 ==
                  sizeof(SPSMachine) + sizeof(SPSXTrackStorage) +
                      sizeof(TrackLoadFadeData));

    // Track-local versions are authoritative within the current project/grid
    // format; the feature deliberately does not bump PROJ_VERSION.
    proj.version = PROJ_VERSION;

    LegacySpsxTrackV5Payload old{};
    for (uint8_t param = 0; param < SPS_PARAMS_V1_PER_TRACK; ++param)
        old.machine.params[param] = (uint8_t)(param + 11);
    old.machine.track = 7;
    old.machine.level = 93;
    old.machine.model = 0x20041;
    old.machine.lfos[0].destinationParam = 21;
    old.machine.lfos[1].destinationParam = 28;
    old.machine.trigGroup = 3;
    old.machine.muteGroup = 9;
    old.seqVersion = SPSX_SEQ_VERSION_SPSX;
    old.seqData.init();
    old.seqData.swing_amount = 19;
    old.seqData.locks_params[SPS_PARAMS_V1_PER_TRACK - 1] =
        SPS_PARAMS_V1_PER_TRACK;
    old.seqData.steps[7].locks =
        uint64_t{1} << (SPS_PARAMS_V1_PER_TRACK - 1);
    old.seqData.locks[0] = 88;
    old.mod.init_mod();
    old.fade.init();

    SPSXTrack migrated;
    migrated.version = SPSX_TRACK_LOCK37_STORAGE_VERSION - 1;
    std::memcpy(&migrated.machine, &old, sizeof(old));
    migrated.on_storage_loaded();
    if (migrated.version != SPSX_TRACK_LOCK37_STORAGE_VERSION ||
        migrated.machine.track != old.machine.track ||
        migrated.machine.level != old.machine.level ||
        migrated.machine.model != old.machine.model ||
        migrated.machine.lfos[0].destinationParam != 21 ||
        migrated.machine.lfos[1].destinationParam != 28 ||
        migrated.machine.trigGroup != old.machine.trigGroup ||
        migrated.machine.muteGroup != old.machine.muteGroup ||
        migrated.machine.params[MODEL_BUS1] != 0 ||
        migrated.machine.params[MODEL_BUS2] != 0 ||
        migrated.machine.params[MODEL_BUS3] != 0 ||
        migrated.seq_storage.seq_version != SPSX_SEQ_VERSION_SPSX ||
        migrated.seq_storage.seq_data.spsx.swing_amount != 19 ||
        migrated.seq_storage.seq_data.spsx.find_param(
            SPS_PARAMS_V1_PER_TRACK - 1) !=
            SPS_PARAMS_V1_PER_TRACK - 1 ||
        migrated.seq_storage.seq_data.spsx.get_lockidx(
            7, SPS_PARAMS_V1_PER_TRACK - 1) != 0 ||
        migrated.seq_storage.seq_data.spsx.locks[0] != 88) {
        std::fprintf(stderr,
                     "row migration diag: ver=%u trk=%u lev=%u model=%lx "
                     "lfo=%u/%u grp=%u/%u bus=%u/%u/%u seq=%u swing=%u "
                     "slot=%u idx=%u val=%u\n",
                     migrated.version, migrated.machine.track,
                     migrated.machine.level,
                     (unsigned long)migrated.machine.model,
                     migrated.machine.lfos[0].destinationParam,
                     migrated.machine.lfos[1].destinationParam,
                     migrated.machine.trigGroup, migrated.machine.muteGroup,
                     migrated.machine.params[MODEL_BUS1],
                     migrated.machine.params[MODEL_BUS2],
                     migrated.machine.params[MODEL_BUS3],
                     migrated.seq_storage.seq_version,
                     migrated.seq_storage.seq_data.spsx.swing_amount,
                     migrated.seq_storage.seq_data.spsx.find_param(
                         SPS_PARAMS_V1_PER_TRACK - 1),
                     migrated.seq_storage.seq_data.spsx.get_lockidx(
                         7, SPS_PARAMS_V1_PER_TRACK - 1),
                     migrated.seq_storage.seq_data.spsx.locks[0]);
        return false;
    }
    for (uint8_t param = 0; param < SPS_PARAMS_V1_PER_TRACK; ++param) {
        if (migrated.machine.params[param] != old.machine.params[param]) {
            std::fprintf(stderr, "row migration param %u: %u != %u\n",
                         param, migrated.machine.params[param],
                         old.machine.params[param]);
            return false;
        }
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
        const uint8_t wireParams = caps ? SPS_PARAMS_PER_TRACK
                                        : SPS_PARAMS_V1_PER_TRACK;
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
        {0, SPS_PARAMS_PER_TRACK, SPS_PARAMS_V1_PER_TRACK},
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

bool checkSequencerNegotiationRejectsV1BusLocks() {
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

    const uint8_t v1BusLock[4] = {0, 0, MODEL_BUS1, 77};
    if (!deliverSequencerFrame(spsseq::CMD_SET_LOCK, 2, v1BusLock,
                               sizeof(v1BusLock)) ||
        track.find_param(MODEL_BUS1) != 255) {
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
    constexpr uint8_t kBus3Value = 91;
    const uint8_t v2BusLock[4] = {0, 0, MODEL_BUS3, kBus3Value};
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
        track.find_param(MODEL_BUS3) != 255 &&
        locks[MODEL_BUS3] == kBus3Value;
    const uint8_t trackRequest = 0;
    const bool roundTripped = acceptedByV2 &&
        deliverSequencerFrame(spsseq::CMD_REQ_TRACK_LOCKS, 5,
                              &trackRequest, 1) &&
        readSingleBusLockResponse(MODEL_BUS3, kBus3Value);
    track.clear_locks();
    return roundTripped;
}

} // namespace

int main() {
    const bool kitV1 = checkKitWireCapability(
        0, SPSX_KIT_VERSION_V1,
        mdKitWireSizeForVersion(SPSX_KIT_VERSION_V1));
    const bool kitV2 = checkKitWireCapability(
        FW_CAP_SPSX_PARAMS_37, SPSX_KIT_VERSION,
        mdKitWireSizeForVersion(SPSX_KIT_VERSION));
    const bool patternV1 = checkPatternWireCapability(
        0, MD_PATTERN_VERSION_SPSX_V1, false);
    const bool patternV2 = checkPatternWireCapability(
        FW_CAP_SPSX_PARAMS_37, MD_PATTERN_VERSION_SPSX, true);
    uint16_t maximumPatternBytes = 0;
    const bool maximumPattern =
        checkMaximumPatternRoundTrip(maximumPatternBytes);
    const bool trackRows = checkTrackRowMigrations();
    const bool extendedCc = checkNegotiatedExtendedCcMapping();
    const bool encoderPackets = checkEncoderInterfacePacketWidths();
    const bool seqNegotiation =
        checkSequencerNegotiationRejectsV1BusLocks();
    if (!kitV1 || !kitV2 || !patternV1 || !patternV2 || !maximumPattern ||
        !trackRows ||
        !extendedCc || !encoderPackets || !seqNegotiation) {
        std::fprintf(stderr,
                     "MCL wire smoke failed: kit65=%d kit66=%d pat40=%d "
                     "pat41=%d max592=%d rows=%d cc=%d encoder=%d seq=%d "
                     "bytes=%u\n",
                     kitV1, kitV2, patternV1, patternV2, maximumPattern,
                     trackRows, extendedCc, encoderPackets, seqNegotiation,
                     maximumPatternBytes);
        return 1;
    }
    std::fprintf(stderr,
                 "MCL wire compatibility passed: kit/pattern, 24/34/37 "
                 "packets, row migrations, v1/v2 negotiation, BUS3 bridge "
                 "round-trip, and 592 rows (%u-byte worst-case fixture)\n",
                 maximumPatternBytes);
    return 0;
}
