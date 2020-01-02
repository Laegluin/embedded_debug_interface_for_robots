#ifndef APP_H
#define APP_H

#include <stm32f7xx.h>
#include <vector>

void run(const std::vector<UART_HandleTypeDef*>& uarts);

#endif
