#ifndef MAIN_H
#define MAIN_H

#include <stm32f7xx.h>

/// The size of each task's stack, in words (4 bytes).
const size_t TASK_STACK_SIZE = 1024;

const uint32_t DISPLAY_WIDTH = 480;
const uint32_t DISPLAY_HEIGHT = 272;

const uint32_t UART_BAUDRATE = 2000000;

const uintptr_t SDRAM_START_ADDR = 0xc0000000;
const uintptr_t FRAME_BUF_1_ADDR = 0xc0000000;
const uintptr_t FRAME_BUF_2_ADDR = 0xc005fa00;
const uintptr_t QSPI_START_ADDR = 0x90000000;

extern LTDC_HandleTypeDef LCD_CONTROLLER;
extern I2C_HandleTypeDef I2C_BUS3;
extern DMA_HandleTypeDef DMA2_STREAM1;
extern TIM_HandleTypeDef TIMER2;
extern TIM_HandleTypeDef TIMER3;

void poll_touch_state();

void on_error();

#endif
