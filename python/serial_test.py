#!/usr/bin/env python3

import serial
import datetime
import sys
import time

# Serial port settings
SERIAL_PORT = "/dev/tty.usbmodem301"  # Replace with your serial device
BAUD_RATE = 115200

def read_serial():
    ser = None
    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
        print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud")
        
        while True:
            try:
                # Read line from serial port
                line = ser.readline().decode("utf-8").strip()
                
                # Get current timestamp with milliseconds
                timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                
                # Print timestamped data
                print(f"{timestamp}: {line}")
                
            except UnicodeDecodeError as e:
                print(f"Error decoding data: {e}")
            except serial.SerialException as e:
                print(f"Serial communication error: {e}")
                break
                
    except serial.SerialException as e:
        print(f"Error opening serial port {SERIAL_PORT}: {e}")
    except KeyboardInterrupt:
        print("\nStopping serial reader...")
    finally:
        # Clean up serial connection
        if ser is not None and ser.is_open:
            ser.close()
            print("Serial port closed")

if __name__ == "__main__":
    read_serial()
