#!/bin/bash
AVR_DIR=$(cd "../../../../tools/avr"; pwd)
DEV=/dev/$(ls /dev | grep usb | grep cu | tail -n 1)
make -j8
if [ $? -eq 0 ]; then
  echo RAM USAGE: $(${AVR_DIR}/bin/avr-size main.elf | grep main | awk '{ print $2 + $3}')
  ${AVR_DIR}/bin/avrdude -C${AVR_DIR}/etc/avrdude.conf -v -V -patmega2560 -cwiring -P${DEV} -b115200 -D -Uflash:w:./main.hex
fi
