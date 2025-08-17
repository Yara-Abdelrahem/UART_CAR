#include <stdint.h>
#include "stm32f4xx_hal.h"

// Light on PB1
#define LIGHT_GPIO_PORT GPIOB
#define LIGHT_PIN       GPIO_PIN_0

void Light_Init(void) ;

void Light_On(void);

void Light_Off(void);