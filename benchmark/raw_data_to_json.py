#!/usr/bin/env python3

import sys
import json

with open(sys.argv[1], "rb") as per_buffer_data_file:
    per_buffer_data = per_buffer_data_file.read()

with open(sys.argv[2], "rb") as between_buffers_data_file:
    between_buffers_data = between_buffers_data_file.read()

json_file = open(sys.argv[3], "x")


def window_iter(iterable, window_size):
    iterator = iter(iterable)

    while True:
        window = []

        for _ in range(0, window_size):
            window.append(next(iterator))

        yield window


data = {"per_buffer": [], "between_buffers": []}

for start_and_end in window_iter(per_buffer_data, 8):
    start = int.from_bytes(start_and_end[0:4], byteorder="little")
    end = int.from_bytes(start_and_end[4:8], byteorder="little")

    if start == 0 or end == 0:
        break

    data["per_buffer"].append([start, end])

for raw_tick in window_iter(between_buffers_data, 4):
    tick = int.from_bytes(raw_tick, byteorder="little")

    if tick == 0:
        break

    data["between_buffers"].append(tick)

json.dump(data, json_file, indent=4)
