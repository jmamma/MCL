# Machinedrum Settings Menu

The Machinedrum settings are used to control how the MCL firmware interacts with the MD.


![machinedrum menu](../assets/images/machinedrum_menu.png)


## Import

The **IMPORT** option opens the Machinedrum Pattern Import Menu.

Multiple patterns can be imported sequentially from their starting location **SRC** on the MD to a position **DEST** on the MCL grid. The **COUNT** parameter specifies the number of patterns to be imported.


![machinedrum import](../assets/images/machinedrum_import.png)


## Normalize

When the **NORMALIZE** option is set to AUTO (default), all saved MD tracks have their LEV boosted to 127, and parameters controlling VOL (including parameter locks) are lowered
to compensate. LFOs with destination VOL are
adjusted too.


The resulting track loudness remains the same, but the Track LEV parameter is no longer set arbitrarily. The maximum value of LEV (127), will always be the loudest volume for each track.


When the sequencer is running, the LEV paramater is never transmitted to the MD. This allows the performer to fade tracks in and out of a mix using the LEV parameter.


The performer can then confidently raise the LEV of the track to 127, knowing this is the is the intended maximum loudness for the track.


# Page Setup Menu


<!-- missing-image: page_setup.png -->
> Missing legacy screenshot: `page_setup.png`.


## Grid Encoder:

The **GRID ENCOD** option re-assigns the Grid Page's Encoders to act as the four Performance Controllers.

## RAM Link

The **RAM LINK** option determines whether the RAM Pages should act in MONO or STEREO during recording and playback.


# System Settings Menu


## Display

The display setting is used to enable the OLED display mirror. Using the provided python script _(Note: <https://github.com/jmamma/MCL/tree/master/python>)_ and a USB MIDI connection between the MC and your computer, it is possible to mirror the OLED display to your computer screen.


_Leaving this setting enabled will reduce the performance of the GUI._
