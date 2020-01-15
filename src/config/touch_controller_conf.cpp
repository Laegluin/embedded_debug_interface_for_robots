#include "main.h"
#include <stm32f7xx.h>

extern "C" void TS_IO_Init(void) {
    // init done in main
}

extern "C" void TS_IO_Write(uint8_t Addr, uint8_t Reg, uint8_t Value) {
    HAL_StatusTypeDef result =
        HAL_I2C_Mem_Write(&I2C_BUS3, Addr, (uint16_t) Reg, I2C_MEMADD_SIZE_8BIT, &Value, 1, 1000);

    if (result != HAL_OK) {
        on_error();
    }
}

extern "C" uint8_t TS_IO_Read(uint8_t Addr, uint8_t Reg) {
    uint8_t value;

    HAL_StatusTypeDef result =
        HAL_I2C_Mem_Read(&I2C_BUS3, Addr, (uint16_t) Reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 1000);

    if (result != HAL_OK) {
        on_error();
    }

    return value;
}

extern "C" void TS_IO_Delay(uint32_t Delay) {
    HAL_Delay(Delay);
}
