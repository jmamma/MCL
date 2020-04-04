#!/bin/bash
# stop on first error
set -e
if [ ! -d "../../../../tools/avr" ]; then
  AVR_DUDE="sudo avrdude"
  AVR_DUDE_CONF=/etc/avrdude.conf
  DEV=/dev/$(ls /dev | grep ttyUSB | head -n 1)
else
  AVR_DIR=$(cd "../../../../tools/avr"; pwd)
  AVR_DUDE=${AVR_DIR}/bin/avrdude
  AVR_DUDE_CONF=${AVR_DIR}/etc/avrdude.conf
  DEV=/dev/$(ls /dev | grep usb | head -n 1)
fi

make -j8
${AVR_DUDE} -C${AVR_DUDE_CONF} -v -V -patmega2560 -cwiring -P${DEV} -b115200 -D -Uflash:w:./main.hex
