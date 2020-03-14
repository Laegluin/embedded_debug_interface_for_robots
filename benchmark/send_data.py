#!/usr/bin/env python3

import sys
import time
import random

serial_path = sys.argv[1]
data_path = sys.argv[2]

if sys.argv[3] == "wait":
    wait_type = "wait"
    wait_time_secs = float(sys.argv[4]) / 1000.0
elif sys.argv[3] == "no-wait":
    wait_type = "no-wait"
else:
    sys.stderr.write(f"unknown wait type `{sys.argv[3]}`\n")
    exit(1)

with open(data_path, "rb") as data_file:
    data = data_file.read()
   
serial = open(serial_path, "wb", buffering=0)

if wait_type == "no-wait":
    while True:
        serial.write(data)
else:
    def spin_wait(secs):
        if secs <= 0.0:
            return

        start = time.perf_counter()
        now = start

        while now - start < secs:
            now = time.perf_counter()
    
    def find_packet_start_offsets():
        current_start = 0

        while True:
            offset = data.find(b"\xff\xff\xfd\x00", current_start)

            if offset == -1:
                break

            yield offset
            current_start += 4

    packet_start_offsets = list(find_packet_start_offsets())
    packet_end_offsets = packet_start_offsets[1:] + [len(data)]

    while True:
        for start, end in zip(packet_start_offsets, packet_end_offsets):
            serial.write(data[start:end])
            spin_wait(wait_time_secs)
