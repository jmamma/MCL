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
AVRSIZE=$(shell which avr-size)
AVRDUDE=$(shell which avrdude)
AVRDUDE_CFG=/etc/avrdude.conf

# fallback if system avr-gcc is not detected
ifeq (${CC},)
AR=${BASE_DIR}/tools/avr/bin/avr-gcc-ar
CC=${BASE_DIR}/tools/avr/bin/avr-gcc
CXX=${BASE_DIR}/tools/avr/bin/avr-g++
OBJCOPY =${BASE_DIR}/tools/avr/bin/avr-objcopy
AVRSIZE=${BASE_DIR}/tools/avr/bin/avr-size
AVRDUDE=${BASE_DIR}/tools/avr/bin/avrdude
AVRDUDE_CFG=${BASE_DIR}/tools/avr/etc/avrdude.conf
endif

AVR_DEVICE=$(shell ls /dev/tty* | grep -i usb | head -n 1)
UISP=uisp
AVR_ARCH = atmega2560
F_CPU = 16000000L


INOFILES=$(wildcard *.pde)

OPTIM_FLAGS   = -Os
WARN_FLAGS    = -Wall -Wextra
COMPILE_FLAGS = -c $(OPTIM_FLAGS) $(WARN_FLAGS) -MMD
FEATURE_FLAGS = -ffunction-sections -fdata-sections -flto=jobserver -fpermissive
LINK_FLAGS    = -lm -Wl,--gc-sections,--defsym=__stack=0x8021ff,--section-start,.data=0x802200,--defsym=__heap_end=0x80ffff,--relax
ARCH_FLAGS    = -mmcu=atmega2560 -DF_CPU=16000000L -DARDUINO=10803 -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR

CFLAGS        = -std=gnu11   $(COMPILE_FLAGS) $(FEATURE_FLAGS) $(LINK_FLAGS) $(ARCH_FLAGS)
CXXFLAGS      = -std=gnu++11 $(COMPILE_FLAGS) $(FEATURE_FLAGS) $(LINK_FLAGS) $(ARCH_FLAGS)
CXXFLAGS     += -fno-exceptions -fno-threadsafe-statics -fno-rtti

LDFLAGS = $(OPTIM_FLAGS) $(WARN_FLAGS) -flto -fuse-linker-plugin -mrelax $(LINK_FLAGS) $(ARCH_FLAGS)

DIRS = Sequencer SDCard Elektron A4 Wire Wire/utility Midi MidiTools Adafruit-GFX-Library Adafruit-GFX-Library/Fonts SdFat SdFat/SdCard SdFat/FatLib SdFat/SpiDriver SdFat/SpiDriver/boards MCL SPI Adafruit_SSD1305_Library CommonTools GUI MD MNM resources

LDIRS = $(foreach dir,$(DIRS),$(CORE_DIR)/$(dir)) ${CORE_DIR} ${VARIENTS_DIR}

$(info    LDIRS is $(LDIRS))
INCS = $(foreach dir,$(LDIRS),-I$(dir))
OBJS = $(foreach dir,$(LDIRS),$(foreach file,$(wildcard $(dir)/*.cpp),$(subst .cpp,.o,$(file))))
OBJS += $(foreach dir,$(LDIRS),$(foreach file,$(wildcard $(dir)/*.c),$(subst .c,.o,$(file))))
$(info    OBJS is $(OBJS))

CFLAGS += $(INCS)
CXXFLAGS += $(INCS)

CFLAGS += -DMCL_NOSKETCH
CXXFLAGS += -DMCL_NOSKETCH

all: main.hex

main.elf: core.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

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
	$(OBJCOPY) -j .text -j .data -O ihex $< $@; \
	$(AVRSIZE) $<

%.ee_srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@


libclean:
	rm -rf $(OBJS)

clean:
	find . -iname '*.d' -exec rm {} \; ; \
	find . -iname '*.o' -exec rm {} \; ; \
	find . -iname '*.os' -exec rm {} \; ; \
	find . -iname '*.elf' -exec rm {} \; ; \
	find . -iname '*.lst' -exec rm {} \; ;


upload: main.hex
	echo "DEVICE is $(AVR_DEVICE)"; \
  echo "RAM USAGE: `$(AVRSIZE) main.elf | grep main | awk '{ print $$2 + $$3}'`"; \
  ${AVRDUDE} -C${AVRDUDE_CFG} -v -V -patmega2560 -cwiring -P${AVR_DEVICE} -b115200 -D -Uflash:w:./main.hex

debug: CXXFLAGS += -DDEBUGMODE=1
debug: CFLAGS += -DDEBUGMODE=1
debug: main.hex

debug_upload: debug upload
