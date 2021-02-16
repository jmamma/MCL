module Hypothesis

// TODO pattern note pitch values are standard midi notes
// TODO pattern dump offset 0x216: normal pattern len
// TODO pattern dump offset 0x20f: normal pattern spd, 0x30 is 1/8, 0x3C is 1x
// TODO tracks are store sequentially in a pattern - Track1-4, then FX, then CV;
// TODO every track can have its own pattern scale settings.
// TODO for both trigger and note pitch data, payloads are grouped in 7-byte blocks, separated by flipping 2a 55 2a 55s (acting as sync bytes?)
//      00 in place of 2a/55 means no data in the block
//      so, the raw bytes:  01 40 09 55 41 01 44 09 45 01 48 2a 09 49 01 4c 09 4d 01 55 50 09 51 01 54 09 55 2a
//      are really:         01 40 09    41 01 44 09 45 01 48    09 49 01 4c 09 4d 01    50 09 51 01 54 09 55
//      whether there's a escape sequence is unknown. but see this ---------------------------------------^
//      furthermore, for trigger data, the payloads are again split by 08/09 + 00/01, with 01&09 indicating that a non-empty payload byte should follow.
//      so, the 7byte blks: 01 40 09    41 01 44 09 45 01 48    09 49 01 4c 09 4d 01    50 09 51 01 54 09 55
//      are really:            40       41    44    45    48       49    4c    4d       50    51    54    55
(*
    truth table:

    byte        trigger     mute        accent      ns          ps
    40          0           0           0           0           0
    41          1           0           0           0           0
    44          0           1           0           0           0
    45          1           1           0           0           0
    48          0           0           1           0           0
    49          1           0           1           0           0
    4c          0           1           1           0           0
    4d          1           1           1           0           0
    50          0           0           0           1           0
    51          1           0           0           1           0
    54          0           1           0           1           0
    55          1           1           0           1           0
    ...
    60          0           0           0           0           1
    ...
    70          0           0           0           1           1
    ...
*)

// TODO sound patch: name at 0x18
(* 
     00          04          08          0c          10          14          18          1c          20
     .  .  .  .  .  .  .  .  .  !  .  .  .  .  .  .  .  .  !  .  !  !  !  !  !  !  .  !  !  !  !  !  !  !  .
00   f0 00 20 3c 06 00 53 01 01 00 78 3e 6f 3a 3a 00 00 00 00 05 07 08 00 01 33 30 00 33 43 4c 4f 4e 45 00 00
                                                                             3  0     3  c  l  o  n  e
*)