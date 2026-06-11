# LFO Page

The MCL firmware is equipped with its own LFO modulation source, and controls both track parameters and master FX parameters.

_LFO settings can be either stored or recalled by saving or loading to the corresponding Auxiliary track LFO slot._


![lfo](../assets/images/lfo.png)


_To enter the LFO Page: press and hold **[Bank Group]**, then press **[Trig 6]**._

| Control | Assignment |
| --- | --- |
| Save / No | Toggle LFO ON/OFF |
| Page | PageSelect |
| Load / Yes | Toggle MOD/DST/OFS |
| Shift | LFO Mode |


By default the LFO engine is deactivated. To toggle it ON, press **[YES]**.

## Modulation Source


To cycle through the Modulation Source subpage press **[Up]**or **[Down]**. The modulation source shape, speed and depth can be controlled. The subpage index will show as ``LFO>MOD'' on the left information panel.


![lfo action](../assets/images/lfo_action.png)


| Control | Assignment |
| --- | --- |
| Encoder 1 | Waveform |
| Encoder 2 | LFO Speed |
| Encoder 3 | Target 1 Depth |
| Encoder 4 | Target 2 Depth |


## Modulation Target

To cycle to the Modulation Target subpage press **[Up]**or **[Down]**. The subpage index will show as ``LFO>DST'' on the left information panel.


![lfo dest](../assets/images/lfo_dest.png)


Only valid parameter types for the current target track will be shown for Encoder2/Encoder4. Master FX machines are regarded as individual tracks, each with its own modulation target parameter types. This extends the mastering capabilities of MD, so that one can use the LFOs to create sidechain-like effects etc.


Destinations tracks M1-M16 correspond to MIDI channels 1 to 16 on port 2. Any MIDI CC parameter (0-127) can be modulated


Destination Params can be learnt by setting DEST to "--" and setting PAR to LER, then turn the desired MD encoder or external MIDI controller.


![lfo learn](../assets/images/lfo_learn.png)


| Control | Assignment |
| --- | --- |
| Encoder 1 | Target Machine 1 |
| Encoder 2 | Param Type 1 |
| Encoder 3 | Target Machine 2 |
| Encoder 4 | Param Type 2 |


## LFO Operation Modes


The LFO engine operates in different modes, namely **FREE**, **TRIG**, **ONESHOT**.
Tap **[Global]** to switch between them, enter **[Trigs]** for LFO resets in TRIG and ONE SHOT modes.


- FREE: Free-running LFO.
- TRIG: The LFO is reset on each active trig.
- ONESHOT: The LFO is reset each active trig but only plays through one cycle


## Parameter Offset


| Control | Assignment |
| --- | --- |
| Encoder 1 | -- |
| Encoder 2 | -- |
| Encoder 3 | Offset 1 |
| Encoder 4 | Offset 2 |


![lfo offset](../assets/images/lfo_offset.png)


The LFO modulation is relative to the destination parameter's original value (the LFO offset). The LFO offset will automatically be updated by adjusting the corresponding parameter on the MD's parameter page.


When the LFO destination is external MIDI (M1->M16) the offset will be MIDI learnt from any matching received CC message. Alternatively the MIDI parameter offsets can be updated manually from the Offset sub page via OFS1 **`Encoder3`** and OFS2 **`Encoder4`**.
