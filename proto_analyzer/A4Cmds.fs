module A4Cmds

// kit response len = 2679
let A4_KIT_MESSAGE_ID =                    0x52
let A4_KIT_REQUEST_ID =                    0x62

// kitx response len = 2679
let A4_KITX_MESSAGE_ID =                   0x58
let A4_KITX_REQUEST_ID =                   0x68

// sound response len = 415
let A4_SOUND_MESSAGE_ID =                  0x53
let A4_SOUND_REQUEST_ID =                  0x63

// soundx response len = 415
let A4_SOUNDX_MESSAGE_ID =                 0x59
let A4_SOUNDX_REQUEST_ID =                 0x69

// pattern response len = 14788
let A4_PATTERN_MESSAGE_ID =                0x54
let A4_PATTERN_REQUEST_ID =                0x64

// patternx response len = 14788
let A4_PATTERNX_MESSAGE_ID =               0x5A
let A4_PATTERNX_REQUEST_ID =               0x6A

// song response len = 1506
let A4_SONG_MESSAGE_ID =                   0x55
let A4_SONG_REQUEST_ID =                   0x65

// songx response len = 1506
let A4_SONGX_MESSAGE_ID =                  0x5B
let A4_SONGX_REQUEST_ID =                  0x6B

// settings response len = 85
let A4_SETTINGS_MESSAGE_ID =               0x56
let A4_SETTINGS_REQUEST_ID =               0x66

// settingsx response len = 85
let A4_SETTINGSX_MESSAGE_ID =              0x5C
let A4_SETTINGSX_REQUEST_ID =              0x6C

// global response len = 2612
let A4_GLOBAL_MESSAGE_ID =                 0x57
let A4_GLOBAL_REQUEST_ID =                 0x67

// global response len = 2612
let A4_GLOBALX_MESSAGE_ID =                0x5D
let A4_GLOBALX_REQUEST_ID =                0x6D

let A4_ANALOG_CALIBRATION_REQUEST_ID =     0x7C

let A4_PRESS_STOP_KEY_REQUEST_ID =         0xF8
// BPM does not display LOCK
let A4_PRESS_STOP_KEY_REQUEST_ID2 =        0xFC

// nothing, but freeze for a while (BPM displays LOCK)
let A4_UNKNOWN_0_REQUEST_ID =              0xFA
// nothing, but freeze for a while (BPM displays LOCK)
let A4_UNKNOWN_1_REQUEST_ID =              0xFB
// dumps something
let A4_UNKNOWN_2_REQUEST_ID =              0xFD

(*
COMMAND 0x60 param

Generates an endless stream of dumps (what is it?)
*)
let A4_UNKNOWN_3_REQUEST_ID =              0x60

(*
E0 00 = pitch down octaves
E0 01 01 = then, pitch up 2semitones?

E1 = track2
E2 = track3
E3 01 = track4 down 2 semitones
...
E8 same
*)
let A4_UNKNOWN_4_REQUEST_ID =              0xE0
let A4_UNKNOWN_5_REQUEST_ID =              0xE8
let A4_UNKNOWN_6_REQUEST_ID =              0xE1

// seems to be unmute, 1 param
let A4_UNKNOWN_7_REQUEST_ID =              0x80 // ~0x84?
// seems to be mute, 1 param
let A4_UNKNOWN_8_REQUEST_ID =              0x90 // ~0x94?

let A4_BAD_COMMANDS = [
    A4_ANALOG_CALIBRATION_REQUEST_ID
    A4_PRESS_STOP_KEY_REQUEST_ID
    A4_PRESS_STOP_KEY_REQUEST_ID2
    A4_UNKNOWN_0_REQUEST_ID
    A4_UNKNOWN_1_REQUEST_ID
    // A4_UNKNOWN_2_REQUEST_ID
    A4_UNKNOWN_3_REQUEST_ID
    A4_UNKNOWN_4_REQUEST_ID
    A4_UNKNOWN_5_REQUEST_ID
    A4_UNKNOWN_6_REQUEST_ID
    A4_UNKNOWN_7_REQUEST_ID
    A4_UNKNOWN_8_REQUEST_ID
]

let A4_KNOWN_COMMANDS = [
    A4_GLOBALX_MESSAGE_ID
    A4_GLOBALX_REQUEST_ID
    A4_GLOBAL_MESSAGE_ID
    A4_GLOBAL_REQUEST_ID
    A4_KITX_MESSAGE_ID
    A4_KITX_REQUEST_ID
    A4_KIT_MESSAGE_ID
    A4_KIT_REQUEST_ID
    A4_PATTERNX_MESSAGE_ID
    A4_PATTERNX_REQUEST_ID
    A4_PATTERN_MESSAGE_ID
    A4_PATTERN_REQUEST_ID
    A4_SETTINGSX_MESSAGE_ID
    A4_SETTINGSX_REQUEST_ID
    A4_SETTINGS_MESSAGE_ID
    A4_SETTINGS_REQUEST_ID
    A4_SONGX_MESSAGE_ID
    A4_SONG_MESSAGE_ID
    A4_SONGX_REQUEST_ID
    A4_SONG_REQUEST_ID
    A4_SOUNDX_MESSAGE_ID
    A4_SOUNDX_REQUEST_ID
    A4_SOUND_MESSAGE_ID
    A4_SOUND_REQUEST_ID

    A4_ANALOG_CALIBRATION_REQUEST_ID
    A4_PRESS_STOP_KEY_REQUEST_ID
    A4_PRESS_STOP_KEY_REQUEST_ID2
    A4_UNKNOWN_0_REQUEST_ID
    A4_UNKNOWN_1_REQUEST_ID
    A4_UNKNOWN_2_REQUEST_ID
    A4_UNKNOWN_3_REQUEST_ID
    A4_UNKNOWN_4_REQUEST_ID
    A4_UNKNOWN_5_REQUEST_ID
    A4_UNKNOWN_6_REQUEST_ID
    A4_UNKNOWN_7_REQUEST_ID
    A4_UNKNOWN_8_REQUEST_ID
]

