#!/usr/bin/env python

import sys

def dec(h):
    return int(h, 16)

def calculate_checksum(values):
    checksum_sum = sum(values)
    lsb = checksum_sum & 0xFF
    twos_complement_lsb = (~lsb + 1) & 0xFF
    record_checksum_str = format(twos_complement_lsb, '02X')
    return record_checksum_str

def insert_record(record_type, address, length, values):
    print("insert record")
    values.reverse() #endianess for atmega
    output = length + address + record_type + values
    checksum = calculate_checksum(output)
    output = [format(byte, '02X') for byte in output] #convert to hex str format
    output.append(checksum)
    output.append("\n")
    output = [":"] + output
    print(''.join(output))
    lines.insert(-1, ''.join(output))

if len(sys.argv) != 2:
    print("Usage: python script.py <filename>")
    sys.exit(1)


filename = sys.argv[1]
lines = []
count = 0

b = []
with open(filename, "r") as file:
    for line in file:
        lines.append(line)
        line = line.strip()  # Remove leading/trailing whitespace
        length = len(line)
        if line.startswith(":"):
            n = dec(line[1:3])  # number of bytes in payload
            record = line[7:7 + 2]
            data = line[9:9 + (n * 2)]
            data_checksum = line[-2:]
            if record != "00": # only include data records
              continue
          # Print every two characters in the data payload
            for i in range(0, n * 2, 2):
              count += 1
              byte = data[i:i+2]
              b.append(dec(byte))

n = 0

byte = 0
checksum=0
print(count)
while n < count:
  last_byte = byte
  byte = b[n]
  if n % 2 == 0:
    checksum ^= (byte << 8) | last_byte
  n += 1

checksum_str = format(checksum, '04X')
length_str = format(count, '08X')

print("Checksum: ", checksum_str)

print(format(checksum, 'X'))

print("Length: ", count)

# Append the checksum to the main.hex file

lines.insert(-1,":020000023000CC\n")  # Extended Segment Address Record

#256 * 1024 - 16 * 1024 - 6

# Length, 32bit @ 0xBFFA
data_bytes = [ dec(length_str[i:i+2]) for i in range(0, len(length_str), 2)]

length_bytes = [ 0x04 ]
address_bytes = [ 0xBF, 0xFA ]
record_bytes = [ 0x00 ]

insert_record(record_bytes, address_bytes, length_bytes, data_bytes)

# Checksum, 16bit @ 0xBFFE

data_bytes = [ dec(checksum_str[i:i+2]) for i in range(0, len(checksum_str), 2)]

length_bytes = [ 0x02 ]
address_bytes = [ 0xBF, 0xFE ]
record_bytes = [ 0x00 ]

insert_record(record_bytes, address_bytes, length_bytes, data_bytes)

with open(filename, "w") as f:
    f.truncate()
    for line in lines:
        f.write(line)

