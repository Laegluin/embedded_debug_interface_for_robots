#include "main.h"
#include <stm32f7xx.h>

extern "C" void SysTick_Handler() {
    HAL_IncTick();
}

extern "C" void DMA2_Stream1_IRQHandler() {
    HAL_DMA_IRQHandler(&DMA2_STREAM1);
}
