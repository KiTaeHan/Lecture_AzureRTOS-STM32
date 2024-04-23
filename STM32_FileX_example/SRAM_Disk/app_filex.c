
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_filex.c
  * @author  MCD Application Team
  * @brief   FileX applicative file
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
#include "app_filex.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* Main thread stack size */
#define FX_APP_THREAD_STACK_SIZE         1024
/* Main thread priority */
#define FX_APP_THREAD_PRIO               10

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Main thread global data structures.  */
TX_THREAD       fx_app_thread;

/* Buffer for FileX FX_MEDIA sector cache. */
uint32_t fx_sram_media_memory[FX_SRAM_SECTOR_SIZE / sizeof(uint32_t)];
/* Define FileX global data structures.  */
FX_MEDIA        sram_disk;

/* USER CODE BEGIN PV */
FX_FILE         fx_file;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* Main thread entry function.  */
void fx_app_thread_entry(ULONG thread_input);

/* USER CODE BEGIN PFP */
UINT file_write(char* file_name, char* data, int data_size);
UINT file_read(char* file_name, char* data, int data_size);
/* USER CODE END PFP */

/**
  * @brief  Application FileX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_FileX_Init(VOID *memory_ptr)
{
  UINT ret = FX_SUCCESS;

  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
  VOID *pointer;

  /* USER CODE BEGIN MX_FileX_MEM_POOL */

  /* USER CODE END MX_FileX_MEM_POOL */

  /* USER CODE BEGIN 0 */

  /* USER CODE END 0 */

  /*Allocate memory for the main thread's stack*/
  ret = tx_byte_allocate(byte_pool, &pointer, FX_APP_THREAD_STACK_SIZE, TX_NO_WAIT);

  /* Check FX_APP_THREAD_STACK_SIZE allocation*/
  if (ret != FX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the main thread.  */
  ret = tx_thread_create(&fx_app_thread, FX_APP_THREAD_NAME, fx_app_thread_entry, 0, pointer, FX_APP_THREAD_STACK_SIZE,
                         FX_APP_THREAD_PRIO, FX_APP_PREEMPTION_THRESHOLD, FX_APP_THREAD_TIME_SLICE, FX_APP_THREAD_AUTO_START);

  /* Check main thread creation */
  if (ret != FX_SUCCESS)
  {
    return TX_THREAD_ERROR;
  }
  /* USER CODE BEGIN MX_FileX_Init */

  /* USER CODE END MX_FileX_Init */

  /* Initialize FileX.  */
  fx_system_initialize();

  /* USER CODE BEGIN MX_FileX_Init 1*/

  /* USER CODE END MX_FileX_Init 1*/

  return ret;
}

 /**
 * @brief  Main thread entry.
 * @param thread_input: ULONG user argument used by the thread entry
 * @retval none
 */
void fx_app_thread_entry(ULONG thread_input)
{
  UINT sram_status = FX_SUCCESS;
  /* USER CODE BEGIN fx_app_thread_entry 0 */
	UINT status;
	CHAR read_buffer[32];
	CHAR data[] = "This is FileX working on STM32";
	char file_name[] = "STM32.TXT";

  /* USER CODE END fx_app_thread_entry 0 */

  /* Format the SRAM_BASE memory as FAT */
  sram_status =  fx_media_format(&sram_disk,                              // RamDisk pointer
                                 fx_stm32_sram_driver,                    // Driver entry
                                 (VOID *)FX_NULL,                         // Device info pointer
                                 (UCHAR *) fx_sram_media_memory,          // Media buffer pointer
                                 sizeof(fx_sram_media_memory),            // Media buffer size
                                 FX_SRAM_VOLUME_NAME,                     // Volume Name
                                 FX_SRAM_NUMBER_OF_FATS,                  // Number of FATs
                                 32,                                      // Directory Entries
                                 FX_SRAM_HIDDEN_SECTORS,                  // Hidden sectors
                                 FX_SRAM_DISK_SIZE / FX_SRAM_SECTOR_SIZE, // Total sectors
                                 FX_SRAM_SECTOR_SIZE,                     // Sector size
                                 8,                                       // Sectors per cluster
                                 1,                                       // Heads
                                 1);                                      // Sectors per track

  /* Check the format sram_status */
  if (sram_status != FX_SUCCESS)
  {
    /* USER CODE BEGIN SRAM MEDIA format error */
		while(1);
    /* USER CODE END SRAM MEDIA format error */
  }

  /* Open the sram_disk driver */
  sram_status =  fx_media_open(&sram_disk, FX_SRAM_VOLUME_NAME, fx_stm32_sram_driver, (VOID *)FX_NULL, (VOID *) fx_sram_media_memory, sizeof(fx_sram_media_memory));

  /* Check the media open sram_status */
  if (sram_status != FX_SUCCESS)
  {
    /* USER CODE BEGIN SRAM DRIVER open error */
		while(1);
    /* USER CODE END SRAM DRIVER open error */
  }

  /* USER CODE BEGIN fx_app_thread_entry 1 */
	printf("SRAM Disk successfully formatted and opened.\r\n");

	/* Create a file called STM32.TXT in the root directory.  */
	status =  fx_file_create(&sram_disk, file_name);
	if (status != FX_SUCCESS)
	{
		/* Check for an already created status. This is expected on the second pass of this loop!  */
		if (status != FX_ALREADY_CREATED)
		{
			Error_Handler();
		}
	}

	file_write(file_name, data, sizeof(data));
	file_read(file_name, read_buffer, sizeof(data));
	printf("read data: %s\r\n", read_buffer);

	while(1)
	{
		printf("fx thread\r\n");
		tx_thread_sleep(40);
	}
  /* USER CODE END fx_app_thread_entry 1 */
}

/* USER CODE BEGIN 1 */
UINT file_write(char* file_name, char* data, int data_size)
{
	UINT status;

	/* Open the test file.  */
	status =  fx_file_open(&sram_disk, &fx_file, file_name, FX_OPEN_FOR_WRITE);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	/* Seek to the beginning of the test file.  */
	status =  fx_file_seek(&fx_file, 0);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	printf("Writing data into the file. \r\n");
	/* Write a string to the test file.  */
	status =  fx_file_write(&fx_file, data, data_size);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	/* Close the test file.  */
	status =  fx_file_close(&fx_file);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	status = fx_media_flush(&sram_disk);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	return status;
}

UINT file_read(char* file_name, char* data, int data_size)
{
	UINT status;
	ULONG bytes_read;

	/* Open the test file.  */
	status =  fx_file_open(&sram_disk, &fx_file, file_name, FX_OPEN_FOR_READ);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	/* Seek to the beginning of the test file.  */
	status =  fx_file_seek(&fx_file, 0);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	/* Read the first 28 bytes of the test file.  */
	status =  fx_file_read(&fx_file, data, data_size, &bytes_read);
	if ((status != FX_SUCCESS) || (bytes_read != data_size))
	{
		return status;
	}

	/* Close the test file.  */
	status =  fx_file_close(&fx_file);
	if (status != FX_SUCCESS)
	{
		return status;
	}

	/* Close the media.  */
	status =  fx_media_close(&sram_disk);
	if (status != FX_SUCCESS)
	{
		return status;
	}
	printf("Data successfully written.\r\n");

	return status;
}
/* USER CODE END 1 */
