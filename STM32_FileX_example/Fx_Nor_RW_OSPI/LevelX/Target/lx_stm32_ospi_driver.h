/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/

#ifndef LX_STM32_OSPI_DRIVER_H
#define LX_STM32_OSPI_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lx_api.h"
#include "stm32l4xx_hal.h"

/* USER CODE BEGIN Includes */
#include "mx25r6425f.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

extern OSPI_HandleTypeDef hospi1;

/* The following semaphore is being to notify about RX/TX completion. It needs to be released in the transfer callbacks */
extern TX_SEMAPHORE ospi_rx_semaphore;
extern TX_SEMAPHORE ospi_tx_semaphore;

#define ospi_handle        hospi1

/* Exported constants --------------------------------------------------------*/

/* the OctoSPI instance, default value set to 0 */
#define LX_STM32_OSPI_INSTANCE                           0

#define LX_STM32_OSPI_DEFAULT_TIMEOUT                    10 * TX_TIMER_TICKS_PER_SECOND

#define LX_STM32_DEFAULT_SECTOR_SIZE                     LX_STM32_OSPI_SECTOR_SIZE
#define LX_STM32_OSPI_DMA_API                            1

/* when set to 1 LevelX is initializing the OctoSPI memory,
 * otherwise it is the up to the application to perform it.
 */
#define LX_STM32_OSPI_INIT                               0

/* allow the driver to fully erase the OctoSPI chip. This should be used carefully.
 * the call is blocking and takes a while. by default it is set to 0.
 */
#define LX_STM32_OSPI_ERASE                              0

/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */
#define LX_STM32_OSPI_CURRENT_TIME                              tx_time_get

/* Macro called after initializing the OSPI driver
 * e.g. create a semaphore used for transfer notification */
 /* USER CODE BEGIN LX_STM32_OSPI_POST_INIT */

#define LX_STM32_OSPI_POST_INIT()                        do { \
                                                         if (tx_semaphore_create(&ospi_rx_semaphore, "ospi rx transfer semaphore", 0) != TX_SUCCESS) \
                                                         { \
                                                           return LX_ERROR; \
                                                         } \
                                                         if (tx_semaphore_create(&ospi_tx_semaphore, "ospi tx transfer semaphore", 0) != TX_SUCCESS) \
                                                         { \
                                                           return LX_ERROR; \
                                                         } \
                                                        } while(0)
/* USER CODE END LX_STM32_OSPI_POST_INIT */

/* Macro called before performing read operation */

/* USER CODE BEGIN LX_STM32_OSPI_PRE_READ_TRANSFER */

#define LX_STM32_OSPI_PRE_READ_TRANSFER(__status__)

/* USER CODE END LX_STM32_OSPI_PRE_READ_TRANSFER */

/* Define how to notify about Read completion operation */

/* USER CODE BEGIN LX_STM32_OSPI_READ_CPLT_NOTIFY */

#define LX_STM32_OSPI_READ_CPLT_NOTIFY(__status__)      do { \
                                                          if(tx_semaphore_get(&ospi_rx_semaphore, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != TX_SUCCESS) \
                                                          { \
                                                            __status__ = LX_ERROR; \
                                                          } \
                                                        } while(0)

/* USER CODE END LX_STM32_OSPI_READ_CPLT_NOTIFY */

/* Macro called after performing read operation */

/* USER CODE BEGIN LX_STM32_OSPI_POST_READ_TRANSFER */

#define LX_STM32_OSPI_POST_READ_TRANSFER(__status__)

/* USER CODE END LX_STM32_OSPI_POST_READ_TRANSFER */

/* Macro for read error handling */
/* USER CODE BEGIN LX_STM32_OSPI_READ_TRANSFER_ERROR */

#define LX_STM32_OSPI_READ_TRANSFER_ERROR(__status__)

/* USER CODE END LX_STM32_OSPI_READ_TRANSFER_ERROR */

/* Macro called before performing write operation */

/* USER CODE BEGIN LX_STM32_OSPI_PRE_WRITE_TRANSFER */

#define LX_STM32_OSPI_PRE_WRITE_TRANSFER(__status__)

/* USER CODE END LX_STM32_OSPI_PRE_WRITE_TRANSFER */

/* Define how to notify about write completion operation */

/* USER CODE BEGIN LX_STM32_OSPI_WRITE_CPLT_NOTIFY */

#define LX_STM32_OSPI_WRITE_CPLT_NOTIFY(__status__)     do { \
                                                          if(tx_semaphore_get(&ospi_tx_semaphore, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != TX_SUCCESS) \
                                                          { \
                                                            __status__ = LX_ERROR; \
                                                          } \
                                                        } while(0)

/* USER CODE END LX_STM32_OSPI_WRITE_CPLT_NOTIFY */

/* Macro called after performing write operation */

/* USER CODE BEGIN LX_STM32_OSPI_POST_WRITE_TRANSFER */

#define LX_STM32_OSPI_POST_WRITE_TRANSFER(__status__)

/* USER CODE END LX_STM32_OSPI_POST_WRITE_TRANSFER */

/* Macro for write error handling */

/* USER CODE BEGIN LX_STM32_OSPI_WRITE_TRANSFER_ERROR */

#define LX_STM32_OSPI_WRITE_TRANSFER_ERROR(__status__)

/* USER CODE END LX_STM32_OSPI_WRITE_TRANSFER_ERROR */

/* Exported functions prototypes ---------------------------------------------*/
INT lx_stm32_ospi_lowlevel_init(UINT instance);
INT lx_stm32_ospi_lowlevel_deinit(UINT instance);

INT lx_stm32_ospi_get_status(UINT instance);
INT lx_stm32_ospi_get_info(UINT instance, ULONG *block_size, ULONG *total_blocks);

INT lx_stm32_ospi_read(UINT instance, ULONG *address, ULONG *buffer, ULONG words);
INT lx_stm32_ospi_write(UINT instance, ULONG *address, ULONG *buffer, ULONG words);

INT lx_stm32_ospi_erase(UINT instance, ULONG block, ULONG erase_count, UINT full_chip_erase);
INT lx_stm32_ospi_is_block_erased(UINT instance, ULONG block);

UINT lx_ospi_driver_system_error(UINT error_code);

UINT lx_stm32_ospi_initialize(LX_NOR_FLASH *nor_flash);

#if (LX_STM32_OSPI_INIT == 1)
extern void MX_OCTOSPI1_Init(void);
#endif

#if (LX_STM32_OSPI_INIT == 1)
#define ospi_driver_init() MX_OCTOSPI1_Init()
#endif

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* USER CODE BEGIN CUSTOM_OSPI */
/* Define the command codes and flags below related to the NOR Flash memory used */

#define LX_STM32_OSPI_DUMMY_CYCLES_READ_OCTAL		0

//#define LX_STM32_OSPI_SECTOR_SIZE					MX25R6435F_SECTOR_SIZE
#define LX_STM32_OSPI_SECTOR_SIZE					MX25R6435F_BLOCK_SIZE
#define LX_STM32_OSPI_FLASH_SIZE					MX25R6435F_FLASH_SIZE
#define LX_STM32_OSPI_PAGE_SIZE						MX25R6435F_PAGE_SIZE

#define LX_STM32_OSPI_BULK_ERASE_MAX_TIME			CHIP_ERASE_CMD
//#define LX_STM32_OSPI_BULK_ERASE_MAX_TIME			MX25R6435F_SECTOR_ERASE_MAX_TIME

#define LX_STM32_OSPI_OCTAL_BULK_ERASE_CMD			CHIP_ERASE_CMD
#define LX_STM32_OSPI_OCTAL_SECTOR_ERASE_CMD		SECTOR_ERASE_CMD

#define LX_STM32_OSPI_WRITE_ENABLE_CMD				WRITE_ENABLE_CMD

#define LX_STM32_OSPI_OCTAL_READ_STATUS_REG_CMD		READ_STATUS_REG_CMD

#define LX_STM32_OSPI_RESET_ENABLE_CMD				RESET_ENABLE_CMD
#define LX_STM32_OSPI_RESET_MEMORY_CMD				RESET_MEMORY_CMD


#define LX_STM32_OSPI_CR2_REG3_ADDR               0xFF
#define LX_STM32_OSPI_CR2_REG1_ADDR               0xFF
#define LX_STM32_OSPI_SR_WEL                      0xFF
#define LX_STM32_OSPI_SR_WIP                      0xFF
#define LX_STM32_OSPI_CR2_SOPI                    0xFF
#define LX_STM32_OSPI_CR2_DOPI                    0xFF

/* USER CODE END CUSTOM_OSPI */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /* LX_STM32_OSPI_DRIVER_H */
