#include "protocol.h"
#include <stdbool.h>
#include <string.h>

#define QSPI_START_ADDR 0x90000000

#define START_BYTE 0xff

#define COMMAND_FLASH 0x00
#define COMMAND_RUN 0x01

static size_t remaining_bytes(const Bootloader* self);
static bool is_start(uint8_t last_byte, uint8_t byte);
static bool is_stuffing(uint8_t last_byte, uint8_t byte);
static void flash_block(uintptr_t start_addr, uint8_t* buf, size_t buf_len);
static void exec_run(void);

void bootloader_init(Bootloader* self) {
    self->state = WAITING;
    self->last_byte = 0;
    self->buf_len = 0;
}

void bootloader_process(Bootloader* self, const uint8_t* buf, size_t buf_len) {
    for (size_t i = 0; i < buf_len; i++) {
        uint8_t byte = buf[i];

        switch (self->state) {
            case WAITING: {
                if (is_start(self->last_byte, byte)) {
                    self->state = COMMAND;
                }

                break;
            }
            case COMMAND: {
                switch (byte) {
                    case COMMAND_FLASH: {
                        // read next field for flash command
                        self->state = IMAGE_LEN;
                        break;
                    }
                    case COMMAND_RUN: {
                        exec_run();
                        break;
                    }
                    default: {
                        // unknown command
                        self->state = WAITING;
                        break;
                    }
                }

                break;
            }
            case IMAGE_LEN: {
                if (is_start(self->last_byte, byte)) {
                    self->state = COMMAND;
                    break;
                }

                if (is_stuffing(self->last_byte, byte)) {
                    break;
                }

                self->buf[self->buf_len] = byte;
                self->buf_len++;

                if (self->buf_len >= 4) {
                    memcpy(&self->image_len, self->buf, 4);
                    self->buf_len = 0;
                    self->next_block_addr = QSPI_START_ADDR;
                }

                // nothing to write if the payload is empty
                if (self->image_len == 0) {
                    self->state = WAITING;
                } else {
                    self->state = FLASHING;
                }

                break;
            }
            case FLASHING: {
                if (is_start(self->last_byte, byte)) {
                    self->state = COMMAND;
                    break;
                }

                if (is_stuffing(self->last_byte, byte)) {
                    break;
                }

                self->buf[self->buf_len] = byte;
                self->buf_len++;

                if (self->buf_len == QSPI_BLOCK_LEN || remaining_bytes(self) == 0) {
                    flash_block(self->next_block_addr, self->buf, self->buf_len);
                    self->next_block_addr += QSPI_BLOCK_LEN;
                    self->buf_len = 0;
                }

                if (remaining_bytes(self) == 0) {
                    self->state = WAITING;
                }

                break;
            }
            default: {
                self->state = WAITING;
                break;
            }
        }

        self->last_byte = byte;
    }
}

static size_t remaining_bytes(const Bootloader* self) {
    size_t written_bytes = self->next_block_addr - QSPI_START_ADDR;
    size_t buffered_bytes = self->buf_len;

    return self->image_len - (written_bytes + buffered_bytes);
}

static bool is_start(uint8_t last_byte, uint8_t byte) {
    return byte == START_BYTE && last_byte != START_BYTE;
}

static bool is_stuffing(uint8_t last_byte, uint8_t byte) {
    return byte == START_BYTE && last_byte == START_BYTE;
}

/// Writes `buf` to the block starting at `start_addr`. The write may be smaller than
/// the block length, but this will still erase the whole block.
///
/// `start_addr` is an address in the processors memory map, the translation to the
/// address for the memory controller is done by this function.
static void flash_block(uintptr_t start_addr, uint8_t* buf, size_t buf_len) {
    start_addr = start_addr - QSPI_START_ADDR;

    if (BSP_QSPI_Erase_Block(start_addr) != QSPI_OK) {
        return;
    }

    if (BSP_QSPI_Write(buf, start_addr, buf_len) != QSPI_OK) {
        return;
    }
}

static void exec_run(void) {
    // TODO: impl
}
