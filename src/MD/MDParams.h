/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MD_PARAMS_H__
#define MD_PARAMS_H__

/**
 * \addtogroup MD Elektron MachineDrum
 *
 * @{
 *
 * \addtogroup md_params MachineDrum parameters
 *
 * @{
 **/

#define MD_ASSIGN_MACHINE_ID 0x5b
#define MD_ASSIGN_MACHINE_INIT_SYNTHESIS 0
#define MD_ASSIGN_MACHINE_INIT_SYNTHESIS_EFFECTS 1
#define MD_ASSIGN_MACHINE_INIT_SYNTHESIS_EFFECTS_ROUTING 2

#define MD_CURRENT_GLOBAL_SLOT_REQUEST 0x01
#define MD_CURRENT_KIT_REQUEST 0x02
#define MD_CURRENT_PATTERN_REQUEST 0x04
#define MD_CURRENT_SONG_REQUEST 0x08
#define MD_CURRENT_SEQUENCER_MODE 0x10
#define MD_CURRENT_LOCK_MODE 0x20
#define MD_CURRENT_TRACK_REQUEST 0x022

#define MD_GLOBAL_MESSAGE_ID 0x50
#define MD_GLOBAL_REQUEST_ID 0x51
#define MD_KIT_MESSAGE_ID 0x52
#define MD_KIT_REQUEST_ID 0x53
#define MD_PATTERN_MESSAGE_ID 0x67
#define MD_PATTERN_REQUEST_ID 0x68
#define MD_SONG_MESSAGE_ID 0x69
#define MD_SONG_REQUEST_ID 0x6a
#define MD_SAMPLE_NAME_ID 0x73

#define MD_LOAD_GLOBAL_ID 0x56
#define MD_LOAD_PATTERN_ID 0x57
#define MD_LOAD_KIT_ID 0x58
#define MD_LOAD_SONG_ID 0x6c

#define MD_SAVE_KIT_ID 0x59
#define MD_SAVE_SONG_ID 0x6d

#define MD_RESET_MIDI_NOTE_MAP_ID 0x64
#define MD_SET_ACTIVE_GLOBAL_ID 0x56
#define MD_SET_CURRENT_KIT_NAME_ID 0x55

#define MD_SET_RHYTHM_ECHO_PARAM_ID 0x5d
#define MD_SET_GATE_BOX_PARAM_ID 0x5e
#define MD_SET_EQ_PARAM_ID 0x5f
#define MD_SET_DYNAMIX_PARAM_ID 0x60

#define MD_SET_LFO_PARAM_ID 0x62
#define MD_SET_MIDI_NOTE_TO_TRACK_MAPPING_ID 0x5a
#define MD_SET_MUTE_GROUP_ID 0x66
#define MD_SET_RECEIVE_DUMP_POSITION_ID 0x6b

#define MD_SET_TEMPO_ID 0x61
#define MD_SET_TRACK_ROUTING_ID 0x5c
#define MD_SET_TRIG_GROUP_ID 0x65

#define MD_SET_STATUS_ID 0x71
#define MD_STATUS_REQUEST_ID 0x70
#define MD_STATUS_RESPONSE_ID 0x72

#define MD_FX_ECHO 0x5d
#define MD_FX_REV 0x5e
#define MD_FX_EQ 0x5f
#define MD_FX_DYN 0x60

#define GND_MODEL 0
#define GND_SN_MODEL 1
#define GND_NS_MODEL 2
#define GND_IM_MODEL 3
#define GND_SW_MODEL 4
#define GND_PU_MODEL 5

#define NFX_EV_MODEL 7
#define NFX_CO_MODEL 8
#define NFX_UC_MODEL 9

#define TRX_BD_MODEL 16
#define TRX_SD_MODEL 17
#define TRX_XT_MODEL 18
#define TRX_CP_MODEL 19
#define TRX_RS_MODEL 20
#define TRX_CB_MODEL 21
#define TRX_CH_MODEL 22
#define TRX_OH_MODEL 23
#define TRX_CY_MODEL 24
#define TRX_MA_MODEL 25
#define TRX_CL_MODEL 26
#define TRX_XC_MODEL 27
#define TRX_B2_MODEL 28
#define TRX_S2_MODEL 29

#define EFM_BD_MODEL 32
#define EFM_SD_MODEL 33
#define EFM_XT_MODEL 34
#define EFM_CP_MODEL 35
#define EFM_RS_MODEL 36
#define EFM_CB_MODEL 37
#define EFM_HH_MODEL 38
#define EFM_CY_MODEL 39

#define E12_BD_MODEL 48
#define E12_SD_MODEL 49
#define E12_HT_MODEL 50
#define E12_LT_MODEL 51
#define E12_CP_MODEL 52
#define E12_RS_MODEL 53
#define E12_CB_MODEL 54
#define E12_CH_MODEL 55
#define E12_OH_MODEL 56
#define E12_RC_MODEL 57
#define E12_CC_MODEL 58
#define E12_BR_MODEL 59
#define E12_TA_MODEL 60
#define E12_TR_MODEL 61
#define E12_SH_MODEL 62
#define E12_BC_MODEL 63

#define P_I_BD_MODEL 64
#define P_I_SD_MODEL 65
#define P_I_MT_MODEL 66
#define P_I_ML_MODEL 67
#define P_I_MA_MODEL 68
#define P_I_RS_MODEL 69
#define P_I_RC_MODEL 70
#define P_I_CC_MODEL 71
#define P_I_HH_MODEL 72

#define INP_GA_MODEL 80
#define INP_GB_MODEL 81
#define INP_FA_MODEL 82
#define INP_FB_MODEL 83
#define INP_EA_MODEL 84
#define INP_EB_MODEL 85
#define INP_CA_MODEL 86
#define INP_CB_MODEL 87

#define MID_MODEL 96

#define MID_01_MODEL 96
#define MID_02_MODEL 97
#define MID_03_MODEL 98
#define MID_04_MODEL 99
#define MID_05_MODEL 100
#define MID_06_MODEL 101
#define MID_07_MODEL 102
#define MID_08_MODEL 103
#define MID_09_MODEL 104
#define MID_10_MODEL 105
#define MID_11_MODEL 106
#define MID_12_MODEL 107
#define MID_13_MODEL 108
#define MID_14_MODEL 109
#define MID_15_MODEL 110
#define MID_16_MODEL 111

#define CTR_AL_MODEL 112
#define CTR_8P_MODEL 113

#define CTR_RE_MODEL 120
#define CTR_GB_MODEL 121
#define CTR_EQ_MODEL 122
#define CTR_DX_MODEL 123

#define ROM_01_MODEL 128
#define ROM_02_MODEL 129
#define ROM_03_MODEL 130
#define ROM_04_MODEL 131
#define ROM_05_MODEL 132
#define ROM_06_MODEL 133
#define ROM_07_MODEL 134
#define ROM_08_MODEL 135
#define ROM_09_MODEL 136
#define ROM_10_MODEL 137
#define ROM_11_MODEL 138
#define ROM_12_MODEL 139
#define ROM_13_MODEL 140
#define ROM_14_MODEL 141
#define ROM_15_MODEL 142
#define ROM_16_MODEL 143
#define ROM_17_MODEL 144
#define ROM_18_MODEL 145
#define ROM_19_MODEL 146
#define ROM_20_MODEL 147
#define ROM_21_MODEL 148
#define ROM_22_MODEL 149
#define ROM_23_MODEL 150
#define ROM_24_MODEL 151
#define ROM_25_MODEL 152
#define ROM_26_MODEL 153
#define ROM_27_MODEL 154
#define ROM_28_MODEL 155
#define ROM_29_MODEL 156
#define ROM_30_MODEL 157
#define ROM_31_MODEL 158
#define ROM_32_MODEL 159

#define RAM_R1_MODEL 160
#define RAM_R2_MODEL 161
#define RAM_P1_MODEL 162
#define RAM_P2_MODEL 163

#define RAM_R3_MODEL 165
#define RAM_R4_MODEL 166
#define RAM_P3_MODEL 167
#define RAM_P4_MODEL 168

#define ROM_33_MODEL 176
#define ROM_34_MODEL 177
#define ROM_35_MODEL 178
#define ROM_36_MODEL 179
#define ROM_37_MODEL 180
#define ROM_38_MODEL 181
#define ROM_39_MODEL 182
#define ROM_40_MODEL 183
#define ROM_41_MODEL 184
#define ROM_42_MODEL 185
#define ROM_43_MODEL 186
#define ROM_44_MODEL 187
#define ROM_45_MODEL 188
#define ROM_46_MODEL 189
#define ROM_47_MODEL 190
#define ROM_48_MODEL 191

#define ROM_MODEL 128

#define MD_ECHO_TIME 0
#define MD_ECHO_MOD 1
#define MD_ECHO_MFRQ 2
#define MD_ECHO_FB 3
#define MD_ECHO_FLTF 4
#define MD_ECHO_FLTW 5
#define MD_ECHO_MONO 6
#define MD_ECHO_LEV 7

#define MD_REV_DVOL 0
#define MD_REV_PRED 1
#define MD_REV_DEC 2
#define MD_REV_DAMP 3
#define MD_REV_HP 4
#define MD_REV_LP 5
#define MD_REV_GATE 6
#define MD_REV_LEV 7

#define MD_EQ_LF 0
#define MD_EQ_LG 1
#define MD_EQ_HF 2
#define MD_EQ_HG 3
#define MD_EQ_PF 4
#define MD_EQ_PG 5
#define MD_EQ_PQ 6
#define MD_EQ_GAIN 7

#define MD_DYN_ATCK 0
#define MD_DYN_REL 1
#define MD_DYN_TRHD 2
#define MD_DYN_RTIO 3
#define MD_DYN_KNEE 4
#define MD_DYN_HP 5
#define MD_DYN_OUTG 6
#define MD_DYN_MIX 7

/* parameter name macros */
#define GND_SN_PTCH 0
#define GND_SN_DEC 1
#define GND_SN_RAMP 2
#define GND_SN_RDEC 3
#define GND_SN_PTCH2 4
#define GND_SN_PTCH3 5
#define GND_SN_PTCH4 6
#define GND_SN_UNI 7

#define GND_SW_PTCH 0
#define GND_SW_DEC 1
#define GND_SW_RAMP 2
#define GND_SW_RDEC 3
#define GND_SW_PTCH2 4
#define GND_SW_PTCH3 5
#define GND_SW_SKEW 6
#define GND_SW_UNI 7

#define GND_PU_PTCH 0
#define GND_PU_DEC 1
#define GND_PU_RAMP 2
#define GND_PU_RDEC 3
#define GND_PU_PTCH2 4
#define GND_PU_PTCH3 5
#define GND_PU_WIDTH 6
#define GND_PU_UNI 7

#define GND_NS_DEC 0

#define GND_IM_UP 0
#define GND_IM_UVAL 1
#define GND_IM_DOWN 2
#define GND_IM_DVAL 3

#define TRX_BD_PTCH 0
#define TRX_BD_DEC 1
#define TRX_BD_RAMP 2
#define TRX_BD_RDEC 3
#define TRX_BD_STRT 4
#define TRX_BD_NOIS 5
#define TRX_BD_HARM 6
#define TRX_BD_CLIP 7
#define TRX_B2_PTCH 0
#define TRX_B2_DEC 1
#define TRX_B2_RAMP 2
#define TRX_B2_HOLD 3
#define TRX_B2_TICK 4
#define TRX_B2_NOIS 5
#define TRX_B2_DIRT 6
#define TRX_B2_DIST 7
#define TRX_SD_PTCH 0
#define TRX_SD_DEC 1
#define TRX_SD_BUMP 2
#define TRX_SD_BENV 3
#define TRX_SD_SNAP 4
#define TRX_SD_TONE 5
#define TRX_SD_TUNE 6
#define TRX_SD_CLIP 7
#define TRX_XT_PTCH 0
#define TRX_XT_DEC 1
#define TRX_XT_RAMP 2
#define TRX_XT_RDEC 3
#define TRX_XT_DAMP 4
#define TRX_XT_DIST 5
#define TRX_XT_DTYP 6
#define TRX_CP_CLPY 0
#define TRX_CP_TONE 1
#define TRX_CP_HARD 2
#define TRX_CP_RICH 3
#define TRX_CP_RATE 4
#define TRX_CP_ROOM 5
#define TRX_CP_RSIZ 6
#define TRX_CP_RTUN 7
#define TRX_RS_PTCH 0
#define TRX_RS_DEC 1
#define TRX_RS_DIST 2
#define TRX_CB_PTCH 0
#define TRX_CB_DEC 2
#define TRX_CB_ENH 3
#define TRX_CB_DAMP 4
#define TRX_CB_TONE 5
#define TRX_CB_BUMP 6
#define TRX_CH_GAP 0
#define TRX_CH_DEC 1
#define TRX_CH_HPF 2
#define TRX_CH_LPF 3
#define TRX_CH_MTAL 4
#define TRX_OH_GAP 0
#define TRX_OH_DEC 1
#define TRX_OH_HPF 2
#define TRX_OH_LPF 3
#define TRX_OH_MTAL 4
#define TRX_CY_RICH 0
#define TRX_CY_DEC 1
#define TRX_CY_TOP 2
#define TRX_CY_TTUN 3
#define TRX_CY_SIZE 4
#define TRX_CY_PEAK 5
#define TRX_MA_ATT 0
#define TRX_MA_SUS 1
#define TRX_MA_REV 2
#define TRX_MA_DAMP 3
#define TRX_MA_RATL 4
#define TRX_MA_RTYP 5
#define TRX_MA_TONE 6
#define TRX_MA_HARD 7
#define TRX_CL_PTCH 0
#define TRX_CL_DEC 1
#define TRX_CL_DUAL 2
#define TRX_CL_ENH 3
#define TRX_CL_TUNE 4
#define TRX_CL_CLIC 5
#define TRX_XC_PTCH 0
#define TRX_XC_DEC 1
#define TRX_XC_RAMP 2
#define TRX_XC_RDEC 3
#define TRX_XC_DAMP 4
#define TRX_XC_DIST 5
#define TRX_XC_DTYP 6

#define TRX_S2_PTCH 0
#define TRX_S2_DEC 1
#define TRX_S2_NOISE 2
#define TRX_S2_NDEC 3
#define TRX_S2_POWER 4
#define TRX_S2_TUNE 5
#define TRX_S2_NTUNE 6
#define TRX_S2_NTYPE 7

#define EFM_BD_PTCH 0
#define EFM_BD_DEC 1
#define EFM_BD_RAMP 2
#define EFM_BD_RDEC 3
#define EFM_BD_MOD 4
#define EFM_BD_MFRQ 5
#define EFM_BD_MDEC 6
#define EFM_BD_MFB 7
#define EFM_SD_PTCH 0
#define EFM_SD_DEC 1
#define EFM_SD_NOIS 2
#define EFM_SD_NDEC 3
#define EFM_SD_MOD 4
#define EFM_SD_MFRQ 5
#define EFM_SD_MDEC 6
#define EFM_SD_HPF 7
#define EFM_XT_PTCH 0
#define EFM_XT_DEC 1
#define EFM_XT_RAMP 2
#define EFM_XT_RDEC 3
#define EFM_XT_MOD 4
#define EFM_XT_MFRQ 5
#define EFM_XT_MDEC 6
#define EFM_XT_CLIC 7
#define EFM_CP_PTCH 0
#define EFM_CP_DEC 1
#define EFM_CP_CLPS 2
#define EFM_CP_CDEC 3
#define EFM_CP_MOD 4
#define EFM_CP_MFRQ 5
#define EFM_CP_MDEC 6
#define EFM_CP_HPF 7
#define EFM_RS_PTCH 0
#define EFM_RS_DEC 1
#define EFM_RS_MOD 2
#define EFM_RS_HPF 3
#define EFM_RS_SNAR 4
#define EFM_RS_SPTC 5
#define EFM_RS_SDEC 6
#define EFM_RS_SMOD 7
#define EFM_CB_PTCH 0
#define EFM_CB_DEC 1
#define EFM_CB_SNAP 2
#define EFM_CB_FB 3
#define EFM_CB_MOD 4
#define EFM_CB_MFRQ 5
#define EFM_CB_MDEC 6
#define EFM_HH_PTCH 0
#define EFM_HH_DEC 1
#define EFM_HH_TREM 2
#define EFM_HH_TFRQ 3
#define EFM_HH_MOD 4
#define EFM_HH_MFRQ 5
#define EFM_HH_MDEC 6
#define EFM_HH_FB 7
#define EFM_CY_PTCH 0
#define EFM_CY_DEC 1
#define EFM_CY_FB 2
#define EFM_CY_HPF 3
#define EFM_CY_MOD 4
#define EFM_CY_MFRQ 5
#define EFM_CY_MDEC 6

#define E12_BD_PTCH 0
#define E12_BD_DEC 1
#define E12_BD_SNAP 2
#define E12_BD_SPLN 3
#define E12_BD_STRT 4
#define E12_BD_RTRG 5
#define E12_BD_RTIM 6
#define E12_BD_BEND 7
#define E12_SD_PTCH 0
#define E12_SD_DEC 1
#define E12_SD_HP 2
#define E12_SD_RING 3
#define E12_SD_STRT 4
#define E12_SD_RTRG 5
#define E12_SD_RTIM 6
#define E12_SD_BEND 7
#define E12_HT_PTCH 0
#define E12_HT_DEC 1
#define E12_HT_HP 2
#define E12_HT_HPQ 3
#define E12_HT_STRT 4
#define E12_HT_RTRG 5
#define E12_HT_RTIM 6
#define E12_HT_BEND 7
#define E12_LT_PTCH 0
#define E12_LT_DEC 1
#define E12_LT_HP 2
#define E12_LT_RING 3
#define E12_LT_STRT 4
#define E12_LT_RTRG 5
#define E12_LT_RTIM 6
#define E12_LT_BEND 7
#define E12_CP_PTCH 0
#define E12_CP_DEC 1
#define E12_CP_HP 2
#define E12_CP_HPQ 3
#define E12_CP_STRT 4
#define E12_CP_RTRG 5
#define E12_CP_RTIM 6
#define E12_CP_BEND 7
#define E12_RS_PTCH 0
#define E12_RS_DEC 1
#define E12_RS_HP 2
#define E12_RS_RRTL 3
#define E12_RS_STRT 4
#define E12_RS_RTRG 5
#define E12_RS_RTIM 6
#define E12_RS_BEND 7
#define E12_CB_PTCH 0
#define E12_CB_DEC 1
#define E12_CB_HP 2
#define E12_CB_HPQ 3
#define E12_CB_STRT 4
#define E12_CB_RTRG 5
#define E12_CB_RTIM 6
#define E12_CB_BEND 7
#define E12_CH_PTCH 0
#define E12_CH_DEC 1
#define E12_CH_HP 2
#define E12_CH_HPQ 3
#define E12_CH_STRT 4
#define E12_CH_RTRG 5
#define E12_CH_RTIM 6
#define E12_CH_BEND 7
#define E12_OH_PTCH 0
#define E12_OH_DEC 1
#define E12_OH_HP 2
#define E12_OH_STOP 3
#define E12_OH_STRT 4
#define E12_OH_RTRG 5
#define E12_OH_RTIM 6
#define E12_OH_BEND 7
#define E12_RC_PTCH 0
#define E12_RC_DEC 1
#define E12_RC_HP 2
#define E12_RC_BELL 3
#define E12_RC_STRT 4
#define E12_RC_RTRG 5
#define E12_RC_RTIM 6
#define E12_RC_BEND 7
#define E12_CC_PTCH 0
#define E12_CC_DEC 1
#define E12_CC_HP 2
#define E12_CC_HPQ 3
#define E12_CC_STRT 4
#define E12_CC_RTRG 5
#define E12_CC_RTIM 6
#define E12_CC_BEND 7
#define E12_BR_PTCH 0
#define E12_BR_DEC 1
#define E12_BR_HP 2
#define E12_BR_REAL 3
#define E12_BR_STRT 4
#define E12_BR_RTRG 5
#define E12_BR_RTIM 6
#define E12_BR_BEND 7
#define E12_TA_PTCH 0
#define E12_TA_DEC 1
#define E12_TA_HP 2
#define E12_TA_HPQ 3
#define E12_TA_STRT 4
#define E12_TA_RTRG 5
#define E12_TA_RTIM 6
#define E12_TA_BEND 7
#define E12_TR_PTCH 0
#define E12_TR_DEC 1
#define E12_TR_HP 2
#define E12_TR_HPQ 3
#define E12_TR_STRT 4
#define E12_TR_RTRG 5
#define E12_TR_RTIM 6
#define E12_TR_BEND 7
#define E12_SH_PTCH 0
#define E12_SH_DEC 1
#define E12_SH_HP 2
#define E12_SH_SLEW 3
#define E12_SH_STRT 4
#define E12_SH_RTRG 5
#define E12_SH_RTIM 6
#define E12_SH_BEND 7
#define E12_BC_PTCH 0
#define E12_BC_DEC 1
#define E12_BC_HP 2
#define E12_BC_BC 3
#define E12_BC_STRT 4
#define E12_BC_RTRG 5
#define E12_BC_RTIM 6
#define E12_BC_BEND 7

#define P_I_BD_PTCH 0
#define P_I_BD_DEC 1
#define P_I_BD_HARD 2
#define P_I_BD_HAMR 3
#define P_I_BD_TENS 4
#define P_I_BD_DAMP 5
#define P_I_SD_PTCH 0
#define P_I_SD_DEC 1
#define P_I_SD_HARD 2
#define P_I_SD_TENS 3
#define P_I_SD_RVOL 4
#define P_I_SD_RDEC 5
#define P_I_SD_RING 6
#define P_I_MT_PTCH 0
#define P_I_MT_DEC 1
#define P_I_MT_HARD 2
#define P_I_MT_HAMR 3
#define P_I_MT_TUNE 4
#define P_I_MT_DAMP 5
#define P_I_MT_SIZE 6
#define P_I_MT_POS 7
#define P_I_ML_PTCH 0
#define P_I_ML_DEC 1
#define P_I_ML_HARD 2
#define P_I_ML_TENS 3
#define P_I_MA_GRNS 0
#define P_I_MA_DEC 1
#define P_I_MA_GLEN 2
#define P_I_MA_SIZE 4
#define P_I_MA_HARD 5
#define P_I_RS_rs_PTCH 0
#define P_I_RS_DEC 1
#define P_I_RS_HARD 2
#define P_I_RS_RING 3
#define P_I_RS_RVOL 4
#define P_I_RS_RDEC 5
#define P_I_RC_PTCH 0
#define P_I_RC_DEC 1
#define P_I_RC_HARD 2
#define P_I_RC_RING 3
#define P_I_RC_AG 4
#define P_I_RC_AU 5
#define P_I_RC_BR 6
#define P_I_RC_GRAB 7
#define P_I_CC_PTCH 0
#define P_I_CC_DEC 1
#define P_I_CC_HARD 2
#define P_I_CC_RING 3
#define P_I_CC_AG 4
#define P_I_CC_AU 5
#define P_I_CC_BR 6
#define P_I_CC_GRAB 7
#define P_I_HH_PTCH 0
#define P_I_HH_DEC 1
#define P_I_HH_CLSN 2
#define P_I_HH_RING 3
#define P_I_HH_AG 4
#define P_I_HH_AU 5
#define P_I_HH_BR 6
#define P_I_HH_CLOS 7

#define INP_GA_VOL 0
#define INP_G_GATE 1
#define INP_G_ATCK 2
#define INP_G_HLD 3
#define INP_G_DEC 4
#define INP_FA_ALEV 0
#define INP_F_GATE 1
#define INP_F_FATK 2
#define INP_F_FHLD 3
#define INP_F_FDEC 4
#define INP_F_FDPH 5
#define INP_F_FFRQ 6
#define INP_F_FQ 7
#define INP_EA_AVOL 0
#define INP_E_AHLD 1
#define INP_E_ADEC 2
#define INP_E_FQ 3
#define INP_E_FDPH 4
#define INP_E_FHLD 5
#define INP_E_FDEC 6
#define INP_E_FFRQ 7

#define MID_NOTE 0
#define MID_N2 1
#define MID_N3 2
#define MID_LEN 3
#define MID_VEL 4
#define MID_PB 5
#define MID_MW 6
#define MID_AT 7
#define MID_CC1D 8
#define MID_CC1V 9
#define MID_CC2D 10
#define MID_CC2V 11
#define MID_CC3D 12
#define MID_CC3V 13
#define MID_CC4D 14
#define MID_CC4V 15
#define MID_CC5D 16
#define MID_CC5V 17
#define MID_CC6D 18
#define MID_CC6V 19
#define MID_PCHG 20
#define MID_LFOS 21
#define MID_LFOD 22
#define MID_LFOM 23

#define CTR_AL_SYN1 0
#define CTR_AL_SYN2 1
#define CTR_AL_SYN3 2
#define CTR_AL_SYN4 3
#define CTR_AL_SYN5 4
#define CTR_AL_SYN6 5
#define CTR_AL_SYN7 6
#define CTR_AL_SYN8 7

#define CTR_8P_P1 0
#define CTR_8P_P2 1
#define CTR_8P_P3 2
#define CTR_8P_P4 3
#define CTR_8P_P5 4
#define CTR_8P_P6 5
#define CTR_8P_P7 6
#define CTR_8P_P8 7
#define CTR_8P_P1T 8
#define CTR_8P_P1P 9
#define CTR_8P_P2T 10
#define CTR_8P_P2P 11
#define CTR_8P_P3T 12
#define CTR_8P_P3P 13
#define CTR_8P_P4T 14
#define CTR_8P_P4P 15
#define CTR_8P_P5T 16
#define CTR_8P_P5P 17
#define CTR_8P_P6T 18
#define CTR_8P_P6P 19
#define CTR_8P_P7T 20
#define CTR_8P_P7P 21
#define CTR_8P_P8T 22
#define CTR_8P_P8P 23

#define ROM_PTCH 0
#define ROM_DEC 1
#define ROM_HOLD 2
#define ROM_BRR 3
#define ROM_STRT 4
#define ROM_END 5
#define ROM_RTRG 6
#define ROM_RTIM 7

#define RAM_R_MLEV 0
#define RAM_R_MBAL 1
#define RAM_R_ILEV 2
#define RAM_R_IBAL 3
#define RAM_R_CUE1 4
#define RAM_R_CUE2 5
#define RAM_R_LEN 6
#define RAM_R_RATE 7

#define MODEL_P1 0
#define MODEL_P2 1
#define MODEL_P3 2
#define MODEL_P4 3
#define MODEL_P5 4
#define MODEL_P6 5
#define MODEL_P7 6
#define MODEL_P8 7

#define MODEL_AMD 8
#define MODEL_AMF 9
#define MODEL_EQF 10
#define MODEL_EQG 11
#define MODEL_FLTF 12
#define MODEL_FLTW 13
#define MODEL_FLTQ 14
#define MODEL_SRR 15

#define MODEL_DIST 16
#define MODEL_VOL 17
#define MODEL_PAN 18
#define MODEL_DEL 19
#define MODEL_REV 20
#define MODEL_LFOS 21
#define MODEL_LFOD 22
#define MODEL_LFOM 23
#define MODEL_LEVEL 33

#define MD_ECHO_TIME 0
#define MD_ECHO_MOD 1
#define MD_ECHO_MFRQ 2
#define MD_ECHO_FB 3
#define MD_ECHO_FLTF 4
#define MD_ECHO_FLTW 5
#define MD_ECHO_MONO 6
#define MD_ECHO_LEV 7

#define MD_REV_DVOL 0
#define MD_REV_PRED 1
#define MD_REV_DEC 2
#define MD_REV_DAMP 3
#define MD_REV_HP 4
#define MD_REV_LP 5
#define MD_REV_GATE 6
#define MD_REV_LEV 7

#define MD_EQ_LF 0
#define MD_EQ_LG 1
#define MD_EQ_HF 2
#define MD_EQ_HG 3
#define MD_EQ_PF 4
#define MD_EQ_PG 5
#define MD_EQ_PQ 6
#define MD_EQ_GAIN 7

#define MD_DYN_ATCK 0
#define MD_DYN_REL 1
#define MD_DYN_TRHD 2
#define MD_DYN_RTIO 3
#define MD_DYN_KNEE 4
#define MD_DYN_HP 5
#define MD_DYN_OUTG 6
#define MD_DYN_MIX 7

#define MD_LFO_TRACK 0
#define MD_LFO_PARAM 1
#define MD_LFO_SHP1 2
#define MD_LFO_SHP2 3
#define MD_LFO_UPDTE 4
#define MD_LFO_SPEED 5
#define MD_LFO_DEPTH 6
#define MD_LFO_SHMIX 7

#define MD_LFO_TYPE_FREE 0
#define MD_LFO_TYPE_TRIG 1
#define MD_LFO_TYPE_HOLD 2

#define MD_GUI_CMD_OFF 0x00
#define MD_GUI_CMD_ON 0x7F

#define MD_GUI_CMD 0x40
#define MD_GUI_KIT_WIN 0x01
#define MD_GUI_LFO_WIN 0x02
#define MD_GUI_UPARROW 0x03
#define MD_GUI_DOWNARROW 0x04
#define MD_GUI_RECORD 0x07
#define MD_GUI_PLAY 0x09
#define MD_GUI_STOP 0x0A
#define MD_GUI_EXTENDED 0x0B
#define MD_GUI_BANKGROUP 0x0C
#define MD_GUI_ACCENT_WIN 0x0D
#define MD_GUI_SWING_WIN 0x0E
#define MD_GUI_SLIDE_WIN 0x0F
#define MD_GUI_TRIG_1 0x10
#define MD_GUI_TRIG_2 0x11
#define MD_GUI_TRIG_3 0x12
#define MD_GUI_TRIG_4 0x13
#define MD_GUI_TRIG_5 0x14
#define MD_GUI_TRIG_6 0x15
#define MD_GUI_TRIG_7 0x16
#define MD_GUI_TRIG_8 0x17
#define MD_GUI_TRIG_9 0x18
#define MD_GUI_TRIG_10 0x19
#define MD_GUI_TRIG_11 0x1A
#define MD_GUI_TRIG_12 0x1B
#define MD_GUI_TRIG_13 0x1C
#define MD_GUI_TRIG_14 0x1D
#define MD_GUI_TRIG_15 0x1E
#define MD_GUI_TRIG_16 0x1F
#define MD_GUI_BANK_1 0x20
#define MD_GUI_BANK_2 0x21
#define MD_GUI_BANK_3 0x22
#define MD_GUI_BANK_4 0x23
#define MD_GUI_TEMPO_WIN 0x24
#define MD_GUI_FUNC 0x25
#define MD_GUI_LEFTARROW 0x26
#define MD_GUI_RIGHTARROW 0x27
#define MD_GUI_YES 0x28
#define MD_GUI_NO 0x29
#define MD_GUI_SCALE 0x2A
#define MD_GUI_SCALE_WIN 0x2B
#define MD_GUI_MUTE_WIN 0x2C
#define MD_GUI_PATTERNSONG 0x2D
#define MD_GUI_SONG_WIN 0x2E
#define MD_GUI_GLOBAL_WIN 0x2F
#define MD_GUI_COPY 0x34
#define MD_GUI_CLEAR 0x35
#define MD_GUI_PASTE 0x36
#define MD_GUI_SYNTH 0x3A
#define MD_GUI_TRACK_1 0x40
#define MD_GUI_TRACK_2 0x41
#define MD_GUI_TRACK_3 0x42
#define MD_GUI_TRACK_4 0x43
#define MD_GUI_TRACK_5 0x44
#define MD_GUI_TRACK_6 0x45
#define MD_GUI_TRACK_7 0x46
#define MD_GUI_TRACK_8 0x47
#define MD_GUI_TRACK_9 0x48
#define MD_GUI_TRACK_10 0x49
#define MD_GUI_TRACK_11 0x4A
#define MD_GUI_TRACK_12 0x4B
#define MD_GUI_TRACK_13 0x4C
#define MD_GUI_TRACK_14 0x4D
#define MD_GUI_TRACK_15 0x4E
#define MD_GUI_TRACK_16 0x4F
#define MD_GUI_ENC_1 0x50
#define MD_GUI_ENC_2 0x51
#define MD_GUI_ENC_3 0x52
#define MD_GUI_ENC_4 0x53
#define MD_GUI_ENC_5 0x54
#define MD_GUI_ENC_6 0x55
#define MD_GUI_ENC_7 0x56
#define MD_GUI_ENC_8 0x57
#define MD_GUI_TEMPO 0x5A

#define MD_GUI_TRACK_1 0x40

extern const char* model_param_name(uint8_t model, uint8_t param);

extern const char *MDLFONames[8];

extern const char* getMDMachineNameShort(uint8_t machine, uint8_t type);
extern const char* fx_param_name(uint8_t fx_type, uint8_t param);

/** This structure stores the tuning information of a melodic machine on the
 * machinedrum. **/
typedef struct tuning_s {

  /** Model of the melodic machine. **/
  uint8_t model;
  /** Base pitch of the melodic machine. **/
  uint8_t base;
  /** Length of the tuning array storing the pitch values for each pitch. **/
  uint8_t len;
  uint8_t offset;
  /** Pointer to an array for pitch values for individual midi notes. **/
  const uint8_t *tuning;

  /* @} */
} tuning_t;


/* @} @} */

#endif /* MD_PARAMS_H__ */
