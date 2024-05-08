#include "lx_stm32_ospi_driver.h"
#include "mx25r6435f_driver.h"

#define OSPI_QUAD_DISABLE       0x0
#define OSPI_QUAD_ENABLE        0x1

static uint8_t ospi_memory_reset(OSPI_HandleTypeDef *hospi);
static uint8_t ospi_set_quad_mode(OSPI_HandleTypeDef *hospi);
static uint8_t ospi_set_write_enable(OSPI_HandleTypeDef *hospi);
static uint8_t ospi_auto_polling_ready(OSPI_HandleTypeDef *hospi, uint32_t timeout);
static uint8_t ospi_highperf_mode(OSPI_HandleTypeDef *hospi);

/* USER CODE BEGIN SECTOR_BUFFER */
ULONG ospi_sector_buffer[LX_STM32_OSPI_SECTOR_SIZE / sizeof(ULONG)];
/* USER CODE END SECTOR_BUFFER */

TX_SEMAPHORE ospi_rx_semaphore;
TX_SEMAPHORE ospi_tx_semaphore;


/**
* @brief system init for octospi levelx driver
* @param UINT instance OSPI instance to initialize
* @retval 0 on success error value otherwise
*/
INT lx_stm32_ospi_lowlevel_init(UINT instance)
{
	INT status = 0;

	/* OSPI memory reset */
	if (ospi_memory_reset(&ospi_handle) != 0)
	{
		return 1;
	}

	/* Enable octal mode */
	if (ospi_set_quad_mode(&ospi_handle) != 0)
	{
		return 1;
	}

	if(ospi_highperf_mode(&ospi_handle) != 0)
	{
		return 1;
	}

	return status;
}


/**
* @brief deinit octospi levelx driver, could be called by the fx_media_close()
* @param UINT instance OSPI instance to deinitialize
* @retval 0 on success error value otherwise
*/
INT lx_stm32_ospi_lowlevel_deinit(UINT instance)
{
	INT status = 0;

	return status;
}

/**
* @brief Get the status of the OSPI instance
* @param UINT instance OSPI instance
* @retval 0 if the OSPI is ready 1 otherwise
*/
INT lx_stm32_ospi_get_status(UINT instance)
{
	uint8_t reg;
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the read security register command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = READ_SEC_REG_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
	sCommand.NbData             = 1;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(&ospi_handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(&ospi_handle, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Check the value of the register */
	if ((reg & (MX25R6435F_SECR_P_FAIL | MX25R6435F_SECR_E_FAIL)) != 0)
	{
		return OSPI_ERROR;
	}
	else if ((reg & (MX25R6435F_SECR_PSB | MX25R6435F_SECR_ESB)) != 0)
	{
		return OSPI_SUSPENDED;
	}

	/* Initialize the read status register command */
	sCommand.Instruction = READ_STATUS_REG_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(&ospi_handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(&ospi_handle, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Check the value of the register */
	if ((reg & MX25R6435F_SR_WIP) != 0)
	{
		return OSPI_BUSY;
	}
	else
	{
		return OSPI_OK;
	}
}


/**
* @brief Get size info of the flash memory
* @param UINT instance OSPI instance
* @param ULONG * block_size pointer to be filled with Flash block size
* @param ULONG * total_blocks pointer to be filled with Flash total number of blocks
* @retval 0 on Success and block_size and total_blocks are correctly filled
          1 on Failure, block_size = 0, total_blocks = 0
*/
INT lx_stm32_ospi_get_info(UINT instance, ULONG *block_size, ULONG *total_blocks)
{
	INT status = 0;

	*block_size = LX_STM32_OSPI_SECTOR_SIZE;
	*total_blocks = (LX_STM32_OSPI_FLASH_SIZE / LX_STM32_OSPI_SECTOR_SIZE);

	return status;
}

/**
* @brief Read data from the OSPI memory into a buffer
* @param UINT instance OSPI instance
* @param ULONG * address the start address to read from
* @param ULONG * buffer the destination buffer
* @param ULONG words the total number of words to be read
* @retval 0 on Success 1 on Failure
*/
INT lx_stm32_ospi_read(UINT instance, ULONG *address, ULONG *buffer, ULONG words)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the read command */
	sCommand.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId               = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction           = QUAD_INOUT_READ_CMD;
	sCommand.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.Address               = (uint32_t)address;
	sCommand.AddressMode           = HAL_OSPI_ADDRESS_4_LINES;
	sCommand.AddressSize           = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytes        = MX25R6435F_ALT_BYTES_NO_PE_MODE;
	sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
	sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
	sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
	sCommand.DataMode              = HAL_OSPI_DATA_4_LINES;
	sCommand.NbData                = words * sizeof(ULONG);
	sCommand.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles           = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
	sCommand.DQSMode               = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(&ospi_handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}


	/* Reception of the data */
	if (HAL_OSPI_Receive_IT(&ospi_handle, (uint8_t*)buffer) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Check success of the transmission of the data */
	if(tx_semaphore_get(&ospi_rx_semaphore, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != TX_SUCCESS)
	{
		return OSPI_ERROR;
	}

	/* Release ospi_transfer_semaphore in case of writing success */
	tx_semaphore_put(&ospi_rx_semaphore);

	return OSPI_OK;
}

/**
* @brief write a data buffer into the OSPI memory
* @param UINT instance OSPI instance
* @param ULONG * address the start address to write into
* @param ULONG * buffer the data source buffer
* @param ULONG words the total number of words to be written
* @retval 0 on Success 1 on Failure
*/
INT lx_stm32_ospi_write(UINT instance, ULONG *address, ULONG *buffer, ULONG words)
{
	uint32_t end_addr, current_size, current_addr, data_buffer;
	OSPI_RegularCmdTypeDef sCommand;

	/* Calculation of the size between the write address and the end of the page */
	current_size = LX_STM32_OSPI_PAGE_SIZE - ((uint32_t)address % LX_STM32_OSPI_PAGE_SIZE);

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > (((uint32_t) words) * sizeof(ULONG)))
	{
		current_size = ((uint32_t) words) * sizeof(ULONG);
	}

	/* Initialize the adress variables */
	current_addr = (uint32_t) address;
	end_addr = ((uint32_t) address) + ((uint32_t) words) * sizeof(ULONG);
	data_buffer= (uint32_t)buffer;

	/* Initialize the program command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_4_LINES;
	sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Perform the write page by page */
	do {
		sCommand.Address = current_addr;
		sCommand.NbData  = current_size;

		/* Enable write operations */
		if (ospi_set_write_enable(&ospi_handle) != OSPI_OK)
		{
			return OSPI_ERROR;
		}

		/* Configure the command */
		if (HAL_OSPI_Command(&ospi_handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			return OSPI_ERROR;
		}

		/* Transmission of the data */
		if (HAL_OSPI_Transmit_IT(&ospi_handle, (uint8_t*)data_buffer)!= HAL_OK)
		{
			return OSPI_ERROR;
		}

	    /* Check success of the transmission of the data */
	    if(tx_semaphore_get(&ospi_tx_semaphore, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != TX_SUCCESS)
	    {
			return OSPI_ERROR;
	    }

		/* Configure automatic polling mode to wait for end of program */
		if (ospi_auto_polling_ready(&ospi_handle, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
		{
			return OSPI_ERROR;
		}

		/* Update the address and size variables for next page programming */
		current_addr += current_size;
		data_buffer += current_size;
		current_size = ((current_addr + LX_STM32_OSPI_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : LX_STM32_OSPI_PAGE_SIZE;
	} while (current_addr < end_addr);

	/* Release ospi_transfer_semaphore in case of writing success */
	tx_semaphore_put(&ospi_tx_semaphore);

	return OSPI_OK;
}

/**
* @brief Erase the whole flash or a single block
* @param UINT instance OSPI instance
* @param ULONG  block the block to be erased
* @param ULONG  erase_count the number of times the block was erased
* @param UINT full_chip_erase if set to 0 a single block is erased otherwise the whole flash is erased
* @retval 0 on Success 1 on Failure
*/
INT lx_stm32_ospi_erase(UINT instance, ULONG block, ULONG erase_count, UINT full_chip_erase)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the erase command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* DTR mode is enabled */
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;

	if(full_chip_erase)
	{
		sCommand.Instruction        = CHIP_ERASE_CMD;
		sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	}
	else
	{
		sCommand.Instruction        = SECTOR_ERASE_CMD;
		sCommand.Address            = (block * MX25R6435F_SECTOR_SIZE);
		sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
		sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
		sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;	/* DTR mode is enabled */
	}

	/* Enable write operations */
	if (ospi_set_write_enable(&ospi_handle) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the command */
	if (HAL_OSPI_Command(&ospi_handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for end of erase */
	if (ospi_auto_polling_ready(&ospi_handle, LX_STM32_OSPI_BULK_ERASE_MAX_TIME) != 0)
	{
		return 1;
	}

	return OSPI_OK;
}

/**
* @brief Check that a block was actually erased
* @param UINT instance OSPI instance
* @param ULONG  block the block to be checked
* @retval 0 on Success 1 on Failure
*/
INT lx_stm32_ospi_is_block_erased(UINT instance, ULONG block)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the erase command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = BLOCK_ERASE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.Address            = block;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
	sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (ospi_set_write_enable(&ospi_handle) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the command */
	if (HAL_OSPI_Command(&ospi_handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for end of erase */
	if (ospi_auto_polling_ready(&ospi_handle, MX25R6435F_BLOCK_ERASE_MAX_TIME) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
* @brief Handle levelx system errors
* @param UINT error_code Code of the concerned error.
* @retval UINT error code.
*/

UINT  lx_ospi_driver_system_error(UINT error_code)
{
	UINT status = LX_ERROR;

	return status;
}

/**
  * @brief  Reset the OSPI memory.
  * @param  hospi: OSPI handle pointer
  * @retval O on success 1 on Failure.
  */
static uint8_t ospi_memory_reset(OSPI_HandleTypeDef *hospi)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the reset enable command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = RESET_ENABLE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;


	/* Send the command */
	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the reset memory command */
	sCommand.Instruction = RESET_MEMORY_CMD;
	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait the memory is ready */
	if (ospi_auto_polling_ready(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  Send a Write Enable command and wait its effective.
  * @param  hospi: OSPI handle pointer
  * @retval O on success 1 on Failure.
  */
static uint8_t ospi_set_write_enable(OSPI_HandleTypeDef *hospi)
{
	OSPI_RegularCmdTypeDef sCommand;
	OSPI_AutoPollingTypeDef sConfig;

	/* Enable write operations */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = WRITE_ENABLE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for write enabling */
	sConfig.Match         = MX25R6435F_SR_WEL;
	sConfig.Mask          = MX25R6435F_SR_WEL;
	sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
	sConfig.Interval      = 0x10;
	sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

	sCommand.Instruction  = READ_STATUS_REG_CMD;
	sCommand.DataMode     = HAL_OSPI_DATA_1_LINE;
	sCommand.NbData       = 1;
	sCommand.DataDtrMode  = HAL_OSPI_DATA_DTR_DISABLE;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_AutoPolling(hospi, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  Read the SR of the memory and wait the EOP.
  * @param  hospi: OSPI handle pointer
  * @param  timeout: timeout value before returning an error
  * @retval O on success 1 on Failure.
  */
static uint8_t ospi_auto_polling_ready(OSPI_HandleTypeDef *hospi, uint32_t timeout)
{
	OSPI_RegularCmdTypeDef sCommand;
	OSPI_AutoPollingTypeDef sConfig;

	/* Configure automatic polling mode to wait for memory ready */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = READ_STATUS_REG_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
	sCommand.NbData             = 1;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	sConfig.Match         = 0;
	sConfig.Mask          = MX25R6435F_SR_WIP;
	sConfig.MatchMode     = HAL_OSPI_MATCH_MODE_AND;
	sConfig.Interval      = 0x10;
	sConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_AutoPolling(hospi, &sConfig, timeout) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  This function enables the octal mode of the memory.
  * @param  hospi: OSPI handle
  * @retval 0 on success 1 on Failure.
  */
static uint8_t ospi_set_quad_mode(OSPI_HandleTypeDef *hospi)
{
	uint8_t reg;
	OSPI_RegularCmdTypeDef sCommand;

	/* Read status register */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = READ_STATUS_REG_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = 0;
	sCommand.NbData             = 1;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hospi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Enable write operations */
	if (ospi_set_write_enable(hospi) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	SET_BIT(reg, MX25R6435F_SR_QE);

	sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Transmit(hospi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Wait that memory is ready */
	if (ospi_auto_polling_ready(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Check the configuration has been correctly done */
	sCommand.Instruction = READ_STATUS_REG_CMD;
	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hospi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

static uint8_t ospi_highperf_mode(OSPI_HandleTypeDef *hospi)
{
	uint8_t reg[3];
	OSPI_RegularCmdTypeDef sCommand;

	/* Read status register */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = READ_STATUS_REG_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;
	sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles        = 0;
	sCommand.NbData             = 1;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hospi, &(reg[0]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Read configuration registers */
	sCommand.Instruction = READ_CFG_REG_CMD;
	sCommand.NbData      = 2;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hospi, &(reg[1]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Enable write operations */
	if (ospi_set_write_enable(hospi) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	SET_BIT(reg[2], MX25R6435F_CR2_LH_SWITCH);

	sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
	sCommand.NbData      = 3;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Transmit(hospi, &(reg[0]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Wait that memory is ready */
	if (ospi_auto_polling_ready(hospi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Check the configuration has been correctly done */
	sCommand.Instruction = READ_CFG_REG_CMD;
	sCommand.NbData      = 2;

	if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hospi, &(reg[0]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}


/**
  * @brief  Rx Transfer completed callbacks.
  * @param  hqspi OSPI handle
  * @retval None
  */
void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi)
{
	tx_semaphore_put(&ospi_rx_semaphore);
}

/**
  * @brief  Tx Transfer completed callbacks.
  * @param  hqspi OSPI handle
  * @retval None
  */
void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi)
{
	tx_semaphore_put(&ospi_tx_semaphore);
}
