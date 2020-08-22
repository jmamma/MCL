/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACKDATA_H__
#define MDSEQTRACKDATA_H__

#define NUM_MD_LOCKS_270 4
#define NUM_MD_STEPS_270 64

#define NUM_MD_LOCK_SLOTS 256
#define NUM_MD_STEPS 64
#define NUM_MD_LOCKS 8

class MDSeqStep {
public:
  bool active;
  uint8_t locks[NUM_MD_LOCKS];
  uint8_t timing;
  uint8_t conditional;
  bool conditional_plock;
  bool pattern_mask;
  bool slide_mask;
};

class MDSeqTrackData_270 {
public:
  uint8_t length;
  uint8_t speed;
  uint32_t slide_mask32; // to be increased to 64bits
  uint8_t locks[NUM_MD_LOCKS_270][NUM_MD_STEPS_270];
  uint8_t locks_params[NUM_MD_LOCKS_270];
  uint64_t pattern_mask;
  uint64_t lock_mask;
  uint8_t conditional[NUM_MD_STEPS_270];
  uint8_t timing[NUM_MD_STEPS_270];
};

class MDSeqStepDescriptor {
public:
  uint8_t locks; // <-- bitfield of 8 locks in the current step, first lock is lsb
  bool trig:1;
  bool slide:1;
  bool cond_plock:1;
  uint8_t cond_id:4;
  bool is_lock(const uint8_t idx) const { return locks & (1 << idx); }
};

class MDSeqTrackData {
public:
  uint8_t locks[NUM_MD_LOCK_SLOTS];
  uint8_t locks_params[NUM_MD_LOCKS];
  uint8_t timing[NUM_MD_STEPS];
  MDSeqStepDescriptor steps[NUM_MD_STEPS];

  void init() {
    memset(this, 0, sizeof(MDSeqTrackData));
  }
  bool convert(MDSeqTrackData_270 *old) {
    /*ordering of these statements is important to ensure memory
     * is copied before being overwritten*/
  }
};

#endif /* MDSEQTRACKDATA_H__ */
