/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACKDATA_H__
#define MDSEQTRACKDATA_H__

class MDSeqTrackData {
public:
  uint8_t length;
  uint8_t locks[4][64];
  uint8_t locks_params[4];
  uint64_t pattern_mask;
  uint64_t lock_mask;
  uint8_t conditional[64];
  uint8_t timing[64];
};

#endif /* MDSEQTRACKDATA_H__ */
