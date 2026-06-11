# Polyphonic Mode


## Voice Select Page

The voice select page is used to allocate MD tracks as polyphonic voices.


![voice select page](../assets/images/voice_select_page.png)


Access the voice select page via the Chromatic Page's Track Menu item **"Polyphony"**

## Voice Assignment

MD track's 1-16 can be assigned as polyphonic voices by pressing the matching **[Trig]** keys.

## Grid Saving or Loading

The polyphonic voice selection can be saved and loaded from Grid Y in Auxiliary slot position 14, labelled "RT (Route)".


Last loaded Polyphonic voice selection is automatically retained across power on/off.


_For more information on slot positions and their corresponding tracks see "Sequencer: Saving and Loading"._

## Voice Modes


When in the Chromatic Page if the current active track is part of the Polyphonic track selection POLY mode will be activated. In this mode the MD will be played polyphonically using voices selected from the POLY Page.


When POLY mode is active, tracks become 'linked'. Track length via the Track menu and parameter changes will be synchronised across the voices.


Combinations of voice and NFX machines can be simultaneously set in POLY mode. MCL separates the synthesis pages of the POLY channels linking only matching machines, maintaining synchronisation over the effects and routing pages.


## Getting started with Poly Mode

Enter the Chromatic page, and via the Track Menu, open the Polyphony page to assign the designated tracks as polyphonic voices.


For best results, make sure that sound generating voices designated as a poly voice are all set to the same machine type on the MD.
_You can quickly copy one track to another using the MD's **[Copy ] / [Paste]** functions._


From the Chromatic Page, Press the **[Trig]** buttons to be begin playing the MD polyphonically.


Use the MD’s **[Rec] + [Play]** function to enable live record mode. Sequence data will be stored across the POLY tracks.


All POLY tracks can be quickly cleared using the MD’s **[Clear]** function. (repeat to UNDO)

## PolyMode External MIDI

The MD can be played polyphonically using an attached MIDI keyboard/sequencer connected to MIDI input on Port 2 or via USB MIDI. Poly channels can be played from this channel regardless of current track selection.


To enable/disable control from an external device you must:


- Ensure that the **Config-->MIDI-->CTRL PORT** is set to the MIDI port that your external keyboard/sequencer is connected.
- Set **Config-->MIDI-->MD MIDI-->POLY CHAN** from INT (internal) to a desired MIDI channel or OMNI (all channels).


**MIDI CC:**


You can control the voice parameters by sending MIDI CC messages via an External MIDI controller to port 2.

CC 16 to 39 control MD parameters 1 to 24 on the active track, or across all polyphonic tracks.
