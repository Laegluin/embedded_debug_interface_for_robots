#include <stm32f7xx.h>

extern "C" void SysTick_Handler() {
    HAL_IncTick();
}
