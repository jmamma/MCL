#pragma once
__T_icons_boot *icons_boot;
void use_icons_boot() { icons_boot = (__T_icons_boot*) __use_resource(__R_icons_boot); }
__T_icons_device *icons_device;
void use_icons_device() { icons_device = (__T_icons_device*) __use_resource(__R_icons_device); }
__T_icons_knob *icons_knob;
void use_icons_knob() { icons_knob = (__T_icons_knob*) __use_resource(__R_icons_knob); }
__T_icons_logo *icons_logo;
void use_icons_logo() { icons_logo = (__T_icons_logo*) __use_resource(__R_icons_logo); }
__T_icons_page *icons_page;
void use_icons_page() { icons_page = (__T_icons_page*) __use_resource(__R_icons_page); }
__T_machine_names_long *machine_names_long;
void use_machine_names_long() { machine_names_long = (__T_machine_names_long*) __use_resource(__R_machine_names_long); }
__T_machine_names_short *machine_names_short;
void use_machine_names_short() { machine_names_short = (__T_machine_names_short*) __use_resource(__R_machine_names_short); }
__T_machine_param_names *machine_param_names;
void use_machine_param_names() { machine_param_names = (__T_machine_param_names*) __use_resource(__R_machine_param_names); }
__T_menu_layouts *menu_layouts;
void use_menu_layouts() { menu_layouts = (__T_menu_layouts*) __use_resource(__R_menu_layouts); }
__T_menu_options *menu_options;
void use_menu_options() { menu_options = (__T_menu_options*) __use_resource(__R_menu_options); }
__T_page_entries *page_entries;
void use_page_entries() { page_entries = (__T_page_entries*) __use_resource(__R_page_entries); }
