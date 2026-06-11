# Sample Manager Page

Samples can be transferred to and received from the Machinedrum via the Sample Manager Page. Two file types are supported:


- **WAV** -- (UW models only) $.wav$ PCM samples.
- **SYSEX** -- (UW models only) $.syx$ Midi SDS samples, this is the format of the official Machinedrum sound packs.


![sample manager](../assets/images/sample_manager.png)


_To enter the Sound Manager Page: press and hold **[Global]**, then press **[Trig 9]**._


## Navigating the Sound Browser Page


![sample browser page](../assets/images/sample_browser_page.png)


- Use the **[Up]** and **[Down]** arrow keys to scroll through.
- Press **`Enter/Yes`** to enter a directory, or load a sound to the current track.
- Press **`Exit/No`** to exit/cancel.
- Press and hold **`Global`** to create a new folder, delete or rename a sound.


Note that the Sample Manager will filter the directory content for $.wav$ $.syx$ file types.


## Receiving Samples

To receive a **WAV** sample from the MD:


- Select [RECV].
- The file browser panel will now display the sample slots in the MD, with ROM slots first, and RAM slots (shown as R1-R4) in the end.
- Select the slot to receive sample from.
- The SDS dump will be converted to a PCM wave file on the fly, and saved to the current folder on the micro SD card.


## Transferring Samples

To transfer, select the file the from the Sample Manager. The display will now show the ROM slots availble in the MD, and you can select one slot to send the sample to.


![rom select](../assets/images/rom_select.png)


## Bulk Sample Load and Receive

To send or receive multiple WAVs, press and hold **[Global]** to access the **Send All / Recv All** functions which will transfer files from/to the current folder.


- When receiving, samples are saved to the current directory.
- Sample names are prefixed with a 2 digit slot number. This 2 digit number is used to preserve sample order when re-uploading. (Therefore, ROM positions can be saved and recalled for each project.)
- Sample names that do not start with a slot number will be excluded from bulk upload.


## Cancelling Transfer

Sample transfers between can be cancelled by pressing the MD's **[No]** key.


## Delete or Rename Samples


![file menu](../assets/images/file_menu.png)


_From within the Sample Manager, press and hold **[Global]** to access the file options menu._


From the file options menu, you may delete or rename sound files or create new directories.


Use the encoder to make your selection, release **[Global]** to activate your choice.

## Preparing and Transferring Files With a Computer

Using an appropriate adaptor, you can mount the SD card as a drive on your computer.


**WAV** files must meet the following requirements:


- Filename up to 31 characters including extension. (When loaded into ROM, MD will truncate this to the first two and last two characters of the filename before the extension.)
- Sample speeds from 4kHz to 48kHz, ideally 44100Hz or 22050Hz to avoid resampling
- Any bit depth is accepted (MD stores samples at 12bit)


\
MD UW only plays back mono samples.
