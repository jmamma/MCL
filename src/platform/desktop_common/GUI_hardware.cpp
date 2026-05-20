#include "GUI_hardware.h"
#include "global.h"

GUIHardware  GUI_hardware;
SR165Class   SR165;
EncodersClass Encoders;
ButtonsClass  Buttons;
uint64_t mcl_desktop_button_mask = 0;

void ButtonsClass::clear() {
    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; i++) {
        CLEAR_B_DOUBLE_CLICK(i);
        CLEAR_B_CLICK(i);
        CLEAR_B_LONG_CLICK(i);
        STORE_B_OLD(i, B_CURRENT(i));
    }
}

void ButtonsClass::poll(uint8_t /*sr*/) {
    const uint64_t mask = mcl_desktop_button_mask;
    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; i++) {
        const bool pressed = (mask & (1ULL << i)) != 0;
        STORE_B_CURRENT(i, pressed ? 0 : 1);
    }
}

void GUIHardware::init() {
    Buttons.clear();
    Buttons.poll(0);
    Encoders.clearEncoders();
}

void GUIHardware::poll() {
    if (inGui) return;
    inGui = true;
    Buttons.clear();
    Buttons.poll(0);
    Encoders.poll(0);
    GUI.events.pollEvents();
    led.show();
    inGui = false;
}

void GUIHardware::clear() {
    Buttons.clear();
    Encoders.clearEncoders();
}
