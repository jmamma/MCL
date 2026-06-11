# Route Page

The Route Page is used to direct the audio of a specific MD track to a selected output.

_Routing configuration can be either stored or recalled by saving or loading to the corresponding Auxiliary track Route slot._


![route](../assets/images/route.png)


_The Route page can be accessed by pressing **[Bank Group]** and **[Trig 4]**._


| Control | Assignment |
| --- | --- |
| Encoder 1 | Output Selection |
| Encoder 2 | -- |
| Encoder 3 | Quantization |
| Encoder 4 | -- |
| Save / No | -- |
| Page | PageSelect |
| Load / Yes | -- |
| Shift | -- |


**`Encoder 1`** can be used to select the audio output destination. The destination can be one of MD's external outputs C, D, E, F.

The MD's **[Trig]** keys are used to toggle the output of selected tracks, between Main Outputs and the chosen external output.


![route action](../assets/images/route_action.png)


## Quantization Rules

The quantization rules specify the timing of the routing changes so that they are in sync with the sequencers.

The minimum quantization value is 2 steps. Quantization values can be changed by adjusting
the ”Q:” parameter, and increase in powers of 2. Holding down the encoder button will step
increment the quantization value.


- --: No quantization.
- 02: Toggle cue on next possible 2nd step
- 04: Toggle cue on next possible 4th step
- 08: Toggle cue on next possible 8th step
- 16: Toggle cue on next possible 16th step
- 32: Toggle cue on next possible 32th step
- 64: Toggle cue on next possible 64th step


## Grid Saving or Loading

The routing configuration can be saved and loaded from Grid Y in Auxiliary slot 14 (Route) along with Polyphonic configuration. In addition, routing is automatically retained across power on/off.


_For more information on slot positions and their corresponding tracks see "Sequencer: Saving and Loading"._
