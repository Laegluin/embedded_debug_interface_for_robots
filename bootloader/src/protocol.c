#include "protocol.h"
#include "main.h"
#include "start_user_app.h"
#include "usb_interface.h"
#include <stdbool.h>
#include <string.h>

#define QSPI_START_ADDR 0x90000000

#define START_BYTE 0xff

#define COMMAND_FLASH 0x00
#define COMMAND_RUN 0x01

static bool is_start(const Bootloader* self, uint8_t byte);
static bool is_stuffing(const Bootloader* self, uint8_t byte);
static size_t remaining_bytes(const Bootloader* self);
static void push_last_byte(Bootloader* self, uint8_t byte);
static void flash_block(uintptr_t start_addr, uint8_t* buf, size_t buf_len);
static void exec_run(void);

void bootloader_init(Bootloader* self) {
    self->state = WAITING;
    self->last_bytes[0] = START_BYTE;
    self->last_bytes[1] = START_BYTE;
    self->buf_len = 0;
}

#pragma GCC push_options
#pragma GCC optimize("O0")

void bootloader_process(Bootloader* self, const uint8_t* buf, size_t buf_len) {
    for (size_t i = 0; i < buf_len; i++) {
        uint8_t byte = buf[i];

        switch (self->state) {
            case WAITING: {
                if (is_start(self, byte)) {
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
                        usb_serial_print("error: unknown command\n");
                        self->state = WAITING;
                        break;
                    }
                }

                break;
            }
            case IMAGE_LEN: {
                if (is_start(self, byte)) {
                    usb_serial_print("error: unexpected start byte\n");
                    self->state = COMMAND;
                    break;
                }

                if (is_stuffing(self, byte)) {
                    break;
                }

                self->buf[self->buf_len] = byte;
                self->buf_len++;

                if (self->buf_len >= 4) {
                    memcpy(&self->image_len, self->buf, 4);
                    self->buf_len = 0;
                    self->next_block_addr = QSPI_START_ADDR;
                    self->state = FLASHING;
                }

                break;
            }
            case FLASHING: {
                // nothing to write if the payload is empty
                if (self->image_len == 0) {
                    usb_serial_print("ok: nothing to flash because payload is empty\n");
                }

                if (is_start(self, byte)) {
                    usb_serial_print("error: unexpected start byte\n");
                    self->state = COMMAND;
                    break;
                }

                if (is_stuffing(self, byte)) {
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
                    usb_serial_print("ok: flashed image successfully\n");
                    self->state = WAITING;
                }

                break;
            }
            default: {
                self->state = WAITING;
                break;
            }
        }

        push_last_byte(self, byte);
    }
}

static bool is_start(const Bootloader* self, uint8_t byte) {
    return self->last_bytes[0] != START_BYTE && self->last_bytes[1] == START_BYTE
        && byte != START_BYTE;
}

static bool is_stuffing(const Bootloader* self, uint8_t byte) {
    return self->last_bytes[0] != START_BYTE && self->last_bytes[1] == START_BYTE
        && byte == START_BYTE;
}

static size_t remaining_bytes(const Bootloader* self) {
    size_t written_bytes = self->next_block_addr - QSPI_START_ADDR;
    size_t buffered_bytes = self->buf_len;

    return self->image_len - (written_bytes + buffered_bytes);
}

static void push_last_byte(Bootloader* self, uint8_t byte) {
    self->last_bytes[0] = self->last_bytes[1];
    self->last_bytes[1] = byte;
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
    // stop usb driver
    if (USBD_DeInit(&USB_DEVICE) != USBD_OK) {
        on_error();
    }

    // disable all interrupts (user application may not handle them)
    HAL_NVIC_DisableIRQ(OTG_FS_WKUP_IRQn);
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    HAL_NVIC_DisableIRQ(SysTick_IRQn);

    // enable mmap mode for running the user code
    if (BSP_QSPI_EnableMemoryMappedMode() != QSPI_OK) {
        on_error();
    }

    usb_serial_print("ok: starting user program\n");
    set_led_mode(LED_ENABLED);

    // configure stack and jump to user application
    start_user_app();
}

#pragma GCC pop_options
