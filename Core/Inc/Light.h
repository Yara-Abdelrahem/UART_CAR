#include <stdint.h>
#include "stm32f4xx_hal.h"

// Light on PB1
#define LIGHT_GPIO_PORT     GPIOB
#define LIGHT_Front_PIN     GPIO_PIN_1
#define LIGHT_Back_PIN      GPIO_PIN_10
#define LIGHT_Right_PIN     GPIO_PIN_12
#define LIGHT_Left_PIN      GPIO_PIN_13

void Light_Init(void);

void Light_Front_On(void);
void Light_Front_Off(void);
void Light_Back_On(void);
void Light_Back_Off(void);
void Light_Right_On(void);
void Light_Right_Off(void);
void Light_Left_Off(void);
void Light_Left_Off(void);