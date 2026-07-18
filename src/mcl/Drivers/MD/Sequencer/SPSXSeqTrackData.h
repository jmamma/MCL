#ifndef SPSX_SEQ_TRACK_DATA_H__
#define SPSX_SEQ_TRACK_DATA_H__

#if !defined(__AVR__)

#include "SPSXSeqDefines.h"
#include "StepSeqTrackData.h"

using SPSXSeqStepDescriptor = StepSeqStepDescriptor;
using SPSXSeqStep = StepSeqStep;
using SPSXSeqTrackData = BasicStepSeqTrackData<SPSX_NUM_LOCKS>;

static_assert(sizeof(SPSXSeqTrackData) == 976,
              "37-lock SPS-X sequence storage wire size changed");
static_assert(sizeof(SPSXSeqTrackData) ==
                  sizeof(BasicStepSeqTrackFixedData<SPSX_NUM_LOCKS>) +
                      sizeof(uint8_t) + STEPSEQ_NUM_LOCK_SLOTS,
              "SPS-X sequence storage width changed unexpectedly");

#endif // !defined(__AVR__)
#endif // SPSX_SEQ_TRACK_DATA_H__
