#!/bin/bash
AVR_DIR=$(cd "../../../../tools/avr"; pwd)
DEV=/dev/$(ls /dev | grep usb | grep cu | tail -n 1)
# Parse command line arguments

run_checksum=false
no_upload=false
while getopts "c" opt; do
  case $opt in
    c)
      run_checksum=true
      ;;
    n)
      no_upload=true
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
  esac
done


rm main.hex
if $run_checksum; then
  make -j8 CHECKSUM=1
else
  make -j8
fi




if ! $no_upload; then

  size=$(${AVR_DIR}/bin/avr-size main.elf | tail -n 1 | awk '{ print $1 + $2}')
  limit=$((256 * 1024 - 16 * 1024 - 6))
  echo ROM_SIZE : $size
  echo ROM_LIMIT: $limit
  echo ROM_FREE : $(($limit - $size))
  ../../../../tools/avr/bin/avr-nm main.elf -Crtd --size-sort | grep -iv ' b ' | head -n 256

  ${AVR_DIR}/bin/avr-size main.elf
  ram_used=$(${AVR_DIR}/bin/avr-size main.elf | grep main | awk '{ print $2 + $3}')
  ram_free=$((1024 * 64 - $ram_used - 8 * 1024))
  echo RAM_USED: $ram_used
  echo RAM_FREE: $ram_free
  ${AVR_DIR}/bin/avr-nm main.elf -Crtd --size-sort | grep -i ' [dbv] ' | sort
  hex_file=main.hex
  if $run_checksum; then
    cp main.hex main_checksum.hex
    ./checksum.py main_checksum.hex;
    hex_file=main_checksum.hex
  fi
  #avrdude -c atmelice_isp -p m2560 -D -Uflash:w:./main.hex  -B 1
  ${AVR_DIR}/bin/avrdude -C${AVR_DIR}/etc/avrdude.conf -V -patmega2560 -c arduino -P${DEV} -b115200 -D -Uflash:w:./$hex_file
  #${AVR_DIR}/bin/avrdude -C${AVR_DIR}/etc/avrdude.conf -V -patmega2560 -c wiring -P${DEV} -b115200 -D -Uflash:w:./main.hex
fi

