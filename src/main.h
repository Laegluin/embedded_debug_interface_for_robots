#ifndef MAIN_H
#define MAIN_H

#include <stm32f7xx.h>

#define LCD_DISPLAY_ENABLE_PORT GPIOI
const uint16_t LCD_DISPLAY_ENABLE_PIN = GPIO_PIN_12;
#define LCD_BACKLIGHT_ENABLE_PORT GPIOK
const uint16_t LCD_BACKLIGHT_ENABLE_PIN = GPIO_PIN_3;

const uint32_t UART_BAUDRATE = 4000000;

const uint32_t SDRAM_TIMEOUT = 0xffff;
const uintptr_t SDRAM_START_ADDR = 0xc0000000;
const uintptr_t FRAME_BUF_1_ADDR = 0xc0000000;
const uintptr_t FRAME_BUF_2_ADDR = 0xc0200000;

const uintptr_t QSPI_START_ADDR = 0x90000000;

extern DMA_HandleTypeDef DMA2_STREAM1;

void init_lcd_controller(LTDC_HandleTypeDef* handle);

void on_error();

#endif
