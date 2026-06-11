#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;
using namespace sps_host_arr_internal;

void SpsHostArrBridge::onReqActive(uint8_t tag) {
    uint8_t body[4 + spsarr::kActiveSlotBytes +
                 spsarr::kActiveReleasedMaskBytes +
                 spsarr::kActiveExtraGridSlotBytes +
                 spsarr::kActiveLoadSettingsBytes +
                 spsarr::kActiveSlotOwnershipBytes +
                 spsarr::kActiveSlotGroupIndexBytes +
                 spsarr::kActivePendingTransitionBytes +
                 spsarr::kActiveSlotSourceRowBytes +
                 spsarr::kActiveLoadQueueLengthBytes];
    body[0] = activeRowOrZero();
    body[1] = grid_task.next_active_row < GRID_LENGTH ? grid_task.next_active_row
                                                       : body[0];
    body[2] = grid_task.chain_behaviour ? 1 : 0;
    body[3] = MidiClock.isStarted() ? 1 : 0;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotBytes; slot++)
        body[4 + slot] = slot < NUM_SLOTS ? grid_page.active_slots[slot] : 255;
    spsArrPutU32(body + 4 + spsarr::kActiveSlotBytes,
                 mcl_arrangement.playbackReleasedMask());
    uint16_t extraOff =
        4 + spsarr::kActiveSlotBytes + spsarr::kActiveReleasedMaskBytes;
    for (uint8_t slot = 0; slot < spsarr::kActiveExtraGridSlotBytes; slot++) {
        uint8_t absoluteSlot = (uint8_t)(spsarr::kNumTracks + slot);
        body[extraOff + slot] =
            absoluteSlot < NUM_SLOTS ? grid_page.active_slots[absoluteSlot]
                                     : 255;
    }
    uint16_t settingsOff = extraOff + spsarr::kActiveExtraGridSlotBytes;
    body[settingsOff] = mcl_cfg.load_mode;
    body[settingsOff + 1] = mcl_cfg.chain_load_quant;
    uint32_t ownershipMask = 0;
    for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
        if (mcl_actions.get_grid_dev_track(slot) != nullptr)
            ownershipMask |= (uint32_t)(1ul << slot);
    }
    spsArrPutU32(body + settingsOff + spsarr::kActiveLoadSettingsBytes,
                 ownershipMask);
    uint16_t groupIndexOff = settingsOff + spsarr::kActiveLoadSettingsBytes +
                             spsarr::kActiveSlotOwnershipBytes;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotGroupIndexBytes; ++slot) {
        body[groupIndexOff + slot] =
            slot < NUM_SLOTS ? groupSelectIndexForSlot((GridSlot)slot) : 0xFF;
    }
    uint16_t pendingTransitionOff =
        groupIndexOff + spsarr::kActiveSlotGroupIndexBytes;
    spsArrPutU16(body + pendingTransitionOff, mcl_actions.next_transition);
    uint16_t sourceRowsOff =
        pendingTransitionOff + spsarr::kActivePendingTransitionBytes;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotSourceRowBytes; ++slot) {
        uint8_t row = 255;
        if (slot < NUM_SLOTS) {
            GridRow active = grid_page.active_slots[slot];
            if (active < GRID_LENGTH) {
                row = active;
            } else if ((active == SLOT_PENDING ||
                        active == SLOT_OFFSET_LOAD) &&
                       mcl_actions.links[slot].row < GRID_LENGTH) {
                row = mcl_actions.links[slot].row;
            }
        }
        body[sourceRowsOff + slot] = row;
    }
    uint16_t queueLengthOff =
        sourceRowsOff + spsarr::kActiveSlotSourceRowBytes;
    body[queueLengthOff] = mcl_cfg.chain_queue_length;
    sendFrame(CMD_ACTIVE, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqGridChain(uint8_t tag) {
    uint8_t body[spsarr::kGridChainRowBytes];
    for (uint8_t slot = 0; slot < spsarr::kGridChainRowBytes; ++slot) {
        uint8_t row = 255;
        if (slot < NUM_SLOTS && mcl_actions.chains[slot].is_mode_queue()) {
            GridRow activeRow = grid_page.active_slots[slot];
            GridChain& chain = mcl_actions.chains[slot];
            uint8_t count = chain.num_of_links;
            if (count > NUM_LINKS)
                count = NUM_LINKS;
            for (uint8_t i = 0; i < count; ++i) {
                GridRow chainRow = chain.rows[i];
                if (chainRow >= GRID_LENGTH)
                    continue;
                if (activeRow < GRID_LENGTH && activeRow == chainRow)
                    continue;
                row = chainRow;
                break;
            }
        }
        body[slot] = row;
    }
    sendFrame(CMD_GRID_CHAIN, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqCells(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (n < 5)
        return;
    uint8_t startRow = b[0];
    uint8_t rowCount = b[1];
    uint16_t trackMask = spsArrGetU16(b + 2);
    uint8_t flags = b[4];
    GridIndex gridBank = n >= 6 ? sanitizeGridBank(b[5]) : (GridIndex)0;
    bool sendLabels = (flags & REQ_CELL_LABELS) != 0;
    bool sendRowNames = (flags & REQ_ROW_NAMES) != 0;

    if (startRow >= GRID_LENGTH || rowCount == 0)
        return;
    uint8_t maxRows = sendRowNames ? (uint8_t)spsarr::kRowsPerNamedCellPage
                                   : (sendLabels
                                          ? (uint8_t)spsarr::kRowsPerLabelCellPage
                                          : (uint8_t)spsarr::kRowsPerCellPage);
    if (rowCount > maxRows)
        rowCount = maxRows;
    if ((int)startRow + (int)rowCount > GRID_LENGTH)
        rowCount = (uint8_t)(GRID_LENGTH - startRow);

    for (uint8_t track = 0; track < spsarr::kNumTracks; track++) {
        if (((trackMask >> track) & 1u) == 0)
            continue;

        uint8_t body[kMaxBodyRaw];
        uint16_t off = 0;
        body[off++] = track;
        body[off++] = startRow;
        body[off++] = rowCount;
        body[off++] = (uint8_t)((sendLabels ? CELL_FORMAT_LABELS : 0) |
                                (sendRowNames ? CELL_FORMAT_ROW_NAMES : 0) |
                                CELL_FORMAT_DEPENDENCIES |
                                CELL_FORMAT_GROUP_INDEX |
                                (n >= 6 ? CELL_FORMAT_GRID_BANK : 0));
        if (n >= 6)
            body[off++] = gridBank;

        uint16_t recordBytes = (uint16_t)(kCellRecordBaseBytes +
            kCellRecordDependencyBytes +
            kCellRecordGroupIndexBytes +
            (sendLabels ? kCellRecordLabelBytes : 0) +
            (sendRowNames ? kCellRecordRowNameBytes : 0));
        for (uint8_t i = 0; i < rowCount && off + recordBytes <= kMaxBodyRaw; i++) {
            ArrCell cell = readCell(track, (GridRow)(startRow + i), gridBank);
            uint8_t cellFlags = 0;
            if (cell.ok && cell.active)
                cellFlags |= CELL_ACTIVE;
            if (cell.ok && cell.loadSound)
                cellFlags |= CELL_LOAD_SOUND;
            if (cell.hasFade)
                cellFlags |= CELL_FADE;

            body[off++] = cellFlags;
            body[off++] = cell.ok ? cell.link.row : 0;
            body[off++] = cell.ok ? cell.link.loops : 0;
            body[off++] = cell.ok ? cell.link.length : 16;
            body[off++] = cell.ok ? cell.link.speed_value() : 0;
            spsArrPutU32(body + off, cell.ok ? cell.durationQ12 : 0);
            off = (uint16_t)(off + 4);
            body[off++] = cell.hasFade ? cell.fade.flags : 0;
            body[off++] = cell.hasFade ? cell.fade.target
                                       : TRACK_LOAD_FADE_TARGET_DEFAULT;
            spsArrPutU16(body + off, cell.hasFade ? cell.fade.duration_q12 : 0);
            off = (uint16_t)(off + 2);
            body[off++] = cell.hasFade ? cell.fade.amount : 0;
            body[off++] = cell.hasFade ? (uint8_t)cell.fade.curve : 0;
            spsArrPutU16(body + off, cell.ok ? cell.dependencyMask : 0);
            off = (uint16_t)(off + kCellRecordDependencyBytes);
            body[off++] = cell.ok ? cell.groupIndex : 0xFF;
            if (sendLabels) {
                body[off++] = (uint8_t)cell.label2[0];
                body[off++] = (uint8_t)cell.label2[1];
                body[off++] = (uint8_t)cell.label4[0];
                body[off++] = (uint8_t)cell.label4[1];
                body[off++] = (uint8_t)cell.label4[2];
                body[off++] = (uint8_t)cell.label4[3];
            }
            if (sendRowNames) {
                copyRowName((GridRow)(startRow + i), gridBank, body + off);
                off = (uint16_t)(off + kCellRecordRowNameBytes);
            }
        }

        sendFrame(CMD_CELLS, tag, body, off);
    }
}

#endif  // MCL_FEATURE_HOST_ARRANGER
