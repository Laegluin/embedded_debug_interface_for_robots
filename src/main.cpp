#include "main.h"
#include "LCDConf.h"
#include "app.h"
#include <GUI.h>
#include <stm32f7xx_hal_rcc_ex.h>
#include <vector>

void init_mpu();
HAL_StatusTypeDef init_lcd_controller(LTDC_HandleTypeDef*);
void reset_lcd_controller(LTDC_HandleTypeDef*);
HAL_StatusTypeDef init_sdram();
HAL_StatusTypeDef init_uarts(std::vector<UART_HandleTypeDef*>&);

int main() {
    {
        SCB_EnableICache();
        SCB_EnableDCache();
        init_mpu();

        if (HAL_Init() != HAL_OK) {
            goto cleanup;
        }

        HAL_NVIC_EnableIRQ(SysTick_IRQn);

        if (init_sdram() != HAL_OK) {
            goto cleanup;
        }

        std::vector<UART_HandleTypeDef*> uarts;
        if (init_uarts(uarts) != HAL_OK) {
            goto cleanup;
        }

        if (GUI_Init() != 0) {
            goto cleanup;
        }

        try {
            run(uarts);
        } catch (...) {
            goto cleanup;
        }
    }

cleanup:
    GUI_Exit();
    reset_lcd_controller(&LCD_CONTROLLER);
    HAL_DeInit();
    return 0;
}

void init_mpu() {
    HAL_MPU_Disable();

    // complete sdram address space
    MPU_Region_InitTypeDef generic_sdram;
    generic_sdram.Enable = true;
    generic_sdram.Number = MPU_REGION_NUMBER0;
    generic_sdram.BaseAddress = SDRAM_START_ADDR;
    generic_sdram.Size = MPU_REGION_SIZE_8MB;
    generic_sdram.SubRegionDisable = false;
    generic_sdram.TypeExtField = MPU_TEX_LEVEL1;  // normal
    generic_sdram.AccessPermission = MPU_REGION_FULL_ACCESS;
    generic_sdram.DisableExec = true;
    generic_sdram.IsShareable = true;
    generic_sdram.IsCacheable = true;
    generic_sdram.IsBufferable = true;
    HAL_MPU_ConfigRegion(&generic_sdram);

    // frame buffer 1 on sdram
    MPU_Region_InitTypeDef frame_buf_1;
    frame_buf_1.Enable = true;
    frame_buf_1.Number = MPU_REGION_NUMBER1;
    frame_buf_1.BaseAddress = FRAME_BUF_1_ADDR;
    frame_buf_1.Size = MPU_REGION_SIZE_512KB;
    frame_buf_1.SubRegionDisable = false;
    frame_buf_1.TypeExtField = MPU_TEX_LEVEL1;  // normal
    frame_buf_1.AccessPermission = MPU_REGION_FULL_ACCESS;
    frame_buf_1.DisableExec = true;
    frame_buf_1.IsShareable = true;
    frame_buf_1.IsCacheable = true;    // cache reads
    frame_buf_1.IsBufferable = false;  // write-through
    HAL_MPU_ConfigRegion(&frame_buf_1);

    // frame buffer 2 on sdram
    MPU_Region_InitTypeDef frame_buf_2;
    frame_buf_2.Enable = true;
    frame_buf_2.Number = MPU_REGION_NUMBER2;
    frame_buf_2.BaseAddress = FRAME_BUF_2_ADDR;
    frame_buf_2.Size = MPU_REGION_SIZE_512KB;
    frame_buf_2.SubRegionDisable = false;
    frame_buf_2.TypeExtField = MPU_TEX_LEVEL1;  // normal
    frame_buf_2.AccessPermission = MPU_REGION_FULL_ACCESS;
    frame_buf_2.DisableExec = true;
    frame_buf_2.IsShareable = true;
    frame_buf_2.IsCacheable = true;    // cache reads
    frame_buf_2.IsBufferable = false;  // write-through
    HAL_MPU_ConfigRegion(&frame_buf_2);

    // complete qspi address space
    MPU_Region_InitTypeDef generic_qspi;
    generic_qspi.Enable = true;
    generic_qspi.Number = MPU_REGION_NUMBER3;
    generic_qspi.BaseAddress = QSPI_START_ADDR;
    generic_qspi.Size = MPU_REGION_SIZE_256MB;
    generic_qspi.SubRegionDisable = false;
    generic_qspi.TypeExtField = MPU_TEX_LEVEL0;  // strongly-ordered
    generic_qspi.AccessPermission = MPU_REGION_NO_ACCESS;
    generic_qspi.DisableExec = true;
    generic_qspi.IsShareable = false;
    generic_qspi.IsCacheable = false;
    generic_qspi.IsBufferable = false;
    HAL_MPU_ConfigRegion(&generic_qspi);

    // installed qspi flash
    MPU_Region_InitTypeDef installed_qspi;
    installed_qspi.Enable = true;
    installed_qspi.Number = MPU_REGION_NUMBER4;
    installed_qspi.BaseAddress = QSPI_START_ADDR;
    installed_qspi.Size = MPU_REGION_SIZE_16MB;
    installed_qspi.SubRegionDisable = false;
    installed_qspi.TypeExtField = MPU_TEX_LEVEL1;  // normal
    installed_qspi.AccessPermission = MPU_REGION_PRIV_RO;
    installed_qspi.DisableExec = false;
    installed_qspi.IsShareable = false;
    installed_qspi.IsCacheable = true;
    installed_qspi.IsBufferable = false;
    HAL_MPU_ConfigRegion(&installed_qspi);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/// Initializes the LCD controller. On success, `handle` is initialized.
///
/// _Note:_ Is called from `LCDConf.c`.
HAL_StatusTypeDef init_lcd_controller(LTDC_HandleTypeDef* handle) {
    // GPIO and clock init
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    GPIO_InitTypeDef gpio_config;
    gpio_config.Pin = GPIO_PIN_4;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &gpio_config);

    gpio_config.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_11 | GPIO_PIN_8
        | GPIO_PIN_10 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_6 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_3
        | GPIO_PIN_2 | GPIO_PIN_0 | GPIO_PIN_1;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOJ, &gpio_config);

    gpio_config.Pin =
        GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_0;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOK, &gpio_config);

    gpio_config.Pin = GPIO_PIN_12;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_config.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOG, &gpio_config);

    gpio_config.Pin = GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_15 | GPIO_PIN_14;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &gpio_config);

    // lcd controller init
    handle->Instance = LTDC;

    // make sure the controller is reset
    auto result = HAL_LTDC_DeInit(handle);
    if (result != HAL_OK) {
        return result;
    }

    handle->Init.HSPolarity = LTDC_HSPOLARITY_AL;
    handle->Init.VSPolarity = LTDC_VSPOLARITY_AL;
    handle->Init.DEPolarity = LTDC_DEPOLARITY_AL;
    handle->Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    handle->Init.HorizontalSync = 40;
    handle->Init.VerticalSync = 9;
    handle->Init.AccumulatedHBP = 53;
    handle->Init.AccumulatedVBP = 11;
    handle->Init.AccumulatedActiveW = 533;
    handle->Init.AccumulatedActiveH = 283;
    handle->Init.TotalWidth = 565;
    handle->Init.TotalHeigh = 285;
    handle->Init.Backcolor.Blue = 0;
    handle->Init.Backcolor.Green = 0;
    handle->Init.Backcolor.Red = 0;

    result = HAL_LTDC_Init(handle);
    if (result != HAL_OK) {
        return result;
    }

    LTDC_LayerCfgTypeDef layer_config;
    layer_config.WindowX0 = 0;
    layer_config.WindowY0 = 0;
    layer_config.WindowX1 = 480;
    layer_config.WindowY1 = 272;
    layer_config.PixelFormat = LTDC_PIXEL_FORMAT_RGB888;
    layer_config.Alpha = 255;
    layer_config.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer_config.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer_config.FBStartAdress = FRAME_BUF_1_ADDR;
    layer_config.ImageWidth = 480;
    layer_config.ImageHeight = 272;
    layer_config.Alpha0 = 0;
    layer_config.Backcolor.Blue = 0;
    layer_config.Backcolor.Green = 0;
    layer_config.Backcolor.Red = 0;

    return HAL_LTDC_ConfigLayer(handle, &layer_config, 0);
}

void reset_lcd_controller(LTDC_HandleTypeDef* handle) {
    HAL_LTDC_DeInit(handle);

    __HAL_RCC_LTDC_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_4);

    HAL_GPIO_DeInit(
        GPIOJ,
        GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | GPIO_PIN_11 | GPIO_PIN_8 | GPIO_PIN_10
            | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_6 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_3
            | GPIO_PIN_2 | GPIO_PIN_0 | GPIO_PIN_1);

    HAL_GPIO_DeInit(
        GPIOK,
        GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_0);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_15 | GPIO_PIN_14);
}

HAL_StatusTypeDef init_sdram() {
    // GPIO and clock init
    __HAL_RCC_FMC_CLK_ENABLE();

    GPIO_InitTypeDef gpio_config;
    gpio_config.Pin = GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11 | GPIO_PIN_14
        | GPIO_PIN_7 | GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_15 | GPIO_PIN_13;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOE, &gpio_config);

    gpio_config.Pin = GPIO_PIN_15 | GPIO_PIN_8 | GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_5 | GPIO_PIN_4;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOG, &gpio_config);

    gpio_config.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_15 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_9 | GPIO_PIN_8;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOD, &gpio_config);

    gpio_config.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
        | GPIO_PIN_12 | GPIO_PIN_15 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_11;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOF, &gpio_config);

    gpio_config.Pin = GPIO_PIN_5 | GPIO_PIN_3;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOH, &gpio_config);

    gpio_config.Pin = GPIO_PIN_3;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOC, &gpio_config);

    // FMC configuration
    SDRAM_HandleTypeDef handle;
    handle.Instance = FMC_SDRAM_DEVICE;
    handle.Init.SDBank = FMC_SDRAM_BANK1;
    handle.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
    handle.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
    handle.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
    handle.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    handle.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
    handle.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    handle.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
    handle.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
    handle.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;

    FMC_SDRAM_TimingTypeDef timing;
    timing.LoadToActiveDelay = 2;
    timing.ExitSelfRefreshDelay = 7;
    timing.SelfRefreshTime = 4;
    timing.RowCycleDelay = 7;
    timing.WriteRecoveryTime = 3;
    timing.RPDelay = 2;
    timing.RCDDelay = 2;

    auto result = HAL_SDRAM_Init(&handle, &timing);
    if (result != HAL_OK) {
        return result;
    }

    // SDRAM initialization
    FMC_SDRAM_CommandTypeDef command;
    command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        return result;
    }

    HAL_Delay(1);

    command.CommandMode = FMC_SDRAM_CMD_PALL;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        return result;
    }

    command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 8;
    command.ModeRegisterDefinition = 0;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        return result;
    }

    const uint32_t MODEREG_BURST_LENGTH_1 = 0x00000000;
    const uint32_t MODEREG_BURST_TYPE_SEQUENTIAL = 0x00000000;
    const uint32_t MODEREG_CAS_LATENCY_3 = 0x00000030;
    const uint32_t MODEREG_OPERATING_MODE_STANDARD = 0x00000000;
    const uint32_t MODEREG_WRITEBURST_MODE_SINGLE = 0x00000200;

    command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = MODEREG_BURST_LENGTH_1 | MODEREG_BURST_TYPE_SEQUENTIAL
        | MODEREG_CAS_LATENCY_3 | MODEREG_OPERATING_MODE_STANDARD | MODEREG_WRITEBURST_MODE_SINGLE;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        return result;
    }

    return HAL_SDRAM_ProgramRefreshRate(&handle, 1674);
}

HAL_StatusTypeDef init_uarts(std::vector<UART_HandleTypeDef*>& uarts) {
    __HAL_RCC_USART6_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio_config;
    gpio_config.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &gpio_config);

    // the handle is leaked and never freed, which is fine since we need
    // for the whole program run time anyway
    UART_HandleTypeDef* handle = new UART_HandleTypeDef;
    handle->Instance = USART6;
    handle->Init.BaudRate = UART_BAUDRATE;
    handle->Init.WordLength = UART_WORDLENGTH_8B;
    handle->Init.StopBits = UART_STOPBITS_1;
    handle->Init.Parity = UART_PARITY_NONE;
    handle->Init.Mode = UART_MODE_RX;
    handle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    handle->Init.OverSampling = UART_OVERSAMPLING_8;
    handle->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;

    auto result = HAL_UART_Init(handle);
    if (result != HAL_OK) {
        return result;
    }

    uarts.push_back(handle);
    return HAL_OK;
}

// TODO: separate file
void SysTick_Handler() {
    HAL_IncTick();
}

__attribute__((noinline)) void on_error() {
    GUI_Exit();
    reset_lcd_controller(&LCD_CONTROLLER);

    while (1) {
        // spin
    }
}
