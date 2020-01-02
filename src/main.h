#ifndef MAIN_H
#define MAIN_H

#include <stm32f7xx.h>

const uint32_t SDRAM_TIMEOUT = 0xffff;
const uint32_t UART_BAUDRATE = 4000000;
const uintptr_t SDRAM_START_ADDR = 0xc0000000;
const uintptr_t FRAME_BUF_1_ADDR = 0xc0000000;
const uintptr_t FRAME_BUF_2_ADDR = 0xc0200000;
const uintptr_t QSPI_START_ADDR = 0x90000000;

HAL_StatusTypeDef init_lcd_controller(LTDC_HandleTypeDef* handle);

#endif
