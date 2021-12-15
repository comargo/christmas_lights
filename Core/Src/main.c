/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>

#include <colorutils.h>

#include "ir_commands.h"
#include "led_modes.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
void OnButtonClicked(struct CM_HAL_BTN *btn, void *pUserData, enum CM_HAL_BTN_Reason reason);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define XMAS_LENGTH 200
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
RGB xmas_buffer[XMAS_LENGTH];
RGB xmas_buffer_from[XMAS_LENGTH];
RGB xmas_buffer_to[XMAS_LENGTH];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static inline uint32_t GetNextTick(int delay)
{
	if(delay == NO_UPDATE) {
		return HAL_MAX_DELAY;
	}
	if(delay == DEFAULT_DELAY) {
		delay = 1000/60;
	}
	return HAL_GetTick()+delay;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  button.callback = &OnButtonClicked;
  CM_HAL_BTN_Init(&button);

  CM_HAL_IRREMOTE_Init(&irremote, TIM3);
  CM_HAL_IRREMOTE_Start_IT(&irremote);

  CM_HAL_WS281X_Init(&ws281x, XMAS_GPIO_Port, TIM2);
  struct CM_HAL_WS281X_Channel xmas_chan1 = {
  		.GPIO_Pin = XMAS_Pin,
			.frameBuffer = (uint8_t*)xmas_buffer,
			.frameBufferSize = sizeof(xmas_buffer),
			.colorMode = WS281x_GRB
  };
  CM_HAL_WS281X_AddChannel(&ws281x, &xmas_chan1);
  memset(xmas_buffer, 0, sizeof(xmas_buffer));
  CM_HAL_WS281X_SendBuffer(&ws281x);
	while(ws281x.state != WS281x_Ready) {
		__NOP();
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
//  size_t pos = 0;
	enum LED_MODES oldMode = 0;
  int oldModePos = 0;
  uint32_t oldModeNextTick = 0;

	enum LED_MODES curMode = 0;
  int curModePos = 0;
  uint32_t curModeNextTick = 0;

  uint32_t transitionStartTick;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(irremote.rcvstate == IRREMOTE_DONE) {
      enum LED_MODES newMode = GetModeFromIR(curMode);
      if(newMode != MODE_INVALID) {
        oldMode = curMode;
        oldModePos = curModePos;
        oldModeNextTick = curModeNextTick;
        memcpy(xmas_buffer_from, xmas_buffer, 3*XMAS_LENGTH);

        curMode = newMode;
        curModePos = 0;
        curModeNextTick = 0;

        transitionStartTick = HAL_GetTick();
      }
      CM_HAL_IRREMOTE_Start_IT(&irremote);
    }

    bool hasUpdates = false;
  	uint32_t tick = HAL_GetTick();
    if(oldMode == curMode) {
    	if(tick >= curModeNextTick) {
    		curModeNextTick = GetNextTick(FillMode(curMode, xmas_buffer, XMAS_LENGTH, &curModePos));
    		hasUpdates = true;
    	}
    }
    else {
    	if(oldModeNextTick != HAL_MAX_DELAY && tick >= oldModeNextTick) {
        oldModeNextTick = GetNextTick(FillMode(oldMode, xmas_buffer_from, XMAS_LENGTH, &oldModePos));
    	}
    	if(curModeNextTick != HAL_MAX_DELAY && tick >= curModeNextTick) {
    		curModeNextTick = GetNextTick(FillMode(curMode, xmas_buffer_to, XMAS_LENGTH, &curModePos));
    	}
    	fract16 transition = (tick-transitionStartTick)*256/1000;
    	if(transition > 0xFF)
    		transition = 0xFF;
      blend_leds(xmas_buffer_from, xmas_buffer_to, xmas_buffer, XMAS_LENGTH, ease8InOutCubic(transition));
      if(transition == 0xFF) {
        oldMode = curMode;
      }
      hasUpdates = true;
    }

    if(hasUpdates) {
    	CM_HAL_WS281X_SendBuffer(&ws281x);
    	while(ws281x.state != WS281x_Ready) {
    		__NOP();
    	}
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void OnButtonClicked(struct CM_HAL_BTN *btn, void *userData, enum CM_HAL_BTN_Reason reason)
{
	switch (reason) {
		case CM_HAL_BTN_CB_PRESSED:
			printf("Button pressed\r\n");
			break;
		case CM_HAL_BTN_CB_RELEASED:
			printf("Button released\r\n");
			break;
		case CM_HAL_BTN_CB_CLICK_TIMEOUT:
			printf("Button times out\r\n");
			break;
		default:
			break;
	}
}

#define FAULT_DELAY(x) for (int i=0; i<x*200000; ++i) __NOP()

static void dot(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	  HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
	  FAULT_DELAY(1);
	  HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

static void dash(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	  HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
	  FAULT_DELAY(3);
	  HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  for(int i=0; i<3; ++i) {
		  dot(LED_GPIO_Port, LED_Pin);
		  FAULT_DELAY(1);
	  }
	  FAULT_DELAY(2);
	  for(int i=0; i<3; ++i) {
		  dash(LED_GPIO_Port, LED_Pin);
		  FAULT_DELAY(1);
	  }
	  FAULT_DELAY(2);
	  for(int i=0; i<3; ++i) {
		  dot(LED_GPIO_Port, LED_Pin);
		  FAULT_DELAY(1);
	  }
	  FAULT_DELAY(6);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
