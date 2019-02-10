/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLMEMORY_H__
#define MCLMEMORY_H__

#define BANK1_R1_START 0x2200
#define BANK1_R1_END (BANK1_R1_START + (sizeof(GridTrack) + sizeof(MDMachine) + sizeof(MDSeqTrackData)) * NUM_MD_TRACKS)
#define BANK1_R2_START (BANK1_R1_END + 1)
#define BANK1_R2_END (BANK1_R2_START + (sizeof(GridTrack) + sizeof(MDMachine) + sizeof(MDSeqTrackData)) * NUM_MD_TRACKS)
#define FILE_ENTRIES_START (BANK1_R2_END + 1)
#define MAX_ENTRIES 1024
#define FILE_ENTRIES_END (FILE_ENTRIES_START + 16 * MAX_ENTRIES
#endif /* MCLMEMORY_H__ */
