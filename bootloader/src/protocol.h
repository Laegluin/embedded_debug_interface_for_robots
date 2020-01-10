#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stm32f7508_discovery_qspi.h>

#define QSPI_BLOCK_LEN N25Q128A_SUBSECTOR_SIZE

typedef enum BootloaderState {
    WAITING,
    COMMAND,
    IMAGE_LEN,
    FLASHING,
} BootloaderState;

typedef struct Bootloader {
    BootloaderState state;
    uint8_t last_bytes[2];
    uint8_t buf[QSPI_BLOCK_LEN];
    size_t buf_len;
    uint32_t image_len;
    uint32_t remaining_image_bytes;
    uintptr_t next_block_addr;
} Bootloader;

void bootloader_init(Bootloader* self);

void bootloader_process(Bootloader* self, const uint8_t* buf, size_t buf_len);

void exec_start_command(void);

#endif
