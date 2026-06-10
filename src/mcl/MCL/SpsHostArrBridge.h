/**
 * SpsHostArrBridge - MCL side of the SPS arranger cell protocol.
 */
#ifndef SPS_HOST_ARR_BRIDGE_H
#define SPS_HOST_ARR_BRIDGE_H

#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "MidiSysex.h"
#include "SpsArrProtocol.h"

class SpsHostArrBridge : public MidiSysexListenerClass {
public:
    SpsHostArrBridge()
        : MidiSysexListenerClass(nullptr, spsarr::kMfrId, spsarr::kSubId0,
                                 spsarr::kSubId1) {}

    void setup();
    void end() override;

    void notifyDirty(int track, uint8_t regions);
    void notifyArrangementPosition(uint32_t positionQ12, uint8_t flags);

private:
    bool ready_ = false;

    void handle(const spsarr::Parsed& p, const uint8_t* b, uint16_t n);
    void sendFrame(uint8_t cmd, uint8_t tag, const uint8_t* body,
                   uint16_t bodyLen);
    void sendErr(uint8_t tag, uint8_t code, uint8_t detail);

    void onHello(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqActive(uint8_t tag);
    void onReqGridChain(uint8_t tag);
    void onReqCells(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqArrMeta(uint8_t tag);
    void onReqArrClips(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqArrMarkers(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqArrLoopRegions(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqArrTrackLabels(uint8_t tag);
    void onReqProjectList(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqProjectVersions(uint8_t tag, const uint8_t* b, uint16_t n);
    void onLoadSlots(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSaveSlots(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrClear(uint8_t tag);
    void onArrImportGrid(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrSelect(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrNew(uint8_t tag);
    void onArrSave(uint8_t tag);
    void onGridCopy(uint8_t tag, const uint8_t* b, uint16_t n);
    void onGridClear(uint8_t tag, const uint8_t* b, uint16_t n);
    void onGridPaste(uint8_t tag, const uint8_t* b, uint16_t n);
#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
    void onGridMove(uint8_t tag, const uint8_t* b, uint16_t n);
    void onGridUndo(uint8_t tag);
#endif
    void onGridApplySlotEdit(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSetRowName(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSetArrMarker(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSetArrLoopRegion(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSetArrTrackLabel(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSetArrClipFade(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrSeekLoad(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrMakeLocal(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrLocalToGrid(uint8_t tag, const uint8_t* b, uint16_t n);
    void onArrSetLoop(uint8_t tag, const uint8_t* b, uint16_t n);
    void onSetLoadSettings(uint8_t tag, const uint8_t* b, uint16_t n);
    void onProjectOp(uint8_t tag, const uint8_t* b, uint16_t n);
    void onProjectVersionOp(uint8_t tag, const uint8_t* b, uint16_t n);

    bool applySetLink(const uint8_t* b, uint16_t n);
    bool applySetFade(const uint8_t* b, uint16_t n);
};

extern SpsHostArrBridge sps_host_arr_bridge;

#endif  // MCL_FEATURE_HOST_ARRANGER
#endif  // SPS_HOST_ARR_BRIDGE_H
