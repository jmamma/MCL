PDEDIR ?= .

ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CORE = megacommand
BASE_DIR=$(shell cd ../../../../; pwd)
CORE_DIR=$(shell pwd)
VARIENTS_DIR=$(shell cd ../../; pwd)/variants/mega

AR=$(shell which avr-gcc-ar)
CC=$(shell which avr-gcc)
CXX=$(shell which avr-g++)
OBJCOPY=$(shell which avr-objcopy)

# fallback if system avr-gcc is not detected
ifeq (${CC},)
AR=${BASE_DIR}/tools/avr/bin/avr-gcc-ar
CC=${BASE_DIR}/tools/avr/bin/avr-gcc
CXX=${BASE_DIR}/tools/avr/bin/avr-g++
OBJCOPY =${BASE_DIR}/tools/avr/bin/avr-objcopy
endif

UISP=uisp
AVR_ARCH = atmega2560
F_CPU = 16000000L


INOFILES=$(wildcard *.pde)

OPTIM=-Os

CFLAGS = -c -Os -w -std=gnu11 -ffunction-sections -fdata-sections -MMD -flto -lm -Wl,--gc-sections,--defsym=__stack=0x8021ff,--section-start,.data=0x802200,--defsym=__heap_end=0x80ffff -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=10803 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR

CXXFLAGS = -c -Os -w -std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -MMD -flto -lm -Wl,--gc-sections,--defsym=__stack=0x8021ff,--section-start,.data=0x802200,--defsym=__heap_end=0x80ffff -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=10803 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR
CLDFLAGS = -w -Os -flto -fuse-linker-plugin -Wl,--gc-sections,--defsym=__stack=0x8021ff,--section-start,.data=0x802200,--defsym=__heap_end=0x80ffff,--relax -mmcu=atmega2560

DIRS = Sequencer SDCard Elektron A4 Wire Wire/utility Midi MidiTools Adafruit-GFX-Library Adafruit-GFX-Library/Fonts SdFat SdFat/SdCard SdFat/FatLib SdFat/SpiDriver SdFat/SpiDriver/boards MCL SPI Adafruit_SSD1305_Library CommonTools GUI MD MNM

LDIRS = $(foreach dir,$(DIRS),$(CORE_DIR)/$(dir)) ${CORE_DIR} ${VARIENTS_DIR}

$(info    LDIRS is $(LDIRS))
INCS = $(foreach dir,$(LDIRS),-I$(dir))
OBJS = $(foreach dir,$(LDIRS),$(foreach file,$(wildcard $(dir)/*.cpp),$(subst .cpp,.o,$(file))))
OBJS += $(foreach dir,$(LDIRS),$(foreach file,$(wildcard $(dir)/*.c),$(subst .c,.o,$(file))))
DFILES += $(foreach obj,$(OBJS),$(subst .o,.d,$(obj)))
$(info    OBJS is $(OBJS))

CFLAGS += $(INCS)
CXXFLAGS += $(INCS)

all: main.hex

main.cxx: $(INOFILES) $(wildcard ${CORE_DIR}/*.cpp)
	echo ${INOFILES} $(wildcard ${CORE_DIR}/*.cpp)

main.elf: core.a
	$(CC) $(CLDFLAGS) -o $@ $^ -lm

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

mcl.o: mcl.pde
	$(CXX) $(CXXFLAGS) -c $< -o $@

core.a: $(OBJS) Makefile
	echo $(AR) rcs $@ $^
	$(AR) rcs $@ $^

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.s Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) -fverbose-asm $< -o $@

%.s: %.cpp
	$(CXX) -S $(CXXFLAGS) -fverbose-asm $< -o $@

%.os: %.o
	avr-objdump -S $< > $@

%.elfs: %.elf
	avr-objdump -S $< > $@

%.o: %.S
	$(CC) $(CFLAGS) -Wa,-adhlns=$@.lst -c $< -o $@

%.d:%.c
	set -e; $(CC) -MM $(CFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@ ; \
	[ -s $@ ] || rm -f $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.ee_srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@


libclean:
	rm -rf $(OBJS)
	rm -rf $(DFILES)

clean:
	rm -rf *.os *.o *.elf *.elfs *.lst main.cxx
