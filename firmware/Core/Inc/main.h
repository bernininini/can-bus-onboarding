/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

/* Pin names come from the Dev-A schematic. LED_A..LED_H are PG1..PG8. */
#define LED_A_Pin GPIO_PIN_1
#define LED_B_Pin GPIO_PIN_2
#define LED_C_Pin GPIO_PIN_3
#define LED_D_Pin GPIO_PIN_4
#define LED_E_Pin GPIO_PIN_5
#define LED_F_Pin GPIO_PIN_6
#define LED_G_Pin GPIO_PIN_7
#define LED_H_Pin GPIO_PIN_8
#define LED_GPIO_Port GPIOG

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
