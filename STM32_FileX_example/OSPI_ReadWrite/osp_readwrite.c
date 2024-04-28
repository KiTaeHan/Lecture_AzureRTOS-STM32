#include <stdio.h>
#include <string.h>
#include "mx25r6435f_driver.h"

extern OSPI_HandleTypeDef hospi1;


void test_OSPI_flash(void)
{
	char data[] = "Hello Octo-SPI!!";
	int data_size = sizeof(data);
	int i;
	char buf[data_size];

	BSP_OSPI_Init(&hospi1);

	HAL_Delay(1);

	printf("Erase sector \r\n");
	BSP_OSPI_Erase_Sector(&hospi1, 0);
	// Waiting for erase complete.
	HAL_Delay(50);

	memset(buf, 0, data_size);
	BSP_OSPI_Read(&hospi1, (uint8_t*)buf, 0, data_size);
	for(i=0; i<data_size; i++)
	{
		if(0xFF != buf[i])
		{
			printf("Error: Data not erased\r\n");
		}
	}

	// write data
	BSP_OSPI_Write(&hospi1, (uint8_t*)data, 0, data_size);

	// Read data
	BSP_OSPI_Read(&hospi1, (uint8_t*)buf, 0, data_size);
	buf[data_size] = 0;

	for(i=0; i<data_size; i++)
	{
		if(buf[i] != data[i])
		{
			printf("Error: data written\r\n");
			return;
		}
	}

	printf("Test success: %s\r\n", buf);
}



/**
  * @brief  Command completed callback.
  */
void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef *hospi)
{
}

/**
  * @brief  Rx Transfer completed callback.
  */
void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi)
{
}

/**
  * @brief  Tx Transfer completed callback.
  */
 void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi)
{
}

/**
  * @brief  Transfer Error callback.
  */
void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hospi)
{
}
