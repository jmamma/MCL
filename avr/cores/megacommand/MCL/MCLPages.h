/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLPAGES_H__
#define MCLPAGES_H__

#define ENCODER_RES_SYS 2

extern MCLEncoder options_param1(0, 5, ENCODER_RES_SYS);
extern MCLEncoder options_param2(0, 3, ENCODER_RES_SYS);
extern MCLSystemPage system_page(&options_param1, &options_param2);

#endif /* MCLPAGES_H__ */
