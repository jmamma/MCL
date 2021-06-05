#!/bin/bash
AVR_DIR=$(cd "../../../../tools/avr"; pwd)
DEV=/dev/$(ls /dev | grep usb | grep cu | tail -n 1)
make -j8

if [ $? -eq 0 ]; then
  ${AVR_DIR}/bin/avr-nm main.elf -Crtd --size-sort | grep -i ' [dbv] ' | sort | tail -n 40
  ram_used=$(${AVR_DIR}/bin/avr-size main.elf | grep main | awk '{ print $2 + $3}')
  ram_free=$((1024 * 64 - $ram_used - 8 * 1024))
  echo RAM_USED: $ram_used
  echo RAM_FREE: $ram_free
  ${AVR_DIR}/bin/avrdude -C${AVR_DIR}/etc/avrdude.conf -v -V -patmega2560 -cwiring -P${DEV} -b115200 -D -Uflash:w:./main.hex
fi
