#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Host/SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;
using namespace sps_host_arr_internal;

#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
void SpsHostArrBridge::onGridMove(uint8_t tag, const uint8_t* b,
                                  uint16_t n) {
    GridMoveRequest req;
    if (!parseGridMove(b, n, req)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!applyGridMove(req)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    uint8_t ack[2] = {CMD_GRID_MOVE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

void SpsHostArrBridge::onGridUndo(uint8_t tag) {
    uint16_t trackMask = 0;
    if (!restoreLatestSpsGridJournalTransaction(trackMask)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    uint8_t ack[2] = {CMD_GRID_UNDO, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    for (uint8_t track = 0; track < GRID_WIDTH; track++) {
        if ((trackMask & (uint16_t)(1u << track)) != 0)
            notifyDirty(track, DIRTY_CELLS);
    }
}
#endif

#endif  // MCL_FEATURE_HOST_ARRANGER
