#include "main.h"
#include <stm32f7xx.h>

extern "C" void SysTick_Handler() {
    HAL_IncTick();
}

extern "C" void DMA2_Stream1_IRQHandler() {
    HAL_DMA_IRQHandler(&DMA2_STREAM1);
}

extern "C" void I2C3_EV_IRQHandler() {
    HAL_I2C_EV_IRQHandler(&I2C_BUS3);
}

extern "C" void I2C3_ER_IRQHandler() {
    HAL_I2C_ER_IRQHandler(&I2C_BUS3);
}

extern "C" void TIM2_IRQHandler() {
    HAL_TIM_IRQHandler(&TIMER2);
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* timer) {
    if (timer == &TIMER2) {
        poll_touch_state();
    }
}
