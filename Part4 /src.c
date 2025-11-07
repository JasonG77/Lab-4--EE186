/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAMPLES        256            // LUT size used by DAC DMA
#define DAC_OFFSET     2048
#define PI             3.14159265359f

// Theremin tuning
#define FREQ_MIN_HZ    200.0f         // low end of pitch sweep
#define FREQ_MAX_HZ    1200.0f        // high end
#define ADC_SMOOTH_A   0.15f          // EMA smoothing (0..1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// ---- Analog constants ----

uint16_t SineTable[SAMPLES];
static float adc_ema = 0.0f;     // smoothed ADC reading
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static float map_adc_to_freq(uint32_t adc);
static void  changeNote(float freq_hz);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
  // Change hlpuart1 to the UART that goes to your PC (ST-LINK VCP).
  HAL_UART_Transmit(&hlpuart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
  return len;
}

static float map_adc_to_freq(uint32_t adc)
{
  float x = (float)adc / 4095.0f;                         // 0..1
  return FREQ_MIN_HZ * powf(FREQ_MAX_HZ / FREQ_MIN_HZ, x);
}

// Retune TIM6 (which clocks the DAC) to change output frequency without stopping DMA.
// f_out = Fs / SAMPLES, with Fs = (PCLK1)/((PSC+1)*(ARR+1)).
static void changeNote(float freq_hz)
{
  if (freq_hz < 1.0f) return;

  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t psc   = htim6.Init.Prescaler + 1;              // use the PSC you already configured
  float    Fs    = freq_hz * (float)SAMPLES;              // desired DAC update rate

  uint32_t arr = (uint32_t)((float)pclk1 / ((float)psc * Fs) + 0.5f);
  if (arr == 0) arr = 1;
  arr -= 1;

  __HAL_TIM_DISABLE(&htim6);
  __HAL_TIM_SET_AUTORELOAD(&htim6, arr);
  __HAL_TIM_ENABLE(&htim6);
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_LPUART1_UART_Init();
  MX_DAC1_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */

  // Build sine LUT (centered, full-scale). You can scale amplitude if you like.
  for (int i = 0; i < SAMPLES; i++)
  {
    float theta = (2.0f * PI * i) / (float)SAMPLES;
    float s = sinf(theta);                       // -1..+1
    SineTable[i] = (uint16_t)(DAC_OFFSET + 2047.0f * s);   // 0..4095
  }
  setvbuf(stdout, NULL, _IONBF, 0);                                // <-- NEW

  // Start DAC with DMA (circular)                                // <-- NEW
  HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1,
                    (uint32_t*)SineTable, SAMPLES, DAC_ALIGN_12B_R);

  // Start TIM6 so it clocks the DAC via TRGO                      // <-- NEW
  HAL_TIM_Base_Start(&htim6);

  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  uint32_t psc   = htim6.Init.Prescaler + 1;
  uint32_t arr   = htim6.Init.Period + 1;
  uint32_t Fs    = pclk1 / psc / arr;
  float     f0   = (float)Fs / (float)SAMPLES;
  printf("Start Fs=%lu Hz, f_out≈%.2f Hz, SAMPLES=%d\r\n",
         (unsigned long)Fs, (double)f0, SAMPLES);

  /* USER CODE END 2 */


  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // Read LDR on PC1 / ADC1_IN2 (single conversion), smooth, map, retune
	  HAL_ADC_Start(&hadc1);
	  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	  uint32_t raw = HAL_ADC_GetValue(&hadc1);

	  adc_ema = (1.0f - ADC_SMOOTH_A) * adc_ema + ADC_SMOOTH_A * (float)raw;

	  float freq = map_adc_to_freq((uint32_t)adc_ema);
	  changeNote(freq);

	  // Optional debug
	  // printf("ADC=%4lu  freq=%.1f Hz\r\n", (unsigned long)raw, (double)freq);

	  HAL_Delay(5);  // smooth pitch updates without audible zippering;
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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

