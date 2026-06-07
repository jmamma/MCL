/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef FXPAGE_H__
#define FXPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "PageIndex.h"

#define NUM_FX_PAGES 2
typedef struct fx_param_t {
  uint8_t type;
  uint8_t param;
} fx_param_t;

//params_ 2 dimensional array, consisting of [MD_FX_TYPE][MD_FX_PARAM_NUMBER]
//
class FXPage : public LightPage {
public:
  FXPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL, fx_param_t *params_ = NULL,
          const char* title = NULL, uint8_t page_id_ = 0)
      : LightPage(e1, e2, e3, e4) {
      page_id = page_id_;
      params = params_;
      fx_page_title = title ? title : "";
  }

  static PageIndex last_page;

  bool handleEvent(gui_event_t *event) override;

  const char *fx_page_title;
  fx_param_t *params;

  bool page_mode;
  uint8_t page_id;

  void display() override;
  void setup() override;
  void init() override;
  void loop() override;
  void cleanup() override;

  void update_encoders();
  virtual bool moveEncoderFocusPage(int8_t direction) override;

};

extern MCLEncoder fx_page_param1;
extern MCLEncoder fx_page_param2;
extern MCLEncoder fx_page_param3;
extern MCLEncoder fx_page_param4;

#endif /* FXPAGE_H__ */
