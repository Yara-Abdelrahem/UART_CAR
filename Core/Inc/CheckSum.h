#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
extern "C" {
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "uart.h"

uint16_t crc16_table_calc(const uint8_t *data, size_t length);
uint16_t checksum(uint8_t* myData, uint8_t size);

#ifdef __cplusplus
}
#endif
