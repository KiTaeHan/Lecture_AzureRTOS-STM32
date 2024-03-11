/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
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
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_STACK_SIZE		512
#define APP_PRIORITY		16
#define DEMO_QUEUE_SIZE		10
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
TX_THREAD one_thread;
TX_THREAD two_thread;
TX_SEMAPHORE semaphore;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
VOID one_thread_entry(ULONG thread_input);
void App_Delay(ULONG Delay);
/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  VOID *one_pointer;

  ret = tx_byte_allocate(byte_pool, &one_pointer, APP_STACK_SIZE, TX_NO_WAIT);
  if(ret != TX_SUCCESS)
  {
	  ret = TX_POOL_ERROR;
  }
  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  // Create Semaphore.
  if (tx_semaphore_create(&semaphore, "Semaphore", 0) != TX_SUCCESS)
  {
	  return TX_SEMAPHORE_ERROR;
  }

  ret = tx_thread_create(&one_thread, "One Thread", one_thread_entry, 0,
                            one_pointer, APP_STACK_SIZE,
                            APP_PRIORITY, APP_PRIORITY,
                            TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS)
  {
	  ret = TX_THREAD_ERROR;
  }

  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/**
  * @brief  App_ThreadX_LowPower_Timer_Setup
  * @param  count : TX timer count
  * @retval None
  */
void App_ThreadX_LowPower_Timer_Setup(ULONG count)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Timer_Setup */

  /* USER CODE END  App_ThreadX_LowPower_Timer_Setup */
}

/**
  * @brief  App_ThreadX_LowPower_Enter
  * @param  None
  * @retval None
  */
void App_ThreadX_LowPower_Enter(void)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Enter */
	printf("App_ThreadX_LowPower_Enter\r\n");
	// Enter to the stop mode
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
  /* USER CODE END  App_ThreadX_LowPower_Enter */
}

/**
  * @brief  App_ThreadX_LowPower_Exit
  * @param  None
  * @retval None
  */
void App_ThreadX_LowPower_Exit(void)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Exit */
	// Reconfigure the system clock
	HAL_RCC_DeInit();
	SystemClock_Config();
	printf("App_ThreadX_LowPower_Exit\r\n");
  /* USER CODE END  App_ThreadX_LowPower_Exit */
}

/**
  * @brief  App_ThreadX_LowPower_Timer_Adjust
  * @param  None
  * @retval Amount of time (in ticks)
  */
ULONG App_ThreadX_LowPower_Timer_Adjust(void)
{
  /* USER CODE BEGIN  App_ThreadX_LowPower_Timer_Adjust */
  return 0;
  /* USER CODE END  App_ThreadX_LowPower_Timer_Adjust */
}

/* USER CODE BEGIN 1 */

VOID one_thread_entry(ULONG thread_input)
{
	(void) thread_input;
	uint32_t i;

	while(1)
	{
		if (tx_semaphore_get(&semaphore, TX_WAIT_FOREVER) == TX_SUCCESS)
	    {
    		for(i=0; i<0xFF; i++)
    		{
    			printf("one thread loop\r\n");
    		}
	    }
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	ULONG currentValue = 0;

	if(USER_BUTTON_Pin == GPIO_Pin)
	{
	    /* Add additional checks to avoid multiple semaphore puts by successively
	    clicking on the user button */
	    tx_semaphore_info_get(&semaphore, NULL, &currentValue, NULL, NULL, NULL);
	    if (currentValue == 0)
	    {
	    	/* Put the semaphore to release the MainThread */
	    	tx_semaphore_put(&semaphore);
	    }
	}
}

/* USER CODE END 1 */
