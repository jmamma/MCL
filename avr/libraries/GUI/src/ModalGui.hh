/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MODALGUI_H__
#define MODALGUI_H__

#include "WProgram.h"
#include "GUI.h"

/**
 * \addtogroup GUI
 *
 * @{
 *
 * \addtogroup gui_modal Modal GUIs
 *
 * @{
 *
 * \file
 * Modal GUIs
 **/


#define ALL_ENCODER_MASK (_BV(ButtonsClass::ENCODER1) | _BV(ButtonsClass::ENCODER2) | _BV(ButtonsClass::ENCODER3) | _BV(ButtonsClass::ENCODER4))

#define ALL_BUTTON_MASK (_BV(ButtonsClass::BUTTON1) | _BV(ButtonsClass::BUTTON2) | _BV(ButtonsClass::BUTTON3) | _BV(ButtonsClass::BUTTON4))

#define ALL_BUTTON_ENCODER_MASK (ALL_ENCODER_MASK | ALL_BUTTON_MASK)

int showModalGui(char *line1, char *line2, uint16_t buttonMask, uint16_t releaseMask = 0);
int showModalGui_p(PGM_P line1, PGM_P line2, uint16_t buttonMask, uint16_t releaseMask = 0);

char *getNameModalGui(char *line1, char *initName);

#endif /* MODALGUI_H__ */
