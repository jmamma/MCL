#!/bin/bash
input=$(echo ${@} | tr ',' ' ')
for j in $input
do
        hex=$(echo $j | cut -f2 -d 'x')
        BIN=$(echo "obase=2; ibase=16; $hex" | bc )
        while [ $(echo -n $BIN | wc -c) -lt 8 ]; do
        BIN='0'$BIN
        done
        echo $BIN
done
