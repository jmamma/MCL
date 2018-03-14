/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MIXERPAGE_H__
#define MIXERPAGE_H__
#include "GUI.h"

class MixerPage : LightPage {
 public:
 MixerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 virtual bool handleEvent(gui_event_t *event);
};

extern MixerPage mixer_page;
#endif /* MIXERPAGE_H__ */
