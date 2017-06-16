/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDI_NOTES_H__
#define MIDI_NOTES_H__

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_notes Midi Notes 
 *
 * @{
 **/


#define MIDI_NOTE_C0   ((0 * 12) + 0) // 0
#define MIDI_NOTE_CS0  ((0 * 12) + 1) // 1
#define MIDI_NOTE_DB0  ((0 * 12) + 1) // 1
#define MIDI_NOTE_D0   ((0 * 12) + 2) // 2
#define MIDI_NOTE_DS0  ((0 * 12) + 3) // 3
#define MIDI_NOTE_EB0  ((0 * 12) + 3) // 3
#define MIDI_NOTE_E0   ((0 * 12) + 4) // 4
#define MIDI_NOTE_F0   ((0 * 12) + 5) // 5
#define MIDI_NOTE_FS0  ((0 * 12) + 6) // 6
#define MIDI_NOTE_GB0  ((0 * 12) + 6) // 6
#define MIDI_NOTE_G0   ((0 * 12) + 7) // 70
#define MIDI_NOTE_GS0  ((0 * 12) + 8) // 8
#define MIDI_NOTE_AB0  ((0 * 12) + 8) // 8
#define MIDI_NOTE_A0   ((0 * 12) + 9) // 9
#define MIDI_NOTE_AS0  ((0 * 12) + 10) // 10
#define MIDI_NOTE_BB0  ((0 * 12) + 10) // 10
#define MIDI_NOTE_B0   ((0 * 12) + 11) // 11

#define MIDI_NOTE_C1   ((1 * 12) + 0) // 12
#define MIDI_NOTE_CS1  ((1 * 12) + 1) // 13
#define MIDI_NOTE_DB1  ((1 * 12) + 1) // 13
#define MIDI_NOTE_D1   ((1 * 12) + 2) // 14
#define MIDI_NOTE_DS1  ((1 * 12) + 3) // 15
#define MIDI_NOTE_EB1  ((1 * 12) + 3) // 15
#define MIDI_NOTE_E1   ((1 * 12) + 4) // 16
#define MIDI_NOTE_F1   ((1 * 12) + 5) // 17
#define MIDI_NOTE_FS1  ((1 * 12) + 6) // 18
#define MIDI_NOTE_GB1  ((1 * 12) + 6) // 18
#define MIDI_NOTE_G1   ((1 * 12) + 7) // 19
#define MIDI_NOTE_GS1  ((1 * 12) + 8) // 20
#define MIDI_NOTE_AB1  ((1 * 12) + 8) // 20
#define MIDI_NOTE_A1   ((1 * 12) + 9) // 21
#define MIDI_NOTE_AS1  ((1 * 12) + 10) // 22
#define MIDI_NOTE_BB1  ((1 * 12) + 10) // 22
#define MIDI_NOTE_B1   ((1 * 12) + 11) // 23

#define MIDI_NOTE_C2   ((2 * 12) + 0) // 24
#define MIDI_NOTE_CS2  ((2 * 12) + 1) // 25
#define MIDI_NOTE_DB2  ((2 * 12) + 1) // 25
#define MIDI_NOTE_D2   ((2 * 12) + 2) // 26
#define MIDI_NOTE_DS2  ((2 * 12) + 3) // 27
#define MIDI_NOTE_EB2  ((2 * 12) + 3) // 27
#define MIDI_NOTE_E2   ((2 * 12) + 4) // 28
#define MIDI_NOTE_F2   ((2 * 12) + 5) // 29
#define MIDI_NOTE_FS2  ((2 * 12) + 6) // 30
#define MIDI_NOTE_GB2  ((2 * 12) + 6) // 30
#define MIDI_NOTE_G2   ((2 * 12) + 7) // 31
#define MIDI_NOTE_GS2  ((2 * 12) + 8) // 32
#define MIDI_NOTE_AB2  ((2 * 12) + 8) // 32
#define MIDI_NOTE_A2   ((2 * 12) + 9) // 33
#define MIDI_NOTE_AS2  ((2 * 12) + 10) // 34
#define MIDI_NOTE_BB2  ((2 * 12) + 10) // 34
#define MIDI_NOTE_B2   ((2 * 12) + 11) // 35

#define MIDI_NOTE_C3   ((3 * 12) + 0) // 36
#define MIDI_NOTE_CS3  ((3 * 12) + 1) // 37
#define MIDI_NOTE_DB3  ((3 * 12) + 1) // 37
#define MIDI_NOTE_D3   ((3 * 12) + 2) // 38
#define MIDI_NOTE_DS3  ((3 * 12) + 3) // 39
#define MIDI_NOTE_EB3  ((3 * 12) + 3) // 39
#define MIDI_NOTE_E3   ((3 * 12) + 4) // 40
#define MIDI_NOTE_F3   ((3 * 12) + 5) // 41
#define MIDI_NOTE_FS3  ((3 * 12) + 6) // 42
#define MIDI_NOTE_GB3  ((3 * 12) + 6) // 42
#define MIDI_NOTE_G3   ((3 * 12) + 7) // 43
#define MIDI_NOTE_GS3  ((3 * 12) + 8) // 44 
#define MIDI_NOTE_AB3  ((3 * 12) + 8) // 44
#define MIDI_NOTE_A3   ((3 * 12) + 9) // 45
#define MIDI_NOTE_AS3  ((3 * 12) + 10) // 46
#define MIDI_NOTE_BB3  ((3 * 12) + 10) // 46
#define MIDI_NOTE_B3   ((3 * 12) + 11) // 47

#define MIDI_NOTE_C4   ((4 * 12) + 0) // 48
#define MIDI_NOTE_CS4  ((4 * 12) + 1) // 49
#define MIDI_NOTE_DB4  ((4 * 12) + 1) // 49
#define MIDI_NOTE_D4   ((4 * 12) + 2) // 50
#define MIDI_NOTE_DS4  ((4 * 12) + 3) // 51
#define MIDI_NOTE_EB4  ((4 * 12) + 3) // 51
#define MIDI_NOTE_E4   ((4 * 12) + 4) // 52
#define MIDI_NOTE_F4   ((4 * 12) + 5) // 53
#define MIDI_NOTE_FS4  ((4 * 12) + 6) // 54
#define MIDI_NOTE_GB4  ((4 * 12) + 6) // 54
#define MIDI_NOTE_G4   ((4 * 12) + 7) // 55
#define MIDI_NOTE_GS4  ((4 * 12) + 8) // 56
#define MIDI_NOTE_AB4  ((4 * 12) + 8) // 56
#define MIDI_NOTE_A4   ((4 * 12) + 9) // 57
#define MIDI_NOTE_AS4  ((4 * 12) + 10) // 58
#define MIDI_NOTE_BB4  ((4 * 12) + 10) // 58
#define MIDI_NOTE_B4   ((4 * 12) + 11) // 59

#define MIDI_NOTE_C5   ((5 * 12) + 0) // 60
#define MIDI_NOTE_CS5  ((5 * 12) + 1) // 61
#define MIDI_NOTE_DB5  ((5 * 12) + 1) // 61
#define MIDI_NOTE_D5   ((5 * 12) + 2) // 62
#define MIDI_NOTE_DS5  ((5 * 12) + 3) // 63
#define MIDI_NOTE_EB5  ((5 * 12) + 3) // 63
#define MIDI_NOTE_E5   ((5 * 12) + 4) // 64
#define MIDI_NOTE_F5   ((5 * 12) + 5) // 65
#define MIDI_NOTE_FS5  ((5 * 12) + 6) // 66
#define MIDI_NOTE_GB5  ((5 * 12) + 6) // 66
#define MIDI_NOTE_G5   ((5 * 12) + 7) // 67
#define MIDI_NOTE_GS5  ((5 * 12) + 8) // 68
#define MIDI_NOTE_AB5  ((5 * 12) + 8) // 68
#define MIDI_NOTE_A5   ((5 * 12) + 9) // 69
#define MIDI_NOTE_AS5  ((5 * 12) + 10) // 70
#define MIDI_NOTE_BB5  ((5 * 12) + 10) // 70
#define MIDI_NOTE_B5   ((5 * 12) + 11) // 71

#define MIDI_NOTE_C6   ((6 * 12) + 0) // 72
#define MIDI_NOTE_CS6  ((6 * 12) + 1) // 73
#define MIDI_NOTE_DB6  ((6 * 12) + 1) // 73
#define MIDI_NOTE_D6   ((6 * 12) + 2) // 74
#define MIDI_NOTE_DS6  ((6 * 12) + 3) // 75
#define MIDI_NOTE_EB6  ((6 * 12) + 3) // 75
#define MIDI_NOTE_E6   ((6 * 12) + 4) // 76
#define MIDI_NOTE_F6   ((6 * 12) + 5) // 77
#define MIDI_NOTE_FS6  ((6 * 12) + 6) // 78
#define MIDI_NOTE_GB6  ((6 * 12) + 6) // 78
#define MIDI_NOTE_G6   ((6 * 12) + 7) // 79
#define MIDI_NOTE_GS6  ((6 * 12) + 8) // 80
#define MIDI_NOTE_AB6  ((6 * 12) + 8) // 80
#define MIDI_NOTE_A6   ((6 * 12) + 9) // 81
#define MIDI_NOTE_AS6  ((6 * 12) + 10) // 82
#define MIDI_NOTE_BB6  ((6 * 12) + 10) // 82
#define MIDI_NOTE_B6   ((6 * 12) + 11) // 83

#define MIDI_NOTE_C7   ((7 * 12) + 0) // 84
#define MIDI_NOTE_CS7  ((7 * 12) + 1) // 85
#define MIDI_NOTE_DB7  ((7 * 12) + 1) // 85
#define MIDI_NOTE_D7   ((7 * 12) + 2) // 86
#define MIDI_NOTE_DS7  ((7 * 12) + 3) // 87
#define MIDI_NOTE_EB7  ((7 * 12) + 3) // 87
#define MIDI_NOTE_E7   ((7 * 12) + 4) // 88
#define MIDI_NOTE_F7   ((7 * 12) + 5) // 89
#define MIDI_NOTE_FS7  ((7 * 12) + 6) // 90
#define MIDI_NOTE_GB7  ((7 * 12) + 6) // 90
#define MIDI_NOTE_G7   ((7 * 12) + 7) // 91
#define MIDI_NOTE_GS7  ((7 * 12) + 8) // 92
#define MIDI_NOTE_AB7  ((7 * 12) + 8) // 92
#define MIDI_NOTE_A7   ((7 * 12) + 9) // 93
#define MIDI_NOTE_AS7  ((7 * 12) + 10) // 94
#define MIDI_NOTE_BB7  ((7 * 12) + 10) // 94
#define MIDI_NOTE_B7   ((7 * 12) + 11) // 95

#define MIDI_NOTE_C8   ((8 * 12) + 0) // 96
#define MIDI_NOTE_CS8  ((8 * 12) + 1) // 97
#define MIDI_NOTE_DB8  ((8 * 12) + 1) // 97
#define MIDI_NOTE_D8   ((8 * 12) + 2) // 98
#define MIDI_NOTE_DS8  ((8 * 12) + 3) // 99
#define MIDI_NOTE_EB8  ((8 * 12) + 3) // 99
#define MIDI_NOTE_E8   ((8 * 12) + 4) // 100
#define MIDI_NOTE_F8   ((8 * 12) + 5) // 101
#define MIDI_NOTE_FS8  ((8 * 12) + 6) // 102
#define MIDI_NOTE_GB8  ((8 * 12) + 6) // 102
#define MIDI_NOTE_G8   ((8 * 12) + 7) // 103
#define MIDI_NOTE_GS8  ((8 * 12) + 8) // 104
#define MIDI_NOTE_AB8  ((8 * 12) + 8) // 104
#define MIDI_NOTE_A8   ((8 * 12) + 9) // 105
#define MIDI_NOTE_AS8  ((8 * 12) + 10) // 106
#define MIDI_NOTE_BB8  ((8 * 12) + 10) // 106
#define MIDI_NOTE_B8   ((8 * 12) + 11) // 107

#define MIDI_NOTE_C9   ((9 * 12) + 0) // 108
#define MIDI_NOTE_CS9  ((9 * 12) + 1) // 109
#define MIDI_NOTE_DB9  ((9 * 12) + 1) // 109
#define MIDI_NOTE_D9   ((9 * 12) + 2) // 110
#define MIDI_NOTE_DS9  ((9 * 12) + 3) // 111
#define MIDI_NOTE_EB9  ((9 * 12) + 3) // 111
#define MIDI_NOTE_E9   ((9 * 12) + 4) // 112
#define MIDI_NOTE_F9   ((9 * 12) + 5) // 113
#define MIDI_NOTE_FS9  ((9 * 12) + 6) // 114
#define MIDI_NOTE_GB9  ((9 * 12) + 6) // 114
#define MIDI_NOTE_G9   ((9 * 12) + 7) // 115
#define MIDI_NOTE_GS9  ((9 * 12) + 8) // 116
#define MIDI_NOTE_AB9  ((9 * 12) + 8) // 116
#define MIDI_NOTE_A9   ((9 * 12) + 9) // 117
#define MIDI_NOTE_AS9  ((9 * 12) + 10) // 118
#define MIDI_NOTE_BB9  ((9 * 12) + 10) // 118
#define MIDI_NOTE_B9   ((9 * 12) + 11) // 119

#define MIDI_NOTE_C10   ((10 * 12) + 0) // 120
#define MIDI_NOTE_CS10  ((10 * 12) + 1) // 121
#define MIDI_NOTE_DB10  ((10 * 12) + 1) // 121
#define MIDI_NOTE_D10   ((10 * 12) + 2) // 122
#define MIDI_NOTE_DS10  ((10 * 12) + 3) // 123
#define MIDI_NOTE_EB10  ((10 * 12) + 3) // 123
#define MIDI_NOTE_E10   ((10 * 12) + 4) // 124
#define MIDI_NOTE_F10   ((10 * 12) + 5) // 125
#define MIDI_NOTE_FS10  ((10 * 12) + 6) // 126
#define MIDI_NOTE_GB10  ((10 * 12) + 6) // 126
#define MIDI_NOTE_G10   ((10 * 12) + 7) // 127

/* @} @} */

#endif /* MIDI_NOTES_H__ */
