#!/usr/bin/env python 
import serial

s = serial.Serial("/dev/ttyACM0", baudrate=115200, timeout=1)

while True:
    try:
        a = s.readline()
        print(a)
    except KeyboardInterrupt:
        break