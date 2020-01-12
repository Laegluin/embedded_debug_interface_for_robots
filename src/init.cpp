#include "main.h"
#include <stm32f7xx.h>

extern "C" void init() {
    // enable FPU (full access)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));

    // relocate isr vector to QSPI flash
    SCB->VTOR = QSPI_START_ADDR;
}
