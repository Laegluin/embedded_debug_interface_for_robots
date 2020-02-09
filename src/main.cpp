#include "main.h"
#include "app.h"

#include <FreeRTOS.h>
#include <task.h>

#include <GUI.h>
#include <WM.h>

#include <stm32f7508_discovery_ts.h>
#include <stm32f7xx_hal_rcc_ex.h>

#include <vector>

LTDC_HandleTypeDef LCD_CONTROLLER;
I2C_HandleTypeDef I2C_BUS3;
DMA_HandleTypeDef DMA2_STREAM1;
TIM_HandleTypeDef TIMER2;
TIM_HandleTypeDef TIMER3;

void init_mpu();
void init_tick_timer();
void init_lcd_controller();
void reset_lcd_controller();
void init_touch_controller();
void init_sdram();
void init_uarts(std::vector<ReceiveBuf*>&);
void init_gui();

// TODO: hardware accel for UI
// TODO: clock selection in user application
int main() {
    static std::vector<ReceiveBuf*> bufs;

    // required since clocks have been preconfigured by bootloader
    SystemCoreClockUpdate();

    if (HAL_Init() != HAL_OK) {
        on_error();
    }

    init_mpu();
    init_tick_timer();
    init_sdram();
    init_uarts(bufs);
    init_lcd_controller();
    init_touch_controller();
    init_gui();

    xTaskCreate(
        [](void* arg) {
            try {
                auto bufs = (std::vector<ReceiveBuf*>*) arg;
                run(*bufs);
            } catch (...) {
                on_error();
            }
        },
        "main",
        TASK_STACK_SIZE,
        &bufs,
        6,
        nullptr);

    vTaskStartScheduler();
}

void init_mpu() {
    HAL_MPU_Disable();

    // complete sdram address space:
    // normal, read and write caching, shareable
    MPU_Region_InitTypeDef generic_sdram;
    generic_sdram.Enable = true;
    generic_sdram.Number = MPU_REGION_NUMBER0;
    generic_sdram.BaseAddress = SDRAM_START_ADDR;
    generic_sdram.Size = MPU_REGION_SIZE_8MB;
    generic_sdram.SubRegionDisable = false;
    generic_sdram.TypeExtField = MPU_TEX_LEVEL1;
    generic_sdram.AccessPermission = MPU_REGION_FULL_ACCESS;
    generic_sdram.DisableExec = true;
    generic_sdram.IsShareable = true;
    generic_sdram.IsCacheable = true;
    generic_sdram.IsBufferable = true;
    HAL_MPU_ConfigRegion(&generic_sdram);

    // frame buffer 1 on sdram:
    // normal, read caching and write-through, shareable
    MPU_Region_InitTypeDef frame_buf_1;
    frame_buf_1.Enable = true;
    frame_buf_1.Number = MPU_REGION_NUMBER1;
    frame_buf_1.BaseAddress = FRAME_BUF_1_ADDR;
    frame_buf_1.Size = MPU_REGION_SIZE_512KB;
    frame_buf_1.SubRegionDisable = false;
    frame_buf_1.TypeExtField = MPU_TEX_LEVEL0;
    frame_buf_1.AccessPermission = MPU_REGION_FULL_ACCESS;
    frame_buf_1.DisableExec = true;
    frame_buf_1.IsShareable = true;
    frame_buf_1.IsCacheable = true;
    frame_buf_1.IsBufferable = false;
    HAL_MPU_ConfigRegion(&frame_buf_1);

    // frame buffer 2 on sdram
    // normal, read caching and write-through, shareable
    MPU_Region_InitTypeDef frame_buf_2;
    frame_buf_2.Enable = true;
    frame_buf_2.Number = MPU_REGION_NUMBER2;
    frame_buf_2.BaseAddress = FRAME_BUF_2_ADDR;
    frame_buf_2.Size = MPU_REGION_SIZE_512KB;
    frame_buf_2.SubRegionDisable = false;
    frame_buf_2.TypeExtField = MPU_TEX_LEVEL0;
    frame_buf_2.AccessPermission = MPU_REGION_FULL_ACCESS;
    frame_buf_2.DisableExec = true;
    frame_buf_2.IsShareable = true;
    frame_buf_2.IsCacheable = true;
    frame_buf_2.IsBufferable = false;
    HAL_MPU_ConfigRegion(&frame_buf_2);

    // complete qspi address space:
    // strongly-ordered, shareable (not accessible)
    MPU_Region_InitTypeDef generic_qspi;
    generic_qspi.Enable = true;
    generic_qspi.Number = MPU_REGION_NUMBER3;
    generic_qspi.BaseAddress = QSPI_START_ADDR;
    generic_qspi.Size = MPU_REGION_SIZE_256MB;
    generic_qspi.SubRegionDisable = false;
    generic_qspi.TypeExtField = MPU_TEX_LEVEL0;
    generic_qspi.AccessPermission = MPU_REGION_NO_ACCESS;
    generic_qspi.DisableExec = true;
    generic_qspi.IsShareable = true;
    generic_qspi.IsCacheable = false;
    generic_qspi.IsBufferable = false;
    HAL_MPU_ConfigRegion(&generic_qspi);

    // installed qspi flash:
    // normal, read caching and write-through, not shareable (read-only)
    MPU_Region_InitTypeDef installed_qspi;
    installed_qspi.Enable = true;
    installed_qspi.Number = MPU_REGION_NUMBER4;
    installed_qspi.BaseAddress = QSPI_START_ADDR;
    installed_qspi.Size = MPU_REGION_SIZE_16MB;
    installed_qspi.SubRegionDisable = false;
    installed_qspi.TypeExtField = MPU_TEX_LEVEL0;
    installed_qspi.AccessPermission = MPU_REGION_PRIV_RO;
    installed_qspi.DisableExec = false;
    installed_qspi.IsShareable = false;
    installed_qspi.IsCacheable = true;
    installed_qspi.IsBufferable = false;
    HAL_MPU_ConfigRegion(&installed_qspi);

    // null pointer page (fault on null pointer derefs)
    // strongly-ordered, shareable (not accessible)
    MPU_Region_InitTypeDef null_page;
    null_page.Enable = true;
    null_page.Number = MPU_REGION_NUMBER5;
    null_page.BaseAddress = 0x00000000;
    null_page.Size = MPU_REGION_SIZE_4KB;
    null_page.SubRegionDisable = false;
    null_page.TypeExtField = MPU_TEX_LEVEL0;
    null_page.AccessPermission = MPU_REGION_NO_ACCESS;
    null_page.DisableExec = true;
    null_page.IsShareable = true;
    null_page.IsCacheable = false;
    null_page.IsBufferable = false;
    HAL_MPU_ConfigRegion(&null_page);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void init_tick_timer() {
    // configure timer for incrementing the HAL tick counter (1000 Hz)
    // cannot use systick because FreeRTOS uses it and requires it to be lowest priority
    // equation: freq = (Clock / (Prescaler + 1)) / (Period + 1)
    __TIM3_CLK_ENABLE();
    TIMER3.Instance = TIM3;
    TIMER3.Init.Prescaler = 4000 - 1;
    TIMER3.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIMER3.Init.Period = 27 - 1;
    TIMER3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIMER3.Init.RepetitionCounter = 0;

    if (HAL_TIM_Base_Init(&TIMER3) != HAL_OK) {
        on_error();
    }

    if (HAL_TIM_Base_Start_IT(&TIMER3) != HAL_OK) {
        on_error();
    }

    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void init_lcd_controller() {
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
    gpio_config.Speed = GPIO_SPEED_FAST;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOE, &gpio_config);

    gpio_config.Pin = GPIO_PIN_12;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOG, &gpio_config);

    gpio_config.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOI, &gpio_config);

    gpio_config.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5
        | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11
        | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOJ, &gpio_config);

    gpio_config.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Alternate = GPIO_AF14_LTDC;
    HAL_GPIO_Init(GPIOK, &gpio_config);

    gpio_config.Pin = LCD_DISPLAY_ENABLE_PIN;
    gpio_config.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_DISPLAY_ENABLE_PORT, &gpio_config);

    gpio_config.Pin = LCD_BACKLIGHT_ENABLE_PIN;
    gpio_config.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(LCD_BACKLIGHT_ENABLE_PORT, &gpio_config);

    // lcd controller init
    LCD_CONTROLLER.Instance = LTDC;

    // make sure the controller is reset
    auto result = HAL_LTDC_DeInit(&LCD_CONTROLLER);
    if (result != HAL_OK) {
        on_error();
    }

    LCD_CONTROLLER.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    LCD_CONTROLLER.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    LCD_CONTROLLER.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    LCD_CONTROLLER.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    LCD_CONTROLLER.Init.HorizontalSync = 40;
    LCD_CONTROLLER.Init.VerticalSync = 9;
    LCD_CONTROLLER.Init.AccumulatedHBP = 53;
    LCD_CONTROLLER.Init.AccumulatedVBP = 11;
    LCD_CONTROLLER.Init.AccumulatedActiveW = 533;
    LCD_CONTROLLER.Init.AccumulatedActiveH = 283;
    LCD_CONTROLLER.Init.TotalWidth = 565;
    LCD_CONTROLLER.Init.TotalHeigh = 285;
    LCD_CONTROLLER.Init.Backcolor.Blue = 0;
    LCD_CONTROLLER.Init.Backcolor.Green = 0;
    LCD_CONTROLLER.Init.Backcolor.Red = 0;

    result = HAL_LTDC_Init(&LCD_CONTROLLER);
    if (result != HAL_OK) {
        on_error();
    }

    // enable display and backlight
    HAL_GPIO_WritePin(LCD_DISPLAY_ENABLE_PORT, LCD_DISPLAY_ENABLE_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_BACKLIGHT_ENABLE_PORT, LCD_BACKLIGHT_ENABLE_PIN, GPIO_PIN_SET);

    LTDC_LayerCfgTypeDef layer_config;
    layer_config.WindowX0 = 0;
    layer_config.WindowY0 = 0;
    layer_config.WindowX1 = DISPLAY_WIDTH;
    layer_config.WindowY1 = DISPLAY_HEIGHT;
    layer_config.PixelFormat = LTDC_PIXEL_FORMAT_RGB888;
    layer_config.Alpha = 255;
    layer_config.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer_config.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer_config.FBStartAdress = FRAME_BUF_1_ADDR;
    layer_config.ImageWidth = DISPLAY_WIDTH;
    layer_config.ImageHeight = DISPLAY_HEIGHT;
    layer_config.Alpha0 = 0;
    layer_config.Backcolor.Blue = 0;
    layer_config.Backcolor.Green = 0;
    layer_config.Backcolor.Red = 0;

    result = HAL_LTDC_ConfigLayer(&LCD_CONTROLLER, &layer_config, 0);
    if (result != HAL_OK) {
        on_error();
    }
}

void reset_lcd_controller() {
    HAL_LTDC_DeInit(&LCD_CONTROLLER);
    __HAL_RCC_LTDC_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_4);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_12);
    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

    HAL_GPIO_DeInit(
        GPIOJ,
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6
            | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_13
            | GPIO_PIN_14 | GPIO_PIN_15);

    HAL_GPIO_DeInit(
        GPIOK,
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

    HAL_GPIO_DeInit(LCD_DISPLAY_ENABLE_PORT, LCD_DISPLAY_ENABLE_PIN);
    HAL_GPIO_DeInit(LCD_BACKLIGHT_ENABLE_PORT, LCD_BACKLIGHT_ENABLE_PIN);
}

void init_touch_controller() {
    __HAL_RCC_GPIOH_CLK_ENABLE();

    GPIO_InitTypeDef gpio_config;
    gpio_config.Pin = GPIO_PIN_7 | GPIO_PIN_8;
    gpio_config.Mode = GPIO_MODE_AF_OD;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FAST;
    gpio_config.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOH, &gpio_config);

    __HAL_RCC_I2C3_CLK_ENABLE();
    __HAL_RCC_I2C3_FORCE_RESET();
    __HAL_RCC_I2C3_RELEASE_RESET();

    // enable interrupts
    HAL_NVIC_SetPriority(I2C3_EV_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
    HAL_NVIC_SetPriority(I2C3_ER_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);

    I2C_BUS3.Instance = I2C3;
    I2C_BUS3.Init.Timing = DISCOVERY_I2Cx_TIMING;
    I2C_BUS3.Init.OwnAddress1 = 0;
    I2C_BUS3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    I2C_BUS3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    I2C_BUS3.Init.OwnAddress2 = 0;
    I2C_BUS3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    I2C_BUS3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&I2C_BUS3) != HAL_OK) {
        on_error();
    }

    if (BSP_TS_Init(DISPLAY_WIDTH, DISPLAY_HEIGHT) != TS_OK) {
        on_error();
    }

    // configure timer for polling the touch controller (30 Hz)
    // equation: freq = (Clock / (Prescaler + 1)) / (Period + 1)
    __TIM2_CLK_ENABLE();
    TIMER2.Instance = TIM2;
    TIMER2.Init.Prescaler = 20000 - 1;
    TIMER2.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIMER2.Init.Period = 180 - 1;
    TIMER2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIMER2.Init.RepetitionCounter = 0;

    if (HAL_TIM_Base_Init(&TIMER2) != HAL_OK) {
        on_error();
    }

    if (HAL_TIM_Base_Start_IT(&TIMER2) != HAL_OK) {
        on_error();
    }

    HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

void init_sdram() {
    // GPIO and clock init
    __HAL_RCC_FMC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

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
        on_error();
    }

    // SDRAM initialization
    FMC_SDRAM_CommandTypeDef command;
    command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        on_error();
    }

    HAL_Delay(1);

    command.CommandMode = FMC_SDRAM_CMD_PALL;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        on_error();
    }

    command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    command.AutoRefreshNumber = 8;
    command.ModeRegisterDefinition = 0;

    result = HAL_SDRAM_SendCommand(&handle, &command, SDRAM_TIMEOUT);
    if (result != HAL_OK) {
        on_error();
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
        on_error();
    }

    result = HAL_SDRAM_ProgramRefreshRate(&handle, 1674);
    if (result != HAL_OK) {
        on_error();
    }
}

void init_uarts(std::vector<ReceiveBuf*>& bufs) {
    // init dma stream
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

    DMA2_STREAM1.Instance = DMA2_Stream1;
    DMA2_STREAM1.Init.Channel = DMA_CHANNEL_5;
    DMA2_STREAM1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    DMA2_STREAM1.Init.PeriphInc = DMA_PINC_DISABLE;
    DMA2_STREAM1.Init.MemInc = DMA_MINC_ENABLE;
    DMA2_STREAM1.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    DMA2_STREAM1.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    DMA2_STREAM1.Init.Mode = DMA_CIRCULAR;
    DMA2_STREAM1.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    DMA2_STREAM1.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    DMA2_STREAM1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    DMA2_STREAM1.Init.MemBurst = DMA_MBURST_INC8;
    DMA2_STREAM1.Init.PeriphBurst = DMA_PBURST_SINGLE;

    if (HAL_DMA_Init(&DMA2_STREAM1) != HAL_OK) {
        on_error();
    }

    // init uart
    __HAL_RCC_USART6_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio_config;
    gpio_config.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_NOPULL;
    gpio_config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_config.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &gpio_config);

    static UART_HandleTypeDef uart;
    uart.Instance = USART6;
    uart.Init.BaudRate = UART_BAUDRATE;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    uart.hdmarx = &DMA2_STREAM1;
    DMA2_STREAM1.Parent = &uart;

    if (HAL_UART_Init(&uart) != HAL_OK) {
        on_error();
    }

    // setup buffer/callbacks and start transfer
    static ReceiveBuf buf __attribute__((section(".dtcm_data")));

    HAL_DMA_RegisterCallback(&DMA2_STREAM1, HAL_DMA_XFER_HALFCPLT_CB_ID, [](auto) {
        buf.is_front_ready = true;
        buf.is_back_ready = false;
    });

    HAL_DMA_RegisterCallback(&DMA2_STREAM1, HAL_DMA_XFER_CPLT_CB_ID, [](auto) {
        buf.is_front_ready = false;
        buf.is_back_ready = true;
    });

    HAL_DMA_RegisterCallback(&DMA2_STREAM1, HAL_DMA_XFER_ERROR_CB_ID, [](auto) { on_error(); });

    auto src_addr = (uint32_t) &uart.Instance->RDR;
    auto dst_addr = (uint32_t) buf.bytes;

    if (HAL_DMA_Start_IT(&DMA2_STREAM1, src_addr, dst_addr, ReceiveBuf::LEN) != HAL_OK) {
        on_error();
    }

    // enable rx register not empty interrupt (handled by DMA controller)
    uart.Instance->CR1 |= USART_CR1_RXNEIE;

    // set DMA as receiver
    uart.Instance->CR3 |= USART_CR3_DMAR;

    bufs.push_back(&buf);
}

void init_gui() {
    // CRC is required by the ST implementation
    __HAL_RCC_CRC_CLK_ENABLE();

    if (GUI_Init() != 0) {
        on_error();
    }

    WM_MOTION_Enable(true);
    WM_MULTIBUF_Enable(true);
}

void poll_touch_state() {
    static TS_StateTypeDef last_state;

    TS_StateTypeDef current_state;
    BSP_TS_ResetTouchData(&current_state);

    if (BSP_TS_GetState(&current_state) != TS_OK) {
        return;
    }

    // do not fill the touch fifo if nothing changed (prevents data loss)
    if (last_state.touchDetected == current_state.touchDetected
        && last_state.touchX[0] == current_state.touchX[0]
        && last_state.touchY[0] == current_state.touchY[0]) {
        return;
    }

    GUI_PID_STATE state;
    state.x = current_state.touchX[0];
    state.y = current_state.touchY[0];
    state.Pressed = current_state.touchDetected > 0;
    state.Layer = 0;

    GUI_TOUCH_StoreStateEx(&state);
    last_state = current_state;
}

__attribute__((noinline)) void on_error() {
    GUI_Exit();
    reset_lcd_controller();

    while (1) {
        // spin
    }
}
