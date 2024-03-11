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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_STACK_SIZE		512
#define APP_PRIORITY		16
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
VOID two_thread_entry(ULONG thread_input);
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
  VOID *two_pointer;

  ret = tx_byte_allocate(byte_pool, &one_pointer, APP_STACK_SIZE, TX_NO_WAIT);
  if(ret != TX_SUCCESS)
  {
	  ret = TX_POOL_ERROR;
  }

  ret = tx_byte_allocate(byte_pool, &two_pointer, APP_STACK_SIZE, TX_NO_WAIT);
  if(ret != TX_SUCCESS)
  {
	  ret = TX_POOL_ERROR;
  }

  /* Create the semaphore. */
  tx_semaphore_create(&semaphore, "semaphore", 1);
  /* USER CODE END App_ThreadX_MEM_POOL */


  /* USER CODE BEGIN App_ThreadX_Init */
  ret = tx_thread_create(&one_thread, "One Thread", one_thread_entry, 0,
                            one_pointer, APP_STACK_SIZE,
                            APP_PRIORITY, APP_PRIORITY,
                            TX_NO_TIME_SLICE, TX_AUTO_START);
  if (ret != TX_SUCCESS)
  {
	  ret = TX_THREAD_ERROR;
  }

  ret = tx_thread_create(&two_thread, "Two Thread", two_thread_entry, 0,
                            two_pointer, APP_STACK_SIZE,
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

/* USER CODE BEGIN 1 */

VOID one_thread_entry(ULONG thread_input)
{
	UINT status;

    while(1)
    {
    	status = tx_semaphore_get(&semaphore, TX_WAIT_FOREVER);
    	if(TX_SUCCESS == status)
    	{
    		printf("one_thread_entry semaphore get\r\n");
    	}

    	status = tx_semaphore_put(&semaphore);
    	if(TX_SUCCESS != status)
    	{
    		printf("error: one_thread_entry semaphore put\r\n");
    	}
    	tx_thread_sleep(1);
    }
}

VOID two_thread_entry(ULONG thread_input)
{
	UINT status;

    while(1)
    {
    	status = tx_semaphore_get(&semaphore, TX_WAIT_FOREVER);
    	if(TX_SUCCESS == status)
    	{
    		printf("two_thread_entry semaphore get\r\n");
    	}

    	status = tx_semaphore_put(&semaphore);
    	if(TX_SUCCESS != status)
    	{
    		printf("error: two_thread_entry semaphore put\r\n");
    	}
    	tx_thread_sleep(1);
    }
}

/* USER CODE END 1 */
