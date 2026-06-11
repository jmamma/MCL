#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;
using namespace sps_host_arr_internal;

void SpsHostArrBridge::onGridCopy(uint8_t tag, const uint8_t* b,
                                  uint16_t n) {
    GridSlot col = 0;
    GridRow row = 0;
    GridSpan w = 0;
    GridSpan h = 0;
    if (!parseGridRect(b, n, col, row, w, h)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_clipboard.copy(col, row, w, h)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    grid_page.slot_undo = 0;
    uint8_t ack[2] = {CMD_GRID_COPY, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

void SpsHostArrBridge::onGridClear(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    GridSlot col = 0;
    GridRow row = 0;
    GridSpan w = 0;
    GridSpan h = 0;
    if (!parseGridRect(b, n, col, row, w, h)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_clipboard.copy(col, row, w, h) || !clearGridRect(col, row, w, h)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    uint8_t ack[2] = {CMD_GRID_CLEAR, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

void SpsHostArrBridge::onGridPaste(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    GridSlot col = 0;
    GridRow row = 0;
    GridSpan w = 0;
    GridSpan h = 0;
    if (!parseGridRect(b, n, col, row, w, h)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_clipboard.paste(col, row)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    grid_page.slot_undo = 0;
    proj.sync_grid();
    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_GRID_PASTE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

void SpsHostArrBridge::onGridApplySlotEdit(uint8_t tag, const uint8_t* b,
                                           uint16_t n) {
    if (n < 11) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    GridSlot sourceCol = b[0];
    GridRow sourceRow = b[1];
    GridSlot targetCol = b[2];
    GridRow targetRow = b[3];
    GridSpan width = b[4];
    GridSpan height = b[5];
    uint8_t fields = b[6] & (GRID_SLOT_APPLY_ROW |
                             GRID_SLOT_APPLY_LOOPS |
                             GRID_SLOT_APPLY_LENGTH |
                             GRID_SLOT_APPLY_LOAD_SOUND);
    GridIndex sourceGridBank =
        n >= 13 ? sanitizeGridBank(b[11]) : (GridIndex)0;
    GridIndex targetGridBank =
        n >= 13 ? sanitizeGridBank(b[12]) : (GridIndex)0;
    sourceCol = visibleSlotToGridSlot(sourceCol, sourceGridBank);
    targetCol = visibleSlotToGridSlot(targetCol, targetGridBank);
    if (sourceCol >= NUM_SLOTS || sourceRow >= GRID_LENGTH ||
        targetCol >= NUM_SLOTS || targetRow >= GRID_LENGTH ||
        width == 0 || height == 0 || fields == 0) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }
    GridSpan targetBankRemaining =
        (GridSpan)(GRID_WIDTH - (targetCol % GRID_WIDTH));
    if (width > targetBankRemaining)
        width = targetBankRemaining;
    if ((uint16_t)targetRow + height > GRID_LENGTH)
        height = (GridSpan)(GRID_LENGTH - targetRow);

    EmptyTrack sourceScratch;
    DeviceTrack* source = sourceScratch.load_from_grid_512(sourceCol,
                                                           sourceRow);
    if (!source || !source->is_active()) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }

    GridLink edited = source->link;
    bool editedLoadSound = source->load_sound();
    if ((fields & GRID_SLOT_APPLY_ROW) != 0)
        edited.row = b[7] < GRID_LENGTH ? b[7] : 0;
    if ((fields & GRID_SLOT_APPLY_LOOPS) != 0)
        edited.loops = b[8] & 0x7F;
    if ((fields & GRID_SLOT_APPLY_LENGTH) != 0) {
        edited.length = b[9] & 0x7F;
        if (edited.length == 0)
            edited.length = 1;
    }
    if ((fields & GRID_SLOT_APPLY_LOAD_SOUND) != 0)
        editedLoadSound = b[10] != 0;

    bool changedRow = (fields & GRID_SLOT_APPLY_ROW) != 0;
    bool changedLoops = (fields & GRID_SLOT_APPLY_LOOPS) != 0;
    bool changedLength = (fields & GRID_SLOT_APPLY_LENGTH) != 0;
    bool changedLoadSound = (fields & GRID_SLOT_APPLY_LOAD_SOUND) != 0;
    bool anyStored = false;

    for (GridSpan y = 0; y < height && (uint16_t)targetRow + y < GRID_LENGTH;
         y++) {
        GridRow row = (GridRow)(targetRow + y);
        bool rowStored = false;
        for (GridSpan x = 0;
             x < width && (uint16_t)targetCol + x < NUM_SLOTS;
             x++) {
            GridSlot col = (GridSlot)(targetCol + x);
            EmptyTrack scratch;
            DeviceTrack* track = scratch.load_from_grid_512(col, row);
            if (!track || !track->is_active())
                continue;

            bool storeSlot = false;
            if (col == sourceCol && row == sourceRow) {
                track->link = edited;
                track->set_load_sound(editedLoadSound);
                storeSlot = true;
            } else {
                GridLink link = track->link;
                if (changedLoadSound) {
                    track->set_load_sound(editedLoadSound);
                    storeSlot = true;
                }
                if (changedLoops && edited.loops == 0) {
                    link.loops = 0;
                    storeSlot = true;
                } else if (changedLoops || changedLength) {
                    if (changedLoops && changedLength) {
                        link.loops = edited.loops;
                        link.length = edited.length;
                        storeSlot = true;
                    } else if (changedLoops) {
                        uint16_t slotLength =
                            (uint16_t)link.length *
                            SeqTrack::get_speed_multiplier_int(
                                link.speed_value()) /
                            12;
                        if (slotLength) {
                            uint16_t targetLength =
                                (uint32_t)edited.length *
                                SeqTrack::get_speed_multiplier_int(
                                    edited.speed_value()) *
                                edited.loops /
                                12;
                            if (!(targetLength % slotLength) &&
                                slotLength <= targetLength) {
                                link.loops = targetLength / slotLength;
                            } else {
                                link.loops = edited.loops;
                            }
                            storeSlot = true;
                        }
                    } else if (changedLength &&
                               link.speed_value() == edited.speed_value()) {
                        link.length = edited.length;
                        storeSlot = true;
                    }
                }
                if (changedRow) {
                    link.row = edited.row;
                    storeSlot = true;
                }
                if (storeSlot)
                    track->link = link;
            }

            if (storeSlot &&
                track->write_grid(track->_this(), track->get_track_size(),
                                  col, row)) {
                rowStored = true;
                anyStored = true;
            }
        }

        if (rowStored) {
            GridRowHeader header;
            GridIndex grid = (GridIndex)(targetCol / GRID_WIDTH);
            if (proj.read_grid_row_header(&header, row, grid)) {
                header.active = true;
                header.name[0] = '\0';
                proj.write_grid_row_header(&header, row, grid);
            }
        }
    }

    if (!anyStored) {
        sendErr(tag, ERR_BUSY, 1);
        return;
    }

    proj.sync_grid((GridIndex)(targetCol / GRID_WIDTH));
    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_GRID_APPLY_SLOT, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

void SpsHostArrBridge::onSetRowName(uint8_t tag, const uint8_t* b,
                                    uint16_t n) {
    if (n < 1 + spsarr::kRowNameBytes) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    GridRow row = b[0];
    if (row >= GRID_LENGTH) {
        sendErr(tag, ERR_RANGE, row);
        return;
    }

    char name[17] = {};
    uint8_t copyLen = (uint8_t)sizeof(name) - 1;
    if (copyLen > spsarr::kRowNameBytes)
        copyLen = spsarr::kRowNameBytes;
    for (uint8_t i = 0; i < copyLen; i++) {
        uint8_t c = b[1 + i];
        if (c == 0)
            break;
        name[i] = labelChar((char)c);
    }
    for (int i = copyLen - 1; i >= 0 && name[i] == ' '; i--)
        name[i] = '\0';

    GridRowHeader rowHeaders[NUM_GRIDS];
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        if (!proj.read_grid_row_header(&rowHeaders[grid], row, grid)) {
            sendErr(tag, ERR_RANGE, grid);
            return;
        }
    }
    if (!rowHeaders[0].active) {
        sendErr(tag, ERR_RANGE, row);
        return;
    }

    bool ok = true;
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        strncpy(rowHeaders[grid].name, name, sizeof(rowHeaders[grid].name) - 1);
        rowHeaders[grid].name[sizeof(rowHeaders[grid].name) - 1] = '\0';
        if (!proj.write_grid_row_header(&rowHeaders[grid], row, grid))
            ok = false;
        if (!proj.sync_grid(grid))
            ok = false;
    }
    if (!ok) {
        sendErr(tag, ERR_BUSY, row);
        return;
    }

    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_SET_ROW_NAME, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

#endif  // MCL_FEATURE_HOST_ARRANGER
