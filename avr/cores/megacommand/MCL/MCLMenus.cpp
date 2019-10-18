#include "MCLMenus.h"

MCLEncoder options_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder config_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param3(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param4(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param5(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param6(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param7(0, 17, ENCODER_RES_SYS);

MenuPage aux_config_page(&auxconfig_menu_layout, &config_param1, &config_param6);
MenuPage system_page(&system_menu_layout, &options_param1, &options_param2);
MenuPage midi_config_page(&midiconfig_menu_layout, &config_param1,
                          &config_param3);
MenuPage md_config_page(&mdconfig_menu_layout, &config_param1, &config_param4);
MenuPage chain_config_page(&chain_menu_layout, &config_param1, &config_param6);
MenuPage mcl_config_page(&mclconfig_menu_layout, &config_param1,
                         &config_param5);
MenuPage ram_config_page(&rampage1_menu_layout, &config_param1,
                         &config_param7);


MCLEncoder input_encoder1(0, 127, ENCODER_RES_SYS);
MCLEncoder input_encoder2(0, 127, ENCODER_RES_SYS);

TextInputPage text_input_page(&input_encoder1, &input_encoder2);
