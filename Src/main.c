/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
char answer[5] = {0};
uint8_t answer_ready = 0;
char guess[5];
uint8_t guess_len = 0;
uint8_t input_overflow = 0;
uint8_t game_rx;

/* 0: off, 1: A, 2: B */
uint8_t led_state[4] = {0, 0, 0, 0};

uint32_t round_number = 0;

const uint32_t led_channel[4] = {
    TIM_CHANNEL_1,
    TIM_CHANNEL_2,
    TIM_CHANNEL_3,
    TIM_CHANNEL_4
};
volatile uint8_t restart_requested = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
static void GenerateAnswer(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void GenerateAnswer(void)
{
    static uint8_t seeded = 0;
    char next[5];

    if (!seeded)
    {
        /* Sample human input timing; TIM3 counts at 1 MHz. */
        uint32_t seed = HAL_GetTick() * 1000U
                      + __HAL_TIM_GET_COUNTER(&htim3);
        srand((unsigned int)seed);
        seeded = 1;
    }

    do
    {
        char digits[10] = "0123456789";
        for (uint8_t i = 0; i < 4; i++)
        {
            uint8_t j = i + (uint8_t)(rand() % (10 - i));
            char temp = digits[i];
            digits[i] = digits[j];
            digits[j] = temp;
            next[i] = digits[i];
        }
        next[4] = '\0';
    } while (answer_ready && memcmp(next, answer, 4) == 0);

    memcpy(answer, next, sizeof(answer));
    answer_ready = 1;
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
  MX_LPUART1_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  uint8_t ready[] = "STM32 UART Ready. Enter 4 different digits.\r\n";
  HAL_UART_Transmit(&hlpuart1, ready, sizeof(ready) - 1, 100);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
  if (restart_requested)
  {
      restart_requested = 0;

      char restart_msg[80];
      int n;
      if (answer_ready)
      {
          n = snprintf(restart_msg, sizeof(restart_msg),
                       "\r\nAnswer: %s\r\nNew game!\r\n", answer);
      }
      else
      {
          n = snprintf(restart_msg, sizeof(restart_msg),
                       "\r\nNew game!\r\n");
      }
      GenerateAnswer();

      guess_len = 0;
      input_overflow = 0;
      round_number = 0;

      for (uint8_t i = 0; i < 4; i++)
      {
          led_state[i] = 0;
          __HAL_TIM_SET_COMPARE(&htim3, led_channel[i], 0);
      }

      if (n > 0 && n < (int)sizeof(restart_msg))
      {
          HAL_UART_Transmit(
              &hlpuart1,
              (uint8_t *)restart_msg,
              (uint16_t)n,
              100
          );
      }
  }


      /* Receive one character */
      if (HAL_UART_Receive(&hlpuart1, &game_rx, 1, 1) == HAL_OK)
      {
          /* First user input supplies timing for the initial seed. */
          if (!answer_ready && game_rx != '\r' && game_rx != '\n')
          {
              GenerateAnswer();
          }

          if (game_rx == '\r' || game_rx == '\n')
          {
              /* Ignore empty lines, including the LF after CR */
              if (guess_len > 0 || input_overflow)
              {
                  uint8_t valid =
                      (guess_len == 4 && !input_overflow);

                  /* Check digits and reject repeated digits */
                  for (uint8_t i = 0; i < guess_len; i++)
                  {
                      if (guess[i] < '0' || guess[i] > '9')
                      {
                          valid = 0;
                      }

                      for (uint8_t j = 0; j < i; j++)
                      {
                          if (guess[i] == guess[j])
                          {
                              valid = 0;
                          }
                      }
                  }

                  char reply[80];
                  int reply_len;

                  if (valid)
                  {
                      uint8_t count_a = 0;
                      uint8_t count_b = 0;

                      guess[4] = '\0';

                      for (uint8_t i = 0; i < 4; i++)
                      {
                          led_state[i] = 0;

                          if (guess[i] == answer[i])
                          {
                              led_state[i] = 1;
                              count_a++;
                          }
                          else
                          {
                              for (uint8_t j = 0; j < 4; j++)
                              {
                                  if (guess[i] == answer[j])
                                  {
                                      led_state[i] = 2;
                                      count_b++;
                                      break;
                                  }
                              }
                          }
                      }

                      round_number++;

                      reply_len = snprintf(
                          reply, sizeof(reply),
                          "Round %lu: %s -> %uA%uB\r\n",
                          (unsigned long)round_number,
                          guess,
                          (unsigned int)count_a,
                          (unsigned int)count_b
                      );
                  }
                  else
                  {
                      reply_len = snprintf(
                          reply, sizeof(reply),
                          "Enter 4 different digits, then Enter.\r\n"
                      );
                  }

                  if (reply_len > 0 &&
                      reply_len < (int)sizeof(reply))
                  {
                      HAL_UART_Transmit(
                          &hlpuart1,
                          (uint8_t *)reply,
                          (uint16_t)reply_len,
                          100
                      );
                  }

                  guess_len = 0;
                  input_overflow = 0;
              }
          }
          else
          {
              if (guess_len < 4)
              {
                  guess[guess_len++] = (char)game_rx;
              }
              else
              {
                  input_overflow = 1;
              }
          }
      }

      /* Update the four LEDs */
      uint32_t phase = HAL_GetTick() % 2000U;
      uint32_t breathing =
          (phase <= 1000U) ? phase : (2000U - phase);

      for (uint8_t i = 0; i < 4; i++)
      {
          uint32_t brightness = 0;

          if (led_state[i] == 1)
          {
              brightness = 1000;
          }
          else if (led_state[i] == 2)
          {
              brightness = breathing;
          }

          __HAL_TIM_SET_COMPARE(
              &htim3, led_channel[i], brightness
          );
      }
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 169;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  GPIO_InitTypeDef button = {0};
  button.Pin = GPIO_PIN_0;
  button.Mode = GPIO_MODE_IT_FALLING;
  button.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &button);

  HAL_NVIC_SetPriority(EXTI0_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);


  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_press = 0;
    static uint8_t first_press = 1;

    if (GPIO_Pin == GPIO_PIN_0)
    {
        uint32_t now = HAL_GetTick();

        /* Ignore repeated triggers within 200 ms */
        if (first_press || (uint32_t)(now - last_press) >= 200U)
        {
            first_press = 0;
            last_press = now;
            restart_requested = 1;
        }
    }
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
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
