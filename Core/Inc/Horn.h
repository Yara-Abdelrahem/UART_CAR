#include <stdint.h>
#include "stm32f4xx_hal.h"

// Light on PB1
#define HORN_GPIO_PORT GPIOB
#define HORN_PIN    GPIO_PIN_1

void Horn_Init(void);
void Horn_On(void);
void Horn_Off(void);
void Horn_Toggle(void);