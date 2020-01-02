#include "usbd_conf.h"
#include <stm32f7xx.h>

void SysTick_Handler(void) {
    HAL_IncTick();
}

void OTG_FS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&USB_PERIPHERAL_CONTROLLER);
}
