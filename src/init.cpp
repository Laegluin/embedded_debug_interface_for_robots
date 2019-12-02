#include "main.h"
#include <stm32f7xx.h>

void init() {
    // enable FPU (full access)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));

    // enable HSI oscillator
    RCC->CR |= (uint32_t) 0x00000001;
    // reset config register
    RCC->CFGR = 0x00000000;
    // disable PLL, CSS and HSE oscillator
    RCC->CR &= (uint32_t) 0xfef6ffff;
    // reset PLL config register
    RCC->PLLCFGR = 0x24003010;
    // disable HSE clock bypass
    RCC->CR &= (uint32_t) 0xfffbffff;
    // disable all clock interrupts
    RCC->CIR = 0x00000000;

    // relocate isr vector to QSPI flash
    SCB->VTOR = QSPI_START_ADDR;
}
