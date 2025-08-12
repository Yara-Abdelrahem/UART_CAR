#ifndef UART_H
#define UART_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>
#include <stdint.h>

/* forward declare the HAL type so header doesn't require including HAL */
typedef struct __UART_HandleTypeDef UART_HandleTypeDef;

void uart_log_init(UART_HandleTypeDef *huart);
void uart_log_send(const char *data, uint16_t len);
void uart_log_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif // UART_H
