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
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>

#include <colorutils.h>
#include <command.h>
#include "led_modes.h"
#include "bkp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
struct play_mode {
	enum LED_MODES oldMode;
  int oldModePos;
  uint32_t oldModeNextTick;
  uint16_t oldModeGlitter;

	enum LED_MODES curMode;
  int curModePos;
  uint32_t curModeNextTick;
  uint16_t curModeGlitter;

  uint32_t transitionStartTick;
};
#define GLITTER_ENABLE UINT16_C(0x100)
#define GLITTER_MASK UINT16_C(0xFF)
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
struct CM_HAL_WS281x ws281x;
struct CM_HAL_IRREMOTE irremote;
struct CM_HAL_BTN button;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static inline uint32_t GetNextTick(int delay, uint8_t speed)
{
	if(delay == NO_UPDATE) {
		return HAL_MAX_DELAY;
	}
	if(delay == DEFAULT_DELAY) {
		delay = 1000/60;
	}
	return HAL_GetTick()+delay;
}

void SetMode(struct play_mode *mode, enum LED_MODES new_mode)
{
  mode->oldMode = mode->curMode;
  mode->oldModePos = mode->curModePos;
  mode->oldModeNextTick = mode->curModeNextTick;
  memcpy(xmas_buffer_from, xmas_buffer, 3*XMAS_LENGTH);

  mode->curMode = new_mode;
  mode->curModePos = 0;
  mode->curModeNextTick = 0;

  mode->transitionStartTick = HAL_GetTick();
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
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
  CM_HAL_BTN_Init(&button);

  CM_HAL_IRREMOTE_Init(&irremote, TIM3);
  CM_HAL_IRREMOTE_Start_IT(&irremote);

  CM_HAL_WS281X_Init(&ws281x, XMAS_GPIO_Port, TIM2);
  struct CM_HAL_WS281X_Channel xmas_chan1 = {
  		.GPIO_Pin = XMAS_Pin,
			.frameBuffer = (uint8_t*)xmas_buffer,
			.frameBufferSize = sizeof(xmas_buffer),
			.colorMode = WS281x_RGB
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

  uint8_t hasBackup = HAL_RTCEx_BKUPRead(&hrtc, BKP_HasBackup);
  HAL_RTCEx_BKUPWrite(&hrtc, BKP_HasBackup, 1);

  struct play_mode mode = {0};
  mode.curMode = hasBackup?HAL_RTCEx_BKUPRead(&hrtc, BKP_LastMode):MODE_Start;
  mode.curModeGlitter = hasBackup?HAL_RTCEx_BKUPRead(&hrtc, BKP_Glitter):0;

  uint8_t brightness = hasBackup?HAL_RTCEx_BKUPRead(&hrtc, BKP_Brightness):127;
  uint8_t speed = hasBackup?HAL_RTCEx_BKUPRead(&hrtc, BKP_Speed):127;
  if(!hasBackup) {
  	HAL_RTCEx_BKUPWrite(&hrtc, BKP_LastMode, mode.curMode);
  	HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, mode.curModeGlitter);
  	HAL_RTCEx_BKUPWrite(&hrtc, BKP_Brightness, brightness);
  	HAL_RTCEx_BKUPWrite(&hrtc, BKP_Speed, speed);
  }

  uint8_t powerState = 0;


  while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		struct command cmd = { .type = CMD_NoCommand };
		if (irremote.rcvstate == IRREMOTE_DONE) {
			GetCommandFromIR(&cmd);
			CM_HAL_IRREMOTE_Start_IT(&irremote);
		}
		else if (CM_HAL_BTN_hasClicks(&button)) {
			GetCommandFromBtn(&cmd);
			if (!powerState && cmd.type == CMD_ChangeMode) {
				// special case. Use hardware button single click to power on
				cmd.type = CMD_Power;
			}
		}

		if (powerState || cmd.type == CMD_Power) {
			switch (cmd.type) {
			case CMD_Power:
				powerState = !powerState;
				if (powerState) {
					enum LED_MODES new_mode = mode.curMode;
					mode.curMode = MODE_Off;
					SetMode(&mode, new_mode);
				}
				else {
					SetMode(&mode, MODE_Off);
				}
				break;
			case CMD_SetMode:
				SetMode(&mode, cmd.mode);
				break;
			case CMD_ToggleGlitter:
				if (mode.curModeGlitter & GLITTER_ENABLE) {
					mode.curModeGlitter &= ~(GLITTER_ENABLE);
				}
				else {
					mode.curModeGlitter |= GLITTER_ENABLE;
				}
				HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, mode.curModeGlitter);
				break;
			case CMD_GlitterChance: {
				uint8_t glitter = mode.curModeGlitter & GLITTER_MASK;
				if (cmd.direction > 0) {
					glitter = qadd8(glitter, 1);
				}
				else if (cmd.direction < 0) {
					glitter = qsub8(glitter, 1);
				}
				mode.curModeGlitter = (mode.curModeGlitter & GLITTER_ENABLE) | glitter;
				HAL_RTCEx_BKUPWrite(&hrtc, BKP_Glitter, mode.curModeGlitter);
				break;
			}
			case CMD_ChangeMode: {
				enum LED_MODES new_mode = mode.curMode + cmd.direction;
				if (new_mode == MODE_Off) {
					new_mode = MODE_Last - 1;
				}
				else if (new_mode == MODE_Last) {
					new_mode = MODE_Off + 1;
				}
				SetMode(&mode, new_mode);
				break;
			}
			case CMD_Brightness:
				if (cmd.direction > 0) {
					brightness = qadd8(brightness, 1);
				}
				else if (cmd.direction < 0) {
					brightness = qsub8(brightness, 1);
				}
				HAL_RTCEx_BKUPWrite(&hrtc, BKP_Brightness, brightness);
				break;
			case CMD_Speed:
				if (cmd.direction > 0) {
					speed = qadd8(speed, 1);
				}
				else if (cmd.direction < 0) {
					speed = qsub8(speed, 1);
				}
				HAL_RTCEx_BKUPWrite(&hrtc, BKP_Speed, speed);
				break;
			case CMD_NoCommand:
				break;
			}
		}

		if(mode.oldMode == MODE_Off && !powerState)
			continue;

    bool hasUpdates = false;
  	uint32_t tick = HAL_GetTick();
    if(mode.oldMode == mode.curMode) {
    	if(tick >= mode.curModeNextTick) {
    		mode.curModeNextTick = GetNextTick(FillMode(mode.curMode, xmas_buffer, XMAS_LENGTH, &mode.curModePos), speed);
    		hasUpdates = true;
    		if(mode.curMode != MODE_Off && mode.curModeGlitter){
    		  add_glitter(xmas_buffer, XMAS_LENGTH, 30);
    		}
    	}
    }
    else {
    	if(mode.oldModeNextTick != HAL_MAX_DELAY && tick >= mode.oldModeNextTick) {
    		mode.oldModeNextTick = GetNextTick(FillMode(mode.oldMode, xmas_buffer_from, XMAS_LENGTH, &mode.oldModePos), speed);
        if(mode.oldMode != MODE_Off && mode.oldModeGlitter){
          add_glitter(xmas_buffer_from, XMAS_LENGTH, 30);
        }
    	}
    	if(mode.curModeNextTick != HAL_MAX_DELAY && tick >= mode.curModeNextTick) {
    		mode.curModeNextTick = GetNextTick(FillMode(mode.curMode, xmas_buffer_to, XMAS_LENGTH, &mode.curModePos), speed);
        if(mode.curMode != MODE_Off && mode.curModeGlitter){
          add_glitter(xmas_buffer_to, XMAS_LENGTH, mode.curModeGlitter&GLITTER_MASK);
        }
    	}
    	fract16 transition = (tick-mode.transitionStartTick)*256/1000;
    	if(transition > 0xFF)
    		transition = 0xFF;
      blend_leds(xmas_buffer_from, xmas_buffer_to, xmas_buffer, XMAS_LENGTH, ease8InOutCubic(transition));
      if(transition == 0xFF) {
      	if(!powerState) {
      		//Special case of power off state:

      		// Save previous mode in _curMode_
      		mode.curMode = mode.oldMode;
      		// Set old mode to power off
      		mode.oldMode = MODE_Off;
      		// It will be later checked to skip mode calculations
      	}
      	else {
          mode.oldMode = mode.curMode;
          if(mode.curMode != MODE_Off) {
          	HAL_RTCEx_BKUPWrite(&hrtc, BKP_LastMode, mode.curMode);
          }
      	}
      }
      hasUpdates = true;
    }

    if(hasUpdates) {
    	nscale8(xmas_buffer, XMAS_LENGTH, brightness);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
