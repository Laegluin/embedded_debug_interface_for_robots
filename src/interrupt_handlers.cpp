#include "main.h"
#include <FreeRTOS.h>
#include <stm32f7xx.h>
#include <task.h>

extern "C" void xPortSysTickHandler();

extern "C" void SysTick_Handler() {
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        xPortSysTickHandler();
    }
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

extern "C" void TIM3_IRQHandler() {
    HAL_TIM_IRQHandler(&TIMER3);
}

extern "C" void TIM4_IRQHandler() {
    if (__HAL_TIM_GET_FLAG(&TIMER4, TIM_FLAG_UPDATE) != RESET
        && __HAL_TIM_GET_IT_SOURCE(&TIMER4, TIM_IT_UPDATE) != RESET) {
        __HAL_TIM_CLEAR_IT(&TIMER4, TIM_IT_UPDATE);
        HIGH_RES_TICK++;
    }
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* timer) {
    if (timer == &TIMER2) {
        poll_touch_state();
    } else if (timer == &TIMER3) {
        HAL_IncTick();
    }
}
