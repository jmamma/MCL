# WAV Designer

WAV designer is a single-cycle waveform generator optimized for the Elektron MD. _To enter the WAV Designer Page: press and hold **[Bank Group]**, then press **[Trig 10]**._

It features a 3 oscillator additive synthesis engine with a level mixer.


Each oscillator can be set to a unique waveform and pitch. The 3 oscillators are then mixed together and rendered to a WAV file which can be transferred to the MD using the MIDI Sample Dump Specification (SDS).


To ensure optimal sample playback, WAV Designer performs all the heavy lifting for you. This involves calculating sample length and rate based on the fundamental frequency, auto detecting loop points and normalizing the waveform.

## Oscillators Pages

Each oscillator can be tuned to a unique frequency. The pitch is adjusted in note increments by rotating **`Encoder 1`**. Rotating **`Encoder 2 `** will adjust the frequency in cents +/-100. **[Exit/No]** will toggle display of the corresponding frequency.


The fundamental frequency of the rendered waveform is automatically selected from the oscillator that has the lowest frequency.


| Control | Assignment |
| --- | --- |
| Encoder 1 | Pitch, note increments. |
| Encoder 2 | Fine Tune, +/- 100 cents |
| Encoder 3 | Pulse Width |
| Encoder 4 | Modify |
| Save / No | Frequency |
| Page | Page Select |
| Load / Yes | -- |
| Shift | Oscillator Menu |


### Oscillator Page Menu


![osc menu](../assets/images/osc_menu.png)


Holding **[Global]** opens the Oscillator Page menu.
Each of the 3 oscillator pages can be accessed via the Oscillator menu's "EDIT" parameter.


| Entry | Function |
| --- | --- |
| EDIT | Oscillator or Mixer Page selection |
| WAV | Oscillator wave shape |


### Waveforms

Every oscillator can be set to a unique waveform chosen from the Oscillator Menu.


The available waveforms are described below:


**SIN:**Sine waveform with adjustable overtones (each overtone is one more octave above the fundamental frequency)


![wav designer sine init](../assets/images/wav_designer_sine_init.png)


![wav designer sin](../assets/images/wav_designer_sin.png)


Overtones are added by using the MD **[Trigs]** and rotating **`Encoder 4`**.
**TRI:** Triangle waveform with adjustable width.


![wav designer tri](../assets/images/wav_designer_tri.png)


**PUL:** Pulse/Square waveform with adjustable width.


![wav designer pulse](../assets/images/wav_designer_pulse.png)


**SAW:**Sawthooth waveform with adjustable width


![wav designer saw](../assets/images/wav_designer_saw.png)


**USR:**User defined waveform, 16 points with linear interpolation.


![wav designer user](../assets/images/wav_designer_user.png)


Sample values are modified by using the MD **[Trigs]** and rotating **`Encoder 4 `**.


### Pulse Width:

TRI, PULSE and SAW waveform's pulse width can be adjusted by rotating **`Encoder 3`**.


![wav designer pulse width](../assets/images/wav_designer_pulse_width.png)


## OSC Mixer Page


![wav designer mixer](../assets/images/wav_designer_mixer.png)


The Oscillator Mixer Page has volume levels for each of the oscillators which can be adjusted by rotating encoders 1 to 3. _Absolute volume levels are not important here as the resulting waveform will be automatically normalized._


| Control | Assignment |
| --- | --- |
| Encoder 1 | Osc1 Level |
| Encoder 2 | Osc2 Level |
| Encoder 3 | Osc3 Level |
| Encoder 4 | |
| Save / No | SubPage Select |
| Page | Page Select |
| Load / Yes | Render+Transfer |
| Shift | OscMixer Menu |


### Oscillator Mixer Page Menu


![oscmixer menu](../assets/images/oscmixer_menu.png)


Holding **[Global]** opens the Oscillator Mixer Page Menu.


| Entry | Function |
| --- | --- |
| EDIT | Oscillator or Mixer Page selection |
| TRANSFER | Render waveform and send to MD |


Use the TRANSFER menu option to render the final waveform. This will open the ROM slot selection menu, and allow you to send the resulting WAV file to a chosen sample slot on the MD.
