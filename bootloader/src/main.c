#include "main.h"
#include "usb_descriptor.h"
#include "usb_interface.h"
#include <stm32f7xx.h>
#include <stm32f7xx_hal_pwr_ex.h>
#include <stm32f7508_discovery_qspi.h>
#include <usbd_cdc.h>

USBD_HandleTypeDef USB_DEVICE;

static void init_clocks(void);
static void init_led(void);
static void init_qspi(void);
static void init_usb(void);

int main(void) {
    if (HAL_Init() != HAL_OK) {
        on_error();
    }

    init_clocks();
    HAL_NVIC_EnableIRQ(SysTick_IRQn);
    SCB_EnableICache();
    SCB_EnableDCache();

    init_led();
    init_qspi();
    init_usb();

    while (1) {
        // wait for interrupts
    }
}

static void init_clocks(void) {
    // Enable HSE Oscillator and activate PLL with HSE as source
    RCC_OscInitTypeDef oscillator_config;
    oscillator_config.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator_config.HSEState = RCC_HSE_ON;
    oscillator_config.HSIState = RCC_HSI_OFF;
    oscillator_config.PLL.PLLState = RCC_PLL_ON;
    oscillator_config.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator_config.PLL.PLLM = 25;
    oscillator_config.PLL.PLLN = 432;
    oscillator_config.PLL.PLLP = RCC_PLLP_DIV2;
    oscillator_config.PLL.PLLQ = 9;
    if (HAL_RCC_OscConfig(&oscillator_config) != HAL_OK) {
        on_error();
    }

    // Activate the OverDrive to reach the 216 Mhz Frequency
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
        on_error();
    }

    // Select PLLSAI output as USB clock source
    RCC_PeriphCLKInitTypeDef peripheral_clock_config;
    peripheral_clock_config.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
    peripheral_clock_config.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
    peripheral_clock_config.PLLSAI.PLLSAIN = 384;
    peripheral_clock_config.PLLSAI.PLLSAIQ = 7;
    peripheral_clock_config.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
    if (HAL_RCCEx_PeriphCLKConfig(&peripheral_clock_config) != HAL_OK) {
        on_error();
    }

    // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers
    RCC_ClkInitTypeDef rcc_config;
    rcc_config.ClockType =
        (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    rcc_config.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    rcc_config.AHBCLKDivider = RCC_SYSCLK_DIV1;
    rcc_config.APB1CLKDivider = RCC_HCLK_DIV4;
    rcc_config.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&rcc_config, FLASH_LATENCY_7) != HAL_OK) {
        on_error();
    }
}

static void init_led(void) {
    __HAL_RCC_GPIOI_CLK_ENABLE();

    GPIO_InitTypeDef gpio_config;
    gpio_config.Pin = GPIO_PIN_1;
    gpio_config.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_config.Pull = GPIO_PULLUP;
    gpio_config.Speed = GPIO_SPEED_HIGH;

    HAL_GPIO_Init(GPIOI, &gpio_config);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_1, GPIO_PIN_SET);
}

static void init_qspi(void) {
    if (BSP_QSPI_Init() != QSPI_OK) {
        on_error();
    }
}

static void init_usb(void) {
    if (USBD_Init(&USB_DEVICE, &USB_DESCRIPTOR, 0) != USBD_OK) {
        on_error();
    }

    if (USBD_RegisterClass(&USB_DEVICE, USBD_CDC_CLASS) != USBD_OK) {
        on_error();
    }

    if (USBD_CDC_RegisterInterface(&USB_DEVICE, &USB_CDC_INTERFACE) != USBD_OK) {
        on_error();
    }

    if (USBD_Start(&USB_DEVICE) != USBD_OK) {
        on_error();
    }
}

__attribute__((noinline)) void on_error(void) {
    while (1) {
        // spin
    }
}