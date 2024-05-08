
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
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* Main thread stack size */
#define FX_APP_THREAD_STACK_SIZE         (1024*4)
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
uint32_t fx_nor_ospi_media_memory[FX_NOR_OSPI_SECTOR_SIZE / sizeof(uint32_t)];
/* Define FileX global data structures.  */
FX_MEDIA        nor_ospi_flash_disk;

/* USER CODE BEGIN PV */
FX_FILE         fx_file;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* Main thread entry function.  */
void fx_app_thread_entry(ULONG thread_input);

/* USER CODE BEGIN PFP */
UINT Create_FxFile(CHAR* file_name, VOID* buffer_ptr, ULONG size);
UINT Read_FxFile(CHAR* file_name, VOID* buffer_ptr, ULONG size);
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
  UINT nor_ospi_status = FX_SUCCESS;
  /* USER CODE BEGIN fx_app_thread_entry 0 */
  ULONG available_space_pre;
  ULONG available_space_post;
  CHAR read_buffer[32];
  CHAR data[] = "This is FileX working on STM32";

  printf("FileX/LevelX NOR OCTO-SPI Application Start.\r\n");
  printf("Total NOR Flash Chip size is: %lu bytes.\r\n", (unsigned long)MX25R6435F_FLASH_SIZE);

  /* USER CODE END fx_app_thread_entry 0 */

  /* Format the OCTO-SPI NOR flash as FAT */
  nor_ospi_status =  fx_media_format(&nor_ospi_flash_disk,                               // nor_ospi_flash_disk pointer
                                     fx_stm32_levelx_nor_driver,                         // Driver entry
                                     (VOID *)LX_NOR_OSPI_DRIVER_ID,                      // Device info pointer
                                     (UCHAR *) fx_nor_ospi_media_memory,                 // Media buffer pointer
                                     sizeof(fx_nor_ospi_media_memory),                   // Media buffer size
                                     FX_NOR_OSPI_VOLUME_NAME,                            // Volume Name
                                     FX_NOR_OSPI_NUMBER_OF_FATS,                         // Number of FATs
                                     32,                                                 // Directory Entries
                                     FX_NOR_OSPI_HIDDEN_SECTORS,                         // Hidden sectors
                                     LX_STM32_OSPI_FLASH_SIZE / FX_NOR_OSPI_SECTOR_SIZE, // Total sectors
                                     FX_NOR_OSPI_SECTOR_SIZE,                            // Sector size
                                     8,                                                  // Sectors per cluster
                                     1,                                                  // Heads
                                     1);                                                 // Sectors per track

  /* Check the format nor_ospi_status */
  if (nor_ospi_status != FX_SUCCESS)
  {
    /* USER CODE BEGIN OCTO-SPI NOR format error */
    while(1);
    /* USER CODE END OCTO-SPI NOR format error */
  }

  /* Open the OCTO-SPI NOR driver */
  nor_ospi_status =  fx_media_open(&nor_ospi_flash_disk, FX_NOR_OSPI_VOLUME_NAME, fx_stm32_levelx_nor_driver, (VOID *)LX_NOR_OSPI_DRIVER_ID, (VOID *) fx_nor_ospi_media_memory, sizeof(fx_nor_ospi_media_memory));

  /* Check the media open nor_ospi_status */
  if (nor_ospi_status != FX_SUCCESS)
  {
    /* USER CODE BEGIN OCTO-SPI NOR format error */
    while(1);
    /* USER CODE END OCTO-SPI NOR format error */
  }

  /* USER CODE BEGIN fx_app_thread_entry 1 */

  /* Get the available usable space */
  nor_ospi_status =  fx_media_space_available(&nor_ospi_flash_disk, &available_space_pre);
  if (nor_ospi_status != FX_SUCCESS)
  {
	  Error_Handler();
  }
  printf("User available NOR Flash disk space size before file is written: %lu bytes.\r\n", available_space_pre);

  nor_ospi_status = Create_FxFile("STM32.TXT", data, sizeof(data));
  if (nor_ospi_status != FX_SUCCESS)
  {
	  Error_Handler();
  }

  nor_ospi_status = Read_FxFile("STM32.TXT", read_buffer, sizeof(read_buffer));
  if (nor_ospi_status != FX_SUCCESS)
  {
	  Error_Handler();
  }
  printf("Read File data: %s\r\n", read_buffer);

  /* Get the available usable space, after the file has been created */
  nor_ospi_status =  fx_media_space_available(&nor_ospi_flash_disk, &available_space_post);
  if (nor_ospi_status != FX_SUCCESS)
  {
	  Error_Handler();
  }

  printf("User available NOR Flash disk space size after file is written: %lu bytes.\r\n", available_space_post);
  printf("The test file occupied a total of %lu cluster(s) (%u per cluster).\r\n",
         (available_space_pre - available_space_post) / (nor_ospi_flash_disk.fx_media_bytes_per_sector * nor_ospi_flash_disk.fx_media_sectors_per_cluster),
         nor_ospi_flash_disk.fx_media_bytes_per_sector * nor_ospi_flash_disk.fx_media_sectors_per_cluster);


  /* Close the media.  */
  nor_ospi_status =  fx_media_close(&nor_ospi_flash_disk);
  if (nor_ospi_status != FX_SUCCESS)
  {
	  Error_Handler();
  }

  while(1)
  {
	  HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
      tx_thread_sleep(400);
  }

  /* USER CODE END fx_app_thread_entry 1 */
}

/* USER CODE BEGIN 1 */
UINT Create_FxFile(CHAR* file_name, VOID* buffer_ptr, ULONG size)
{
	UINT nor_ospi_status = FX_SUCCESS;

	/* Create a file called STM32.TXT in the root directory.  */
	nor_ospi_status =  fx_file_create(&nor_ospi_flash_disk, file_name);
	if (nor_ospi_status != FX_SUCCESS)
	{
		/* Check for an already created status. This is expected on the second pass of this loop!  */
		if (nor_ospi_status != FX_ALREADY_CREATED)
		{
			goto CREATE_FILE_END;
		}
	}

	/* Open the test file.  */
	nor_ospi_status =  fx_file_open(&nor_ospi_flash_disk, &fx_file, file_name, FX_OPEN_FOR_WRITE);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto CREATE_FILE_END;
	}

	/* Seek to the beginning of the test file.  */
	nor_ospi_status =  fx_file_seek(&fx_file, 0);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto CREATE_FILE_END;
	}

	/* Write a string to the test file.  */
	nor_ospi_status =  fx_file_write(&fx_file, buffer_ptr, size);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto CREATE_FILE_END;
	}

	/* Close the test file.  */
	nor_ospi_status =  fx_file_close(&fx_file);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto CREATE_FILE_END;
	}

	nor_ospi_status = fx_media_flush(&nor_ospi_flash_disk);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto CREATE_FILE_END;
	}

CREATE_FILE_END:
	return nor_ospi_status;
}

UINT Read_FxFile(CHAR* file_name, VOID* buffer_ptr, ULONG size)
{
	UINT nor_ospi_status = FX_SUCCESS;
	ULONG bytes_read;

	/* Open the test file.  */
	nor_ospi_status =  fx_file_open(&nor_ospi_flash_disk, &fx_file, file_name, FX_OPEN_FOR_READ);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto READ_FILE_END;
	}

	/* Seek to the beginning of the test file.  */
	nor_ospi_status =  fx_file_seek(&fx_file, 0);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto READ_FILE_END;
	}

	/* Read the first 28 bytes of the test file.  */
	nor_ospi_status =  fx_file_read(&fx_file, buffer_ptr, size, &bytes_read);
	if ((nor_ospi_status != FX_SUCCESS) || (bytes_read != size))
	{
		goto READ_FILE_END;
	}

	/* Close the test file.  */
	nor_ospi_status =  fx_file_close(&fx_file);
	if (nor_ospi_status != FX_SUCCESS)
	{
		goto READ_FILE_END;
	}

READ_FILE_END:
	return nor_ospi_status;
}

/* USER CODE END 1 */
