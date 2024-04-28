#ifndef MX25R6435F_DRIVER_H_
#define MX25R6435F_DRIVER_H_

#include "main.h"
#include "mx25r6425f.h"

/* QSPI Error codes */
#define OSPI_OK            ((uint8_t)0x00)
#define OSPI_ERROR         ((uint8_t)0x01)
#define OSPI_BUSY          ((uint8_t)0x02)
#define OSPI_NOT_SUPPORTED ((uint8_t)0x04)
#define OSPI_SUSPENDED     ((uint8_t)0x08)

/* OSPI Info */
typedef struct
{
  uint32_t FlashSize;          /*!< Size of the flash */
  uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
  uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
  uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
  uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
} OSPI_Info;


uint8_t BSP_OSPI_Init(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_DeInit(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_Read(OSPI_HandleTypeDef* handle, uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
uint8_t BSP_OSPI_Write(OSPI_HandleTypeDef* handle, uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
uint8_t BSP_OSPI_Erase_Block(OSPI_HandleTypeDef* handle, uint32_t BlockAddress);
uint8_t BSP_OSPI_Erase_Sector(OSPI_HandleTypeDef* handle, uint32_t Sector);
uint8_t BSP_OSPI_Erase_Chip(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_GetStatus(OSPI_HandleTypeDef* handle);

uint8_t BSP_OSPI_GetInfo(OSPI_Info *pInfo);
uint8_t BSP_OSPI_EnableMemoryMappedMode(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_SuspendErase(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_ResumeErase(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_EnterDeepPowerDown(OSPI_HandleTypeDef* handle);
uint8_t BSP_OSPI_LeaveDeepPowerDown(OSPI_HandleTypeDef* handle);

#endif /* MX25R6435F_DRIVER_H_ */
