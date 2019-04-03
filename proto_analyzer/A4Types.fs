module A4Types

open System
open System

type PatternBank          = char
type SoundBank            = char

// param types
type SignedParam          = sbyte
type UnsignedParam        = byte
type SpecialUnsignedParam = byte
type NoteParam            = byte
type NoteLenParam         = | Value of float // 8bit float encoding with special values like 1/8, 0.625 etc.
type TimeLenParam         = | Value of float // 8bit float encoding with doubling steps
type DoubleParam          = | Value of float // 16bit fixed-point, range=0~127 step = 128 / 16384
type SDoubleParam         = | Value of float // 16bit signed fixed-point, range=-128~127 step = 256 / 16384
type BoolParam            = bool
type PatternSpeed         = | PS_1x | PS_3_2x | PS_2x | PS_3_4x | PS_1_2x | PS_1_4x | PS_1_8x
type Waveform             = | W_SAW | W_TRP | W_PUL | W_TRI | W_INL | W_INR | W_FDB | W_OFF
type Sub                  = | S_OFF | S_1OCT | S_2OCT | S_2PUL | S_5TH
type SyncMode             = | SM_OFF | SM_1TO2 | SM_2TO1 | SM_MET
type FilterType           = | FT_LP1 | FT_LP2 | FT_BP | FT_HP1 | FT_HP2 | FT_BS | FT_PK
type ShapeType            = | ST_0 | ST_1 | ST_2 | ST_3 | ST_4 | ST_5 | ST_6 | ST_7 | ST_8 | ST_9 | ST_10 | ST_11

type PerfKnob =
    {
        Value:   UnsignedParam
        Configs: (SpecialUnsignedParam * SignedParam) [] // x5
        Bipolar: BoolParam
        Name:    string //x15
    }

// assume all special params are just "special unsigned"
type Sound = 
    {
        // Osc1
        Osc1Tuning:       SignedParam
        Osc1Fine:         SignedParam
        Osc1Detune:       SignedParam
        Osc1KeyTracking:  BoolParam
        Osc1Level:        UnsignedParam
        Osc1Waveform:     SpecialUnsignedParam
        Osc1PulseWidth:   SignedParam
        Osc1PWMSpeed:     UnsignedParam
        Osc1PWMDepth:     UnsignedParam

        // Noise
        NoiseSH:          UnsignedParam
        NoiseColor:       SignedParam
        NoiseFade:        SignedParam
        NoiseLevel:       UnsignedParam

        // Osc2
        Osc2Tuning:       SignedParam
        Osc2Fine:         SignedParam
        Osc2Detune:       SignedParam
        Osc2KeyTracking:  BoolParam
        Osc2Level:        UnsignedParam
        Osc2Waveform:     SpecialUnsignedParam
        Osc2PulseWidth:   SignedParam
        Osc2PWMSpeed:     UnsignedParam
        Osc2PWMDepth:     UnsignedParam

        // 22 items to now
        // Osc common
        Osc1AM:           BoolParam
        SyncMode:         SpecialUnsignedParam
        SyncAmount:       UnsignedParam
        BendDepth:        SignedParam
        NoteSlideTime:    UnsignedParam
        Osc2AM:           BoolParam
        OscRetrig:        BoolParam
        VibratoFade:      SignedParam
        VibratoSpeed:     UnsignedParam
        VibratoDepth:     UnsignedParam

        // 32 items to now
        // Filter
        F1Freq:           DoubleParam
        F1Res:            UnsignedParam
        F1Overdrive:      SignedParam
        // Value 32 = note intervals
        F1KeyTrack:       SignedParam
        F1EnvDepth:       SignedParam

        F2Freq:           DoubleParam
        F2Res:            UnsignedParam
        F2Type:           SpecialUnsignedParam
        // Value 32 = note intervals
        F2KeyTrack:       SignedParam
        F2EnvDepth:       SignedParam

        // 42 items to now
        // added after getting 42 sounds in the pool...
        Osc1Sub:          SpecialUnsignedParam
        Osc2Sub:          SpecialUnsignedParam

        // 44 items to now
        // Amp
        AmpAttack:        UnsignedParam
        AmpDecay:         UnsignedParam
        AmpSustain:       UnsignedParam
        AmpRelease:       UnsignedParam
        AmpShape:         SpecialUnsignedParam
        Chorus:           UnsignedParam
        Delay:            UnsignedParam
        Reverb:           UnsignedParam
        AmpLevel:         UnsignedParam
        Panning:          SignedParam
        Accent:           UnsignedParam

        // 55 items to now
        // EnvF = Filter Env
        EnvFltAttack:     UnsignedParam
        EnvFltDecay:      UnsignedParam
        EnvFltSustain:    UnsignedParam
        EnvFltRelease:    UnsignedParam
        EnvFltShape:      SpecialUnsignedParam
        EnvFltGateLen:    TimeLenParam
        EnvFltDstA:       SpecialUnsignedParam
        EnvFltDepthA:     SDoubleParam
        EnvFltDstB:       SpecialUnsignedParam
        EnvFltDepthB:     SDoubleParam

        // 65 items to now
        // Env2
        Env2Attack:       UnsignedParam
        Env2Decay:        UnsignedParam
        Env2Sustain:      UnsignedParam
        Env2Release:      UnsignedParam
        Env2Shape:        SpecialUnsignedParam
        Env2GateLen:      TimeLenParam
        Env2DstA:         SpecialUnsignedParam
        Env2DepthA:       SDoubleParam
        Env2DstB:         SpecialUnsignedParam
        Env2DepthB:       SDoubleParam

        // 75 items to now
        // LFO1
        LFO1Speed:        SignedParam
        LFO1Multiplier:   SpecialUnsignedParam
        LFO1Fade:         SignedParam
        LFO1StartPhase:   UnsignedParam
        LFO1Mode:         SpecialUnsignedParam
        LFO1Wave:         SpecialUnsignedParam
        LFO1DstA:         SpecialUnsignedParam
        LFO1DepthA:       SDoubleParam
        LFO1DstB:         SpecialUnsignedParam
        LFO1DepthB:       SDoubleParam

        // LFO2
        LFO2Speed:        SignedParam
        LFO2Multiplier:   SpecialUnsignedParam
        LFO2Fade:         SignedParam
        LFO2StartPhase:   UnsignedParam
        LFO2Mode:         SpecialUnsignedParam
        LFO2Wave:         SpecialUnsignedParam
        LFO2DstA:         SpecialUnsignedParam
        LFO2DepthA:       SDoubleParam
        LFO2DstB:         SpecialUnsignedParam
        LFO2DepthB:       SDoubleParam

        //95 items to now

        // added after 95 sounds in pool..
        OscDrift:         BoolParam
        F1ResBoost:       BoolParam
        Legato:           BoolParam
        PortaMode:        SpecialUnsignedParam
        Vel2Vol:          SpecialUnsignedParam

        //100 items to now

        // VelMod
        VelModBipolar:    BoolParam
        // patch 102: dst ramping, depth all 0
        // ramping order follows A4 vst menu listing
        // patch 103: dst all None, depth ramping
        VelModDst1:       SpecialUnsignedParam
        VelModDepth1:     SignedParam
        VelModDst2:       SpecialUnsignedParam
        VelModDepth2:     SignedParam
        VelModDst3:       SpecialUnsignedParam
        VelModDepth3:     SignedParam
        VelModDst4:       SpecialUnsignedParam
        VelModDepth4:     SignedParam
        VelModDst5:       SpecialUnsignedParam
        VelModDepth5:     SignedParam

        // ATouchMod
        ATouchModDst1:    SpecialUnsignedParam
        ATouchModDepth1:  SignedParam
        ATouchModDst2:    SpecialUnsignedParam
        ATouchModDepth2:  SignedParam
        ATouchModDst3:    SpecialUnsignedParam
        ATouchModDepth3:  SignedParam
        ATouchModDst4:    SpecialUnsignedParam
        ATouchModDepth4:  SignedParam
        ATouchModDst5:    SpecialUnsignedParam
        ATouchModDepth5:  SignedParam

        // MWMod = modwheel mod
        MWModDst1:        SpecialUnsignedParam
        MWModDepth1:      SignedParam
        MWModDst2:        SpecialUnsignedParam
        MWModDepth2:      SignedParam
        MWModDst3:        SpecialUnsignedParam
        MWModDepth3:      SignedParam
        MWModDst4:        SpecialUnsignedParam
        MWModDepth4:      SignedParam
        MWModDst5:        SpecialUnsignedParam
        MWModDepth5:      SignedParam

        // PBMod = pitchbend mod
        PBModDst1:        SpecialUnsignedParam
        PBModDepth1:      SignedParam
        PBModDst2:        SpecialUnsignedParam
        PBModDepth2:      SignedParam
        PBModDst3:        SpecialUnsignedParam
        PBModDepth3:      SignedParam
        PBModDst4:        SpecialUnsignedParam
        PBModDepth4:      SignedParam
        PBModDst5:        SpecialUnsignedParam
        PBModDepth5:      SignedParam

        // BreadthMod
        BreadthModDst1:   SpecialUnsignedParam
        BreadthModDepth1: SignedParam
        BreadthModDst2:   SpecialUnsignedParam
        BreadthModDepth2: SignedParam
        BreadthModDst3:   SpecialUnsignedParam
        BreadthModDepth3: SignedParam
        BreadthModDst4:   SpecialUnsignedParam
        BreadthModDepth4: SignedParam
        BreadthModDst5:   SpecialUnsignedParam
        BreadthModDepth5: SignedParam

        // 151 items to now

        Name: string // x15

        // 152 items up to now
    }

// assume triggers themselves are p-locks
type Trigger =
    {
        Active:     BoolParam
        Note:       NoteParam
        Mute:       BoolParam
        Accent:     BoolParam
        NoteSlide:  BoolParam
        ParamSlide: BoolParam
    }

type PatternScale =
    | Normal   of int * PatternSpeed
    | Advanced of Tracks: (int * PatternSpeed) array * MasterLen: int * MasterChange: int * MasterSpeed: PatternSpeed

type TrackSettings = 
    {
        Sound: Sound
        // Note
        MainNote: NoteParam
        NoteVel:  UnsignedParam
        NoteLen:  NoteLenParam
        // uTiming, TRC, ENV, ENV, LFO, LFO are only for P-LOCK.

        //Arp
        ArpMode:  SpecialUnsignedParam
        ArpSpeed: UnsignedParam // up to 96
        ArpRange: UnsignedParam // [1..8]
        ArpNoteLength: NoteLenParam
        ArpLegato: BoolParam
        ArpNote2: SignedParam
        ArpNote3: SignedParam
        ArpNote4: SignedParam
        ArpLen:   UnsignedParam // [1..16]
        ArpSeq:   (BoolParam * UnsignedParam) [] // 16x
    }

type Sequence = 
    {
        Notes: Trigger array
        Scale: PatternScale
    }

type Kit = 
    {
        Tracks: TrackSettings array
        //Perf x10
        PerfKnobs: PerfKnob[]
        // TODO other settings?
    }

type Pattern =
    {
        Kit:       Kit
        Sequences: Sequence array
        Swing:     UnsignedParam
        SwingSeq:  BoolParam[] // x16
        Scale:     PatternScale

        // TODO others?
    }

type Global =
    {
        SynthMasterTune: float
        // TODO
    }

type Source =
| Unknown
| Kit of int * Kit
| Pattern of PatternBank * int * Pattern
| Sound of SoundBank * int * Sound
| Global of int * Global

let C4 = byte 0x3C
let C5 = byte 0x48

let o = 
    {
        Active = false
        Note = C4
        Mute = false
        Accent = false
        NoteSlide = false
        ParamSlide = false
    }

let mknote note = 
    {
        Active = true
        Note = byte note
        Mute = false
        Accent = false
        NoteSlide = false
        ParamSlide = false
    }

let default_scale = Normal(16, PS_1x)
let scale_32 = Normal(32, PS_1x)
let scale_64 = Normal(64, PS_1x)
let default_sound = Unchecked.defaultof<Sound>
let default_kit = Unchecked.defaultof<Kit>

// make a pattern on track 1
type mkpattern_param =
| Track1 of Trigger list
| Tracks of Trigger list list

let mkpattern (notes: mkpattern_param) = 
    let tracks = 
        match notes with
        | Track1 xs -> [| Array.ofList xs |]
        | Tracks ts -> Array.ofList <| List.map Array.ofList ts

    {
        Kit   = default_kit
        Sequences = Array.map (fun x -> { Notes = x; Scale = default_scale; } ) tracks
        Scale    = default_scale
        Swing    = byte 50
        SwingSeq = Array.init 16 (fun i -> i % 2 = 0)
    }



let SoundPatches = [|
        // Osc1
        "Osc1Tuning:       SignedParam"
        "Osc1Fine:         SignedParam"
        "Osc1Detune:       SignedParam"
        "Osc1KeyTracking:  BoolParam"
        "Osc1Level:        UnsignedParam"
        "Osc1Waveform:     SpecialUnsignedParam"
        "Osc1PulseWidth:   SignedParam"
        "Osc1PWMSpeed:     UnsignedParam"
        "Osc1PWMDepth:     UnsignedParam"

        // Noise
        "NoiseSH:          UnsignedParam"
        "NoiseColor:       SignedParam"
        "NoiseFade:        SignedParam"
        "NoiseLevel:       UnsignedParam"

        // Osc2
        "Osc2Tuning:       SignedParam"
        "Osc2Fine:         SignedParam"
        "Osc2Detune:       SignedParam"
        "Osc2KeyTracking:  BoolParam"
        "Osc2Level:        UnsignedParam"
        "Osc2Waveform:     SpecialUnsignedParam"
        "Osc2PulseWidth:   SignedParam"
        "Osc2PWMSpeed:     UnsignedParam"
        "Osc2PWMDepth:     UnsignedParam"

        // 22 items to now
        // Osc common
        "Osc1AM:           BoolParam"
        "SyncMode:         SpecialUnsignedParam"
        "SyncAmount:       UnsignedParam"
        "BendDepth:        SignedParam"
        "NoteSlideTime:    UnsignedParam"
        "Osc2AM:           BoolParam"
        "OscRetrig:        BoolParam"
        "VibratoFade:      SignedParam"
        "VibratoSpeed:     UnsignedParam"
        "VibratoDepth:     UnsignedParam"

        // 32 items to now
        // Filter
        "F1Freq:           DoubleParam"
        "F1Res:            UnsignedParam"
        "F1Overdrive:      SignedParam"
        // Value 32 = note intervals
        "F1KeyTrack:       SignedParam"
        "F1EnvDepth:       SignedParam"

        "F2Freq:           DoubleParam"
        "F2Res:            UnsignedParam"
        "F2Type:           SpecialUnsignedParam"
        // Value 32 = note intervals
        "F2KeyTrack:       SignedParam"
        "F2EnvDepth:       SignedParam"

        // 42 items to now
        // added after getting 42 sounds in the pool...
        "Osc1Sub:          SpecialUnsignedParam"
        "Osc2Sub:          SpecialUnsignedParam"

        // 44 items to now
        // Amp
        "AmpAttack:        UnsignedParam"
        "AmpDecay:         UnsignedParam"
        "AmpSustain:       UnsignedParam"
        "AmpRelease:       UnsignedParam"
        "AmpShape:         SpecialUnsignedParam"
        "Chorus:           UnsignedParam"
        "Delay:            UnsignedParam"
        "Reverb:           UnsignedParam"
        "AmpLevel:         UnsignedParam"
        "Panning:          SignedParam"
        "Accent:           UnsignedParam"

        // 55 items to now
        // EnvF = Filter Env
        "EnvFltAttack:     UnsignedParam"
        "EnvFltDecay:      UnsignedParam"
        "EnvFltSustain:    UnsignedParam"
        "EnvFltRelease:    UnsignedParam"
        "EnvFltShape:      SpecialUnsignedParam"
        "EnvFltGateLen:    TimeLenParam"
        "EnvFltDstA:       SpecialUnsignedParam"
        "EnvFltDepthA:     SDoubleParam"
        "EnvFltDstB:       SpecialUnsignedParam"
        "EnvFltDepthB:     SDoubleParam"

        // 65 items to now
        // Env2
        "Env2Attack:       UnsignedParam"
        "Env2Decay:        UnsignedParam"
        "Env2Sustain:      UnsignedParam"
        "Env2Release:      UnsignedParam"
        "Env2Shape:        SpecialUnsignedParam"
        "Env2GateLen:      TimeLenParam"
        "Env2DstA:         SpecialUnsignedParam"
        "Env2DepthA:       SDoubleParam"
        "Env2DstB:         SpecialUnsignedParam"
        "Env2DepthB:       SDoubleParam"

        // 75 items to now
        // LFO1
        "LFO1Speed:        SignedParam"
        "LFO1Multiplier:   SpecialUnsignedParam"
        "LFO1Fade:         SignedParam"
        "LFO1StartPhase:   UnsignedParam"
        "LFO1Mode:         SpecialUnsignedParam"
        "LFO1Wave:         SpecialUnsignedParam"
        "LFO1DstA:         SpecialUnsignedParam"
        "LFO1DepthA:       SDoubleParam"
        "LFO1DstB:         SpecialUnsignedParam"
        "LFO1DepthB:       SDoubleParam"

        // LFO2
        "LFO2Speed:        SignedParam"
        "LFO2Multiplier:   SpecialUnsignedParam"
        "LFO2Fade:         SignedParam"
        "LFO2StartPhase:   UnsignedParam"
        "LFO2Mode:         SpecialUnsignedParam"
        "LFO2Wave:         SpecialUnsignedParam"
        "LFO2DstA:         SpecialUnsignedParam"
        "LFO2DepthA:       SDoubleParam"
        "LFO2DstB:         SpecialUnsignedParam"
        "LFO2DepthB:       SDoubleParam"

        //95 items to now

        // added after 95 sounds in pool..
        "OscDrift:         BoolParam"
        "F1ResBoost:       BoolParam"
        "Legato:           BoolParam"
        "PortaMode:        SpecialUnsignedParam"
        "Vel2Vol:          SpecialUnsignedParam"

        //100 items to now

        // VelMod
        "VelModBipolar:    BoolParam"
        "patch 102: dst ramping, depth all 0; ramping order follows A4 vst menu listing"
        "patch 103: dst all None, depth ramping"

|]