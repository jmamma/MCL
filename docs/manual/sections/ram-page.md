# RAM Page

A RAM page is used to automate the recording and playback of the MD's RAM machines. The MCL firmware is equipped with two RAM Pages.


A RAM page may be used to sample the MD main outputs or the External Inputs in either Mono or Stereo. The recorded loop can then be played.
Slicing with the various 'slice modes' allows for interesting musical and performance effects.


RAM Page 1 will record and playback on MD track 15

RAM Page 2 will record and playback on MD track 16


_When stereo mode is enabled from **Configuration-->Aux Pages-->RAMPage**, the recording and playback of RAM machines on track 15 + 16 are linked._


![ram1](../assets/images/ram1.png)


To enter RAM Page one or two:
Press and hold
**[Bank Group]**, then press **[Trig 15]** or **[Trig 16]**.


![ram1 action](../assets/images/ram1_action.png)


| Control | Assignment |
| --- | --- |
| Encoder 1 | Record Source |
| Encoder 2 | Slice Modulation |
| Encoder 3 | Number of Slices |
| Encoder 4 | Record/Playback Length |


| Control | Assignment |
| --- | --- |
| Save / No | Record |
| Page | PageSelect |
| Load / Yes | Play |
| Shift | Slice Mode Toggle |


## Recording


To use the RAM Machines, first start playing a pattern on the MD.
Leave tracks 15 + 16 available as these will be overwritten for RAM machine sampling and playback.
From the RAM Page, choose the recording source **`Encoder1`**, INT for sampling the MD internal sound engine or A/B from the respective external input. Select the recording length **`Encoder4`** and press **[Enter/Yes]** to initiate recording. Recording is quantized with respect to the chosen length.
- The status text "Queue" will be displayed indicating that record event is about to occur. The status text "Recording" will be displayed indicating that the RAM machines are recording. Recording is continuous and can only be stopped by initiating playback.


## Playback


- Press **[Exit/No]** to initiate playback. The status text "Queue" will be displayed indicating that play event is about to occur.
- The status text "Playback" will be displayed indicating that the RAM machine(s) are playing. Playback is continuous until recording is re-initiated.


## Mono

When in Mono mode RAM machines can either sample a mono MIX of the MD output, or external inputs A or B

## Stereo

When in Stereo mode RAM machine one will record MD output A and RAM machine 2 will record MD output B. Similarly if the recording source is set to EXT RAM machine one will record EXT input A and RAM machine 2 will record EXT input B

On playback stereo tracks are panned and linked. Parameter changes across tracks 15 and 16 are synchronised.


## Slice

Slicing will divide the recorded sample in to N slices, and set up a playback trigger for each slice. It does this by setting start/end parameters locks for each trig/slice. The default number of slices is 1.


To slice the recorded sample, select the number of slices using **`Encoder 3`** and then press **[Exit/No]**.


## Dice


The dice modes affect how slices are played back.


- 0: Reverse slices
- 1-4: Reverse every nth slice
- 5: Play slices in backwards order
- 6: Play slices in random order


To dice the recorded sample, select the dice mode using **`Encoder 2`** and then press **[Global]**.
