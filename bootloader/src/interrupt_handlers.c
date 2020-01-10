#include "main.h"
#include "protocol.h"
#include "usbd_conf.h"
#include <stm32f7xx.h>

void SysTick_Handler(void) {
    HAL_IncTick();
    on_led_tick();
}

void OTG_FS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&USB_PERIPHERAL_CONTROLLER);
}

// is only enabled for user button
void EXTI15_10_IRQHandler(void) {
    exec_start_command();
}
