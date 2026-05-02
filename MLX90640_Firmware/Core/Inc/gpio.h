/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/******************************************************************************************/
/* ���� ���� */
#define LED_Pin GPIO_PIN_1
#define LED_GPIO_Port GPIOA
#define LED_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)             /* PF��ʱ��ʹ�� */

/* LED�˿ڶ��� */
#define LED(x)   do{ x ? \
                      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); \
                  }while(0)       /* LED0 = RED */

/* LEDȡ������ */
#define LED_TOGGLE()    do{ HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); }while(0)       /* LED0 = !LED0 */

/******************************************************************************************/
/* �ⲿ�ӿں���*/
void led_init(void);                                                                            /* ��ʼ�� */
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

