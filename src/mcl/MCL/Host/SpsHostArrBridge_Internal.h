#pragma once

#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Host/SpsHostArrBridge.h"

#include "DeviceTrack.h"
#include "EmptyTrack.h"
#include "GUI/Pages/Grid/GridPages.h"
#include "GridTask.h"
#include "MCLActions.h"
#include "MCLClipBoard.h"
#include "Sequencer/MCLSeq.h"
#include "MCLSd.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiSysex.h"
#include "MidiUart.h"
#include "SeqTrack.h"
#include "Arrangement/MCLArrangement.h"
#include "Project.h"
#include "TrackLoadFade.h"
#include "DeviceManager.h"
#include "platform.h"
#include "../../Drivers/MD/MD.h"
#include "MDTrack.h"
#include "SPSXTrack.h"
#include "../../Drivers/MD/MDParams.h"
#include "../../Drivers/MNM/MNMParams.h"

#include <string.h>

#if defined(PLATFORM_WASM) && defined(DEBUGMODE)
#define ARR_FADE_TRACE(fmt, ...) DEBUG_PRINT_FN("[arr-fade] " fmt, ##__VA_ARGS__)
#else
#define ARR_FADE_TRACE(fmt, ...)
#endif

namespace sps_host_arr_internal {

constexpr uint16_t kCurrentPatternTimeoutMs = 500;

struct ArrCell {
    bool ok = false;
    bool active = false;
    bool loadSound = true;
    GridLink link;
    uint32_t durationQ12 = 0;
    TrackLoadFadeData fade;
    bool hasFade = false;
    uint16_t dependencyMask = 0;
    uint8_t groupIndex = 0xFF;
    char label2[3] = {'-', '-', '\0'};
    char label4[5] = {'-', '-', ' ', ' ', '\0'};
};

struct GridMoveRequest {
    GridSlot sourceCol = 0;
    GridRow sourceRow = 0;
    GridSpan width = 0;
    GridSpan height = 0;
    GridSlot targetCol = 0;
    GridRow targetRow = 0;
    bool sparse = false;
    uint16_t rowMasks[GRID_LENGTH] = {};
};

uint32_t linkDurationQ12(const GridLink& link);
char labelChar(char c);
void setLabel2(char label[3], char a, char b);
void copyLabelPair(const char* src, char* dst);
GridIndex sanitizeGridBank(uint8_t gridBank);
GridSlot visibleSlotToGridSlot(uint8_t visibleSlot, GridIndex gridBank);
void copyRowName(GridRow row, GridIndex gridBank, uint8_t* dst);
const char* shortNamePart(uint8_t trackType, uint8_t model,
                                 uint8_t part);
void populateCellLabels(ArrCell& cell, DeviceTrack* tr,
                               GridSlot slot, GridRow row);
void addDependencyTrack(uint16_t& mask, uint8_t track);
uint16_t directCellDependencyMask(DeviceTrack* tr);
uint16_t cellDependencyMask(uint8_t track, GridRow row,
                                   GridIndex gridBank,
                                   DeviceTrack* tr);
uint8_t groupSelectIndexForSlot(GridSlot slot);
uint8_t hostLoadQueueMode(uint8_t mode, uint8_t flags);
uint32_t hostLoadDestinationMask(const GridRow rowSelect[NUM_SLOTS],
                                        GridSlot loadOffset);
void releaseHostLoadedArrangementTracks(
    uint8_t mode, const GridRow rowSelect[NUM_SLOTS], GridSlot loadOffset);
ArrCell readCell(uint8_t track, GridRow row, GridIndex gridBank);
bool writeCellLink(uint8_t track, GridRow row, const GridLink& link,
                          bool loadSound);
bool writeCellFade(uint8_t track, GridRow row,
                          const TrackLoadFadeData& fade);
GridRow activeRowOrZero();
void putArrClip(uint8_t* dst, const mclarrfile::Clip& clip);
void putArrMarker(uint8_t* dst, const mclarrfile::Marker& marker);
void putArrLoopRegion(uint8_t* dst,
                             const mclarrfile::LoopRegion& region);
bool parseGridRect(const uint8_t* b, uint16_t n, GridSlot& col,
                          GridRow& row, GridSpan& w, GridSpan& h);
uint16_t gridMoveWidthMask(GridSpan width);
bool parseGridMove(const uint8_t* b, uint16_t n,
                          GridMoveRequest& req);
bool clearGridRect(GridSlot col, GridRow row, GridSpan w, GridSpan h);
uint8_t spsGridJournalPrev(uint8_t index);
void resetSpsGridJournalTransaction(uint8_t tx);
bool openSpsGridJournalFiles(uint8_t tx);
bool closeSpsGridJournalFiles(uint8_t tx);
bool beginSpsGridJournalTransaction(uint8_t& tx);
bool commitSpsGridJournalTransaction(uint8_t tx);
void discardSpsGridJournalTransaction(uint8_t tx);
bool snapshotSpsGridJournalCell(uint8_t tx, GridSlot slot,
                                       GridRow row);
bool prepareSpsGridMoveJournal(const GridMoveRequest& req,
                                      uint8_t& tx);
bool restoreSpsGridJournalTransaction(uint8_t tx);
uint16_t spsGridJournalLocalTrackMask(uint8_t tx);
bool restoreLatestSpsGridJournalTransaction(uint16_t& trackMask);
bool cleanupGridRowIfEmpty(GridIndex grid, GridRow row);
bool copyGridMoveCell(GridSlot sourceSlot, GridRow sourceRow,
                             GridSlot targetSlot, GridRow targetRow,
                             bool destinationSame);
bool moveSourceCellIsDestination(const GridMoveRequest& req,
                                        GridSlot sourceSlot,
                                        GridRow sourceRow);
bool clearGridMoveSources(const GridMoveRequest& req);
bool applySparseGridMove(const GridMoveRequest& req);
bool applyGridMove(const GridMoveRequest& req);
bool saveNeedsMdCurrentPattern();
void copyBounded(char* dst, size_t dstLen, const char* src);
bool validProjectRelPath(const char* path, bool allowEmpty);
bool readProjectBodyString(const uint8_t* b, uint16_t n,
                                  uint16_t& off, char* out, size_t outLen,
                                  bool allowEmpty);
bool simpleProjectName(const char* name);
bool splitProjectRelPath(const char* path, char* parent,
                                size_t parentLen, char* base,
                                size_t baseLen);
bool joinProjectRelPath(const char* parent, const char* entry,
                               char* out, size_t outLen);
bool buildProjectRootedPath(const char* rel, char* out, size_t outLen);
bool chdirProjectParent(const char* rel, char* base, size_t baseLen);
bool pathStartsWithProjectDir(const char* path, const char* dir);
bool currentProjectUnder(const char* path);
bool isProjectDirNameAtCwd(const char* entry);
bool projectRelPathIsProject(const char* rel);
bool currentProjectChildForCwd(const char* cwd, char* out,
                                      size_t outLen);
uint8_t projectEntryFlags(uint8_t type, const char* path);
void writeProjectEntry(uint8_t* dst, uint8_t type, uint8_t flags,
                              const char* name);
bool appendProjectEntry(uint8_t* body, uint16_t logicalIndex,
                               uint16_t offset, uint8_t maxEntries,
                               uint8_t& count, uint8_t type,
                               uint8_t flags, const char* name);
void fillProjectListHeader(uint8_t* body, uint8_t flags,
                                  uint16_t offset, uint8_t count,
                                  uint16_t total, uint16_t currentIndex,
                                  const char* cwd);
uint16_t fillProjectListError(uint8_t* body, const char* cwd);
bool sameProjectParent(const char* a, const char* b);

}  // namespace sps_host_arr_internal

#endif  // MCL_FEATURE_HOST_ARRANGER
