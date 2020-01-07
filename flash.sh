#!/usr/bin/env bash

# Flashes a binary image using the bootloader
# Example: ./flash.sh /dev/ttyS4 target/firmware.bin

SERIAL="$1"
IMAGE_PATH="$2"

if [ ! -f "$IMAGE_PATH" ]; then
    echo "$IMAGE_PATH does not exist"
    exit 1
fi

function hex_be_to_le {
    echo -n ${1:6:2}${1:4:2}${1:2:2}${1:0:2}
}

LEN_STR=$(stat --printf %s "$IMAGE_PATH")
LEN_HEX_STR=$(printf "%.8x" "$LEN_STR")
LEN_HEX_STR_LE=$(hex_be_to_le "$LEN_HEX_STR")

# make sure there are no messages from previous runs
timeout 0.5s cat "$SERIAL" > /dev/null

printf '\xff\x00' > "$SERIAL"
echo -n "0x$LEN_HEX_STR_LE" | xxd -r | sed 's/\xff/\xff\xff/g' >> "$SERIAL"
cat "$IMAGE_PATH" | sed 's/\xff/\xff\xff/g' >> "$SERIAL"

# print first message by bootloader
timeout 5s cat "$SERIAL" | head -n 1
