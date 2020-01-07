#!/usr/bin/env bash

# Starts the current image in QSPI flash using the bootloader
# Example: ./flash.sh /dev/ttyS4

SERIAL="$1"

# make sure there are no messages from previous runs
timeout 0.5s cat "$SERIAL" > /dev/null

printf '\xff\x01' > "$SERIAL"

# print first message by bootloader
timeout 5s cat "$SERIAL" | head -n 1
