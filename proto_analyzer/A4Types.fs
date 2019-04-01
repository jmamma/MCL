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
type FloatParam           = | Value of float // 8bit float encoding with special values like 1/8, 0.625 etc.
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

        // Filter
        F1Freq:           SignedParam
        F1Res:            SignedParam
        F1Overdrive:      SignedParam
        F1KeyTrack:       SignedParam
        F1EnvDepth:       SignedParam

        F2Freq:           SignedParam
        F2Res:            SignedParam
        F2Type:           SpecialUnsignedParam
        F2KeyTrack:       SignedParam
        F2EnvDepth:       SignedParam

        // Amp
        AmpAttack:        UnsignedParam
        AmpDecay:         UnsignedParam
        AmpSustain:       UnsignedParam
        AmpRelease:       UnsignedParam
        AmpShape:         SpecialUnsignedParam
        Chorus:           UnsignedParam
        Delay:            UnsignedParam
        Reverb:           UnsignedParam
        Panning:          SignedParam
        Volume:           UnsignedParam

        // Env1
        Env1Attack:       UnsignedParam
        Env1Decay:        UnsignedParam
        Env1Sustain:      UnsignedParam
        Env1Release:      UnsignedParam
        Env1Shape:        SpecialUnsignedParam
        Env1GateLen:      FloatParam
        Env1DstA:         SpecialUnsignedParam
        Env1DepthA:       SignedParam
        Env1DstB:         SpecialUnsignedParam
        Env1DepthB:       SignedParam

        // Env2
        Env2Attack:       UnsignedParam
        Env2Decay:        UnsignedParam
        Env2Sustain:      UnsignedParam
        Env2Release:      UnsignedParam
        Env2Shape:        SpecialUnsignedParam
        Env2GateLen:      FloatParam
        Env2DstA:         SpecialUnsignedParam
        Env2DepthA:       SignedParam
        Env2DstB:         SpecialUnsignedParam
        Env2DepthB:       SignedParam

        // LFO1
        LFO1Speed:        SignedParam
        LFO1Multiplier:   SpecialUnsignedParam
        LFO1Fade:         SignedParam
        LFO1StartPhase:   UnsignedParam
        LFO1Mode:         SpecialUnsignedParam
        LFO1Wave:         SpecialUnsignedParam
        LFO1DstA:         SpecialUnsignedParam
        LFO1DepthA:       SignedParam
        LFO1DstB:         SpecialUnsignedParam
        LFO1DepthB:       SignedParam

        // LFO2
        LFO2Speed:        SignedParam
        LFO2Multiplier:   SpecialUnsignedParam
        LFO2Fade:         SignedParam
        LFO2StartPhase:   UnsignedParam
        LFO2Mode:         SpecialUnsignedParam
        LFO2Wave:         SpecialUnsignedParam
        LFO2DstA:         SpecialUnsignedParam
        LFO2DepthA:       SignedParam
        LFO2DstB:         SpecialUnsignedParam
        LFO2DepthB:       SignedParam

        // Note
        MainNote: NoteParam
        NoteVel:  UnsignedParam
        NoteLen:  FloatParam
        // uTiming, TRC, ENV, ENV, LFO, LFO are only for P-LOCK.

        //Arp
        ArpMode: SpecialUnsignedParam
        ArpSpeed: UnsignedParam // up to 96
        ArpRange: UnsignedParam // [1..8]
        ArpNoteLength: FloatParam
        ArpLegato: BoolParam
        ArpNote2: SignedParam
        ArpNote3: SignedParam
        ArpNote4: SignedParam
        ArpLen:   UnsignedParam // [1..16]
        ArpSeq:   (BoolParam * UnsignedParam) [] // 16x

        // 136 items up to now

        Name: string // x15

        // 151 items up to now
    }

type Kit = 
    {
        Tracks: Sound array
        //Perf x10
        PerfKnobs: PerfKnob[]
        // TODO other settings?
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

type Track = 
    {
        Notes: Trigger array
        Scale: PatternScale
        Sound: Sound
    }

type Pattern =
    {
        Tracks:   Track array
        Accent:   UnsignedParam
        Swing:    UnsignedParam
        SwingSeq: BoolParam[] // x16
        Scale:    PatternScale

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
        Tracks   = Array.map (fun x -> { Notes = x; Scale = default_scale; Sound = default_sound } ) tracks
        Accent   = byte 32
        Scale    = default_scale
        Swing    = byte 50
        SwingSeq = Array.init 16 (fun i -> i % 2 = 0)
    }
