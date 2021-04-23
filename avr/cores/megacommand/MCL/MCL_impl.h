#pragma once

#include "MCL.h"

#include "A4.h"
#include "MD.h"
#include "MNM.h"

#ifdef MEGACOMMAND
#include "MidiSDS.h"
#include "MidiSDSSysex.h"
#endif

#include "Shared.h"
#include "MidiSetup.h"
#include "MidiActivePeering.h"
#include "MidiID.h"
#include "MidiIDSysex.h"
#include "TurboLight.h"
#include "NoteInterface.h"
#include "TrigInterface.h"

#include "MCLGfx.h"
#include "MCLSd.h"
#include "MCLSysConfig.h"
#include "Project.h"
#include "MCLGUI.h"
#include "MCLMemory.h"

#include "Menu.h"
#include "MCLMenus.h"
#include "MenuPage.h"
#include "PolyPage.h"

#ifdef SOUND_PAGE
#include "SoundBrowserPage.h"
#endif

#ifdef LOUDNESS_PAGE
#include "LoudnessPage.h"
#endif

#ifdef WAV_DESIGNER
#include "Osc.h"
#include "OscMixerPage.h"
#include "OscPage.h"
#include "Wav.h"
#include "WavDesigner.h"
#include "WavDesignerPage.h"
#include "DSP.h"
#endif

#include "PageSelectPage.h"
#include "ProjectPages.h"
#include "SeqPages.h"
#include "AuxPages.h"
#include "GridPages.h"

#include "GridEncoder.h"
#include "MCLEncoder.h"

#include "Grid.h"
#include "GridChain.h"
#include "GridRowHeader.h"
#include "GridTask.h"

#include "MCLClipBoard.h"
#include "MDExploit.h"
#include "MDSound.h"
#include "MDTrackSelect.h"

#include "LFO.h"
#include "MDTrack.h"
#include "MDLFOTrack.h"
#include "MDFXTrack.h"
#include "MDRouteTrack.h"
#include "MDTempoTrack.h"

#include "EmptyTrack.h"
#include "LFOSeqTrack.h"
#include "MNMTrack.h"

#include "MCLActions.h"
#include "MCLSeq.h"

