#!/usr/bin/python
# Author: Justin Mammarella
# Date: 06/4/2023
# Description: Connect to MegaCommand via MIDI and receive
#              dumps of the OLED display bufffer.

import pygame
import pygame.midi
import os
import sys

read_buf = []
read_count = 0
BLACK = (0,   0,   0)
WHITE = (255, 255, 255)

BG = WHITE
FG = BLACK

file_count = 0;

def main():

    # Initialize the game engine
    global screen

    pygame.init()
    pygame.midi.init()
    pygame.display.set_caption("MegaCommand Display")
    # Define the colors we will use in RGB format
    w = 7
    size = [(128 + 2) * w, (32 + 2) * w]
    screen = pygame.display.set_mode(size)
    done = False

    midi_device_in = -1
    midi_device_out = -1
    devices = []
    device_count = pygame.midi.get_count()
    for d in range(device_count):
        device = pygame.midi.get_device_info(d)
        device_name = device[1].decode()
        device_type = device[2]
        if device_name == "MegaCMD":
            if device_type == 1:
               midi_device_in = d
            if device_type == 0:
               midi_device_out = d
        print(device, device_name, device_type)

    if midi_device_in == -1 or midi_device_out == -1:
      print("MegaCMD not detected")
      sys.exit()

    midi_input = pygame.midi.Input(midi_device_in)
    pygame.event.clear()
#    pygame.key.set_repeat(0, 250);
    headingfont = pygame.font.SysFont("courier new", 28)
    myfont = pygame.font.SysFont("courier new", 18)
    label = myfont.render("Waiting for connection...", 1, FG)

    screen.blit(label, (10, 10))

    pygame.display.flip()
    screen.fill(BG)


    count = 0
    x = 0
    y = 0

    header = [ 0xF0, 0x7D, 0x4D, 0x43, 0x4C, 0x40 ]
    n = 0
    while not done:
        if midi_input.poll():
            c = read_byte(midi_input)
            if header[n] == c:
              n += 1
            else:
              n = 0
            if n == len(header):
              n = 0
              receive_screen_dump(midi_input, screen, w)

def receive_screen_dump(midi_input, screen, w):
    # 7bit decode, 0 command bytes, 1 data bytes.
        global BG, FG, file_count
        draw = True
        n = 0
        c = 255
        m = 0
 
        buf = [0] * 512
        while n < 512:
            c = read_byte(midi_input)
            if c == 0xF7:
              return
            msbs = c
            for a in range(7):
                if n + a > 511:
                    break
                c = read_byte(midi_input)
                if c == 0xF7:
                  return
                msb = (msbs & 1) << 7
                buf[n + a] = c | msb
                msbs = msbs >> 1
            n = n + 7

        if draw:
            for y in range(32):
                for x in range(128):
                    yd = 32 - y - 1
                    bit = (buf[x + (int(yd/8) * 128)] >> (yd % 8)) & 0x01
                    if bit > 0:
                        pygame.draw.rect(
                            screen, FG, [(x + 1) * w, (32 - y) * w, w, w])
        pygame.display.flip()
        for event in pygame.event.get():  # User did something
            if event.type == pygame.QUIT:  # If user clicked close
                sys.exit()
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_2:
                    BG = BLACK
                    FG = WHITE
                if event.key == pygame.K_1:
                    FG = BLACK
                    BG = WHITE
                if event.key == pygame.K_9:
                    pygame.image.save(screen, os.path.expanduser("~/Desktop/") + "mcl_screenshot" + str(file_count) + ".png");
                    file_count += 1
                if event.key == pygame.K_ESCAPE:
                    pygame.quit()
                    sys.exit()
        screen.fill(BG)

def read_byte(midi_input):
  global read_count, read_buf

  while not midi_input.poll():
    pass

  if read_count == 0:
    c = midi_input.read(1)
    if len(c):
      read_buf = c[0][0]
    else:
      return 0
  ret = read_buf[read_count]
  read_count += 1
  if read_count == 4:
    read_count = 0
  return ret


if __name__ == '__main__':
    main()
