#include "GridEncoder.h"
#include "MCL.h"

int GridEncoder::update(encoder_t *enc) {

  int inc = enc->normal +
            (pressmode ? 0 : (scroll_fastmode ? 4 * enc->button : enc->button));
  // int inc = 4 + (pressmode ? 0 : (fastmode ? 5 * enc->button : enc->button));

  rot_counter += enc->normal;
  if (rot_counter > rot_res) {
    cur = limit_value(cur, inc, min, max);
    rot_counter = 0;
  } else if (rot_counter < 0) {
    cur = limit_value(cur, inc, min, max);
    rot_counter = rot_res;
  }

  return cur;
}

void GridEncoder::displayAt(int i) {

  const char *str;
  /*Calculate the position of the Grid to be displayed based on the Current Row,
   * Column and Encoder*/
  // int value = displayx + (displayy * 16) + i;

  GUI.setLine(GUI.LINE1);

  char a4_name2[3] = "TK";

  char strn[3] = "--";
  // A4Track track_buf;
  uint8_t model = grid_page.grid_models[getValue() + i];

  // getGridModel(encoders[1]->getValue() + i, encoders[2]->getValue(), true, (A4Track*)
  // &track_buf);

  /*Retrieve the first 2 characters of Maching Name associated with the Track at
   * the current Grid. First obtain the Model object from the Track object, then
   * convert the MachineType into a string*/
  if (getValue() + i < 16) {
    str = getMachineNameShort(model, 1);

    if (str == NULL) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      GUI.put_p_string_at((0 + (i * 3)), str);
    }
  } else {
    if (model == EMPTY_TRACK_TYPE) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      if (model == A4_TRACK_TYPE) {
        char a4_name1[3] = "A4";
        GUI.put_string_at((0 + (i * 3)), a4_name1);
      }
      if (model == EXT_TRACK_TYPE) {
        char ex_name1[3] = "EX";
        GUI.put_string_at((0 + (i * 3)), ex_name1);
      }
    }
  }

  GUI.setLine(GUI.LINE2);
  str = NULL;

  if (getValue() + i < 16) {

    str = getMachineNameShort(model, 2);

    if (str == NULL) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      GUI.put_p_string_at((0 + (i * 3)), str);
    }
  }

  else {
    if (model == EMPTY_TRACK_TYPE) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      GUI.put_string_at((0 + (i * 3)), a4_name2);
      GUI.put_value_at1(1 + (i * 3), getValue() + i - 15);
    }
  }
  redisplay = false;
}
