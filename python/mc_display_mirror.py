#!/usr/bin/python
# Author: Justin Mammarella
# Date: 30/7/2018
# Description: Connect to MegaCommand via serial and receive
#              dumps of the OLED display bufffer.

import serial
import pygame
import os

def main():

    # Initialize the game engine

    pygame.init()

    pygame.display.set_caption("MegaCommand Display")
    # Define the colors we will use in RGB format
    w = 7
    BLACK = (0,   0,   0)
    WHITE = (255, 255, 255)
    BLUE = (0,   0, 255)
    size = [(128 + 2) * 7, (32 + 2) * 7]
    screen = pygame.display.set_mode(size)
    done = False
    file_count = 0;
    BG = WHITE
    FG = BLACK
    import serial.tools.list_ports
    ports = list(serial.tools.list_ports.comports())

    pygame.event.clear()
#    pygame.key.set_repeat(0, 250);
    headingfont = pygame.font.SysFont("courier new", 28)
    myfont = pygame.font.SysFont("courier new", 18)
    select = 0
    menu = True
    while menu:
        for event in pygame.event.get():  # User did something
            if event.type == pygame.QUIT:  # If user clicked close
                done = True  # Flag that we are done so we exit this loop
            if event.type == pygame.KEYDOWN:
                if select > 0 and event.key == pygame.K_UP:
                    select -= 1
                if select < len(ports) - 1 and event.key == pygame.K_DOWN:
                    select += 1
                if event.key == pygame.K_RETURN:
                    serial_port = ports[select][0]
                    menu = False

        count = 0

        label = myfont.render("Select Serial Port:", 1, FG)
        screen.blit(label, (10, 20))
        for p in ports:
            if count == select:
                pygame.draw.rect(screen, FG, [10, 50 + count * 20, 500, 20])
                label = myfont.render(str(count) + ": " + p[0], 1, BG)
                screen.blit(label, (10, 50 + count * 20))
            else:
                label = myfont.render(str(count) + ": " + p[0], 1, FG)
                screen.blit(label, (10, 50 + count * 20))
            count += 1
        pygame.display.flip()
        screen.fill(BG)

    label = myfont.render("Waiting for connection...", 1, FG)

    screen.blit(label, (10, 10))

    pygame.display.flip()
    screen.fill(BG)

    ser = serial.Serial(serial_port, 250000)
    ser.flush()
    buf = [0] * 512
    count = 0
    x = 0
    y = 0
    # 7bit decode, 0 command bytes, 1 data bytes.
    while not done:
        draw = True
        n = 0
        c = 255
        m = 0
        while c != 0:
            c = ord(ser.read(1))
        while n < 512:
            c = ord(ser.read(1))
            if c == 0 and n < 512:
                draw = False
                break
            c &= 0x7F
            msbs = c
            for a in range(7):
                if n + a > 511:
                    break
                c = ord(ser.read(1))
                if c == 0 and n < 512:
                    draw = False
                    break
                c &= 0x7F
                msb = (msbs & 1) << 7
                buf[n + a] = c | msb
                msbs = msbs >> 1
            n = n + 7

        if draw:
            for y in range(32):
                for x in range(128):
                    yd = 32 - y - 1
                    bit = (buf[x + ((yd/8) * 128)] >> (yd % 8)) & 0x01
                    if bit > 0:
                        pygame.draw.rect(
                            screen, FG, [(x + 1) * w, (y + 1) * w, w, w])
        pygame.display.flip()
        for event in pygame.event.get():  # User did something
            if event.type == pygame.QUIT:  # If user clicked close
                done = True  # Flag that we are done so we exit this loop
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
        screen.fill(BG)


if __name__ == '__main__':
    main()
