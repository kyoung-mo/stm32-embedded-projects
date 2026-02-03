/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_RED_Pin GPIO_PIN_13
#define LED_RED_GPIO_Port GPIOC
#define RGB_R_Pin GPIO_PIN_0
#define RGB_R_GPIO_Port GPIOC
#define RGB_G_Pin GPIO_PIN_1
#define RGB_G_GPIO_Port GPIOC
#define RGB_B_Pin GPIO_PIN_2
#define RGB_B_GPIO_Port GPIOC
#define LED_GREEN_Pin GPIO_PIN_3
#define LED_GREEN_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define JOW_SW_Pin GPIO_PIN_0
#define JOW_SW_GPIO_Port GPIOB
#define DHT11_DATA_Pin GPIO_PIN_1
#define DHT11_DATA_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_10
#define BUZZER_GPIO_Port GPIOB
#define KEYPAD_COL3_Pin GPIO_PIN_12
#define KEYPAD_COL3_GPIO_Port GPIOB
#define KEYPAD_COL4_Pin GPIO_PIN_13
#define KEYPAD_COL4_GPIO_Port GPIOB
#define KEYPAD_ROW1_Pin GPIO_PIN_8
#define KEYPAD_ROW1_GPIO_Port GPIOA
#define KEYPAD_ROW2_Pin GPIO_PIN_9
#define KEYPAD_ROW2_GPIO_Port GPIOA
#define KEYPAD_ROW3_Pin GPIO_PIN_10
#define KEYPAD_ROW3_GPIO_Port GPIOA
#define KEYPAD_ROW4_Pin GPIO_PIN_11
#define KEYPAD_ROW4_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define KEYPAD_COL1_Pin GPIO_PIN_5
#define KEYPAD_COL1_GPIO_Port GPIOB
#define KEYPAD_COL2_Pin GPIO_PIN_6
#define KEYPAD_COL2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
