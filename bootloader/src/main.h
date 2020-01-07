#ifndef MAIN_H
#define MAIN_H

#include <usbd_core.h>

typedef enum LedMode {
    LED_ENABLED,
    LED_BLINKING,
    LED_DISABLED,
} LedMode;

typedef struct LedState {
    uint32_t last_toggle_tick;
    LedMode mode;
} LedState;

extern USBD_HandleTypeDef USB_DEVICE;

void on_led_tick(void);

void set_led_mode(LedMode mode);

void on_error(void);

#endif
