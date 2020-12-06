# Understand the bootloader limitations 

The bootloader for arduino 2560 can be located here: `avr/bootloaders/stk500v2/`
To compile, it should be set up like this (extracted from the makefile):
```
-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -fno-jump-tables
-Wall -Wstrict-prototypes
-DBOOTLOADER_ADDRESS=3E000
-D_MEGA_BOARD_
-DMCU=atmega2560
-DF_CPU=16000000
-D__AVR_ARCH__=__AVR_ATmega2560__
-D__AVR_ATmega2560__
```

The bootloader assumes the boot section size:
```c
//#define BOOTSIZE 1024
#if FLASHEND > 0x0F000
	#define BOOTSIZE 8192
#else
	#define BOOTSIZE 2048
#endif

#define APP_END  (FLASHEND -(2*BOOTSIZE) + 1)
```

Therefore, the application section is `[0, 3C000) / [0, 245760)` -- that's the limit in the bootloader code.

This contradicts `boards.txt` in the core: `mega.menu.cpu.atmega2560.upload.maximum_size=253952`, which assumes that the bootloader takes 4K words, 8KB.

Interestingly, the BOOTSZ fuse bits for arduino specifies:

![image](https://user-images.githubusercontent.com/20684720/101241642-e97b5a80-3732-11eb-8625-96de168413b3.png)
with 8KB as the maximum, so 16KB is an impossible value.

However, there are some early ATMega2560 batches with the defunct/workaround:
![image](https://user-images.githubusercontent.com/20684720/101241656-1a5b8f80-3733-11eb-94c2-a67898fbc7e1.png)

So that may be why the stk500v2 bootloader decided to do that.

If we switch to optiboot, we can reclaim 15872 bytes of PROGMEM.
