#include "uart.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define HAL_MAX_DELAY      0xFFFFFFFFU


void uart_log_init(UART_HandleTypeDef *huart) { 

 }

void uart_log_send(const char *data, uint16_t len) {
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, HAL_MAX_DELAY);
}

void uart_log_printf(const char *fmt, ...) {
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    uart_log_send(buffer, (uint16_t)strlen(buffer));
}
