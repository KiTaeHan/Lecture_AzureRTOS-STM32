#include "mx25r6435f_driver.h"

#define OSPI_QUAD_DISABLE       0x0
#define OSPI_QUAD_ENABLE        0x1
#define OSPI_HIGH_PERF_DISABLE  0x0
#define OSPI_HIGH_PERF_ENABLE   0x1

static uint8_t OSPI_WriteEnable(OSPI_HandleTypeDef* hxspi);
static uint8_t OSPI_AutoPollingMemReady(OSPI_HandleTypeDef* hxspi, uint32_t Timeout);
static uint8_t OSPI_QuadMode(OSPI_HandleTypeDef* hxspi, uint8_t Operation);
static uint8_t OSPI_HighPerfMode(OSPI_HandleTypeDef* hxspi, uint8_t Operation);
static uint8_t OSPI_ResetMemory(OSPI_HandleTypeDef* hxspi);


static uint8_t OSPI_WriteEnable(OSPI_HandleTypeDef* hxspi)
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

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_AutoPolling(hxspi, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  This function read the SR of the memory and wait the EOP.
  * @param  hxspi   : QSPI handle
  * @param  Timeout : Timeout for auto-polling
  * @retval None
  */
static uint8_t OSPI_AutoPollingMemReady(OSPI_HandleTypeDef* hxspi, uint32_t Timeout)
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

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_AutoPolling(hxspi, &sConfig, Timeout) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  This function enables/disables the Quad mode of the memory.
  * @param  hxspi     : OSPI handle
  * @param  Operation : OSPI_QUAD_ENABLE or OSPI_QUAD_DISABLE mode
  * @retval None
  */
static uint8_t OSPI_QuadMode(OSPI_HandleTypeDef* hxspi, uint8_t Operation)
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

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hxspi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Enable write operations */
	if (OSPI_WriteEnable(hxspi) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Activate/deactivate the Quad mode */
	if (Operation == OSPI_QUAD_ENABLE)
	{
		SET_BIT(reg, MX25R6435F_SR_QE);
	}
	else
	{
		CLEAR_BIT(reg, MX25R6435F_SR_QE);
	}

	sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Transmit(hxspi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Wait that memory is ready */
	if (OSPI_AutoPollingMemReady(hxspi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Check the configuration has been correctly done */
	sCommand.Instruction = READ_STATUS_REG_CMD;
	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hxspi, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if ((((reg & MX25R6435F_SR_QE) == 0) && (Operation == OSPI_QUAD_ENABLE)) ||
	  (((reg & MX25R6435F_SR_QE) != 0) && (Operation == OSPI_QUAD_DISABLE)))
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  This function enables/disables the high performance mode of the memory.
  * @param  hxspi     : QSPI handle
  * @param  Operation : QSPI_HIGH_PERF_ENABLE or QSPI_HIGH_PERF_DISABLE high performance mode
  * @retval None
  */
static uint8_t OSPI_HighPerfMode(OSPI_HandleTypeDef* hxspi, uint8_t Operation)
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

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hxspi, &(reg[0]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Read configuration registers */
	sCommand.Instruction = READ_CFG_REG_CMD;
	sCommand.NbData      = 2;

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hxspi, &(reg[1]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Enable write operations */
	if (OSPI_WriteEnable(hxspi) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Activate/deactivate the Quad mode */
	if (Operation == OSPI_HIGH_PERF_ENABLE)
	{
		SET_BIT(reg[2], MX25R6435F_CR2_LH_SWITCH);
	}
	else
	{
		CLEAR_BIT(reg[2], MX25R6435F_CR2_LH_SWITCH);
	}

	sCommand.Instruction = WRITE_STATUS_CFG_REG_CMD;
	sCommand.NbData      = 3;

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Transmit(hxspi, &(reg[0]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Wait that memory is ready */
	if (OSPI_AutoPollingMemReady(hxspi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Check the configuration has been correctly done */
	sCommand.Instruction = READ_CFG_REG_CMD;
	sCommand.NbData      = 2;

	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if (HAL_OSPI_Receive(hxspi, &(reg[0]), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	if ((((reg[1] & MX25R6435F_CR2_LH_SWITCH) == 0) && (Operation == OSPI_HIGH_PERF_ENABLE)) ||
	  (((reg[1] & MX25R6435F_CR2_LH_SWITCH) != 0) && (Operation == OSPI_HIGH_PERF_DISABLE)))
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}


/**
  * @brief  This function reset the OSPI memory.
  * @param  hxspi : OSPI handle
  * @retval None
  */
static uint8_t OSPI_ResetMemory(OSPI_HandleTypeDef* hxspi)
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
	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the reset memory command */
	sCommand.Instruction = RESET_MEMORY_CMD;
	if (HAL_OSPI_Command(hxspi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait the memory is ready */
	if (OSPI_AutoPollingMemReady(hxspi, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}


//----------------------------------------------------------------------------------------------------------------------------------------//

/**
  * @brief  Initializes the OSPI interface.
  * @param  handle : pointer to OSPI_t structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_Init(OSPI_HandleTypeDef* handle)
{
	/* OSPI memory reset */
	if(OSPI_ResetMemory(handle) != OSPI_OK)
	{
		return OSPI_NOT_SUPPORTED;
	}

	/* QSPI quad enable */
	if (OSPI_QuadMode(handle, OSPI_QUAD_ENABLE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* High performance mode enable */
	if (OSPI_HighPerfMode(handle, OSPI_HIGH_PERF_ENABLE) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}


/**
  * @brief  De-Initializes the OSPI interface.
  * @param  handle : pointer to OSPI_t structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_DeInit(OSPI_HandleTypeDef* handle)
{
	if(HAL_OSPI_DeInit(handle) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  Reads an amount of data from the OSPI memory.
  * @param  handle : pointer to OSPI_t structure
  * @param  pData    : Pointer to data to be read
  * @param  ReadAddr : Read start address
  * @param  Size     : Size of data to read
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_Read(OSPI_HandleTypeDef* handle, uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the read command */
	sCommand.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId               = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction           = QUAD_INOUT_READ_CMD;
	sCommand.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.Address               = ReadAddr;
	sCommand.AddressMode           = HAL_OSPI_ADDRESS_4_LINES;
	sCommand.AddressSize           = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytes        = MX25R6435F_ALT_BYTES_NO_PE_MODE;
	sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
	sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
	sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
	sCommand.DataMode              = HAL_OSPI_DATA_4_LINES;
	sCommand.NbData                = Size;
	sCommand.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles           = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
	sCommand.DQSMode               = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(handle, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}


/**
  * @brief  Writes an amount of data to the OSPI memory.
  * @param  handle : pointer to OSPI_t structure
  * @param  pData     : Pointer to data to be written
  * @param  WriteAddr : Write start address
  * @param  Size      : Size of data to write
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_Write(OSPI_HandleTypeDef* handle, uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
	uint32_t end_addr, current_size, current_addr;
	OSPI_RegularCmdTypeDef sCommand;

	/* Calculation of the size between the write address and the end of the page */
	current_size = MX25R6435F_PAGE_SIZE - (WriteAddr % MX25R6435F_PAGE_SIZE);

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > Size)
	{
	current_size = Size;
	}

	/* Initialize the adress variables */
	current_addr = WriteAddr;
	end_addr = WriteAddr + Size;

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
		if (OSPI_WriteEnable(handle) != OSPI_OK)
		{
			return OSPI_ERROR;
		}

		/* Configure the command */
		if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			return OSPI_ERROR;
		}

		/* Transmission of the data */
		if (HAL_OSPI_Transmit(handle, pData, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			return OSPI_ERROR;
		}

		/* Configure automatic polling mode to wait for end of program */
		if (OSPI_AutoPollingMemReady(handle, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != OSPI_OK)
		{
			return OSPI_ERROR;
		}

		/* Update the address and size variables for next page programming */
		current_addr += current_size;
		pData += current_size;
		current_size = ((current_addr + MX25R6435F_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : MX25R6435F_PAGE_SIZE;
	} while (current_addr < end_addr);

	return OSPI_OK;
}

/**
  * @brief  Erases the specified block of the OSPI memory.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @param  BlockAddress : Block address to erase
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_Erase_Block(OSPI_HandleTypeDef* handle, uint32_t BlockAddress)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the erase command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = BLOCK_ERASE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.Address            = BlockAddress;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
	sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (OSPI_WriteEnable(handle) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the command */
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for end of erase */
	if (OSPI_AutoPollingMemReady(handle, MX25R6435F_BLOCK_ERASE_MAX_TIME) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  Erases the specified sector of the OSPI memory.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @param  Sector : Sector address to erase (0 to 255)
  * @retval OSPI memory status
  * @note This function is non blocking meaning that sector erase
  *       operation is started but not completed when the function
  *       returns. Application has to call BSP_OSPI_GetStatus()
  *       to know when the device is available again (i.e. erase operation
  *       completed).
  */
uint8_t BSP_OSPI_Erase_Sector(OSPI_HandleTypeDef* handle, uint32_t Sector)
{
	OSPI_RegularCmdTypeDef sCommand;

	if (Sector >= (uint32_t)(MX25R6435F_FLASH_SIZE / MX25R6435F_SECTOR_SIZE))
	{
		return OSPI_ERROR;
	}

	/* Initialize the erase command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = SECTOR_ERASE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.Address            = (Sector * MX25R6435F_SECTOR_SIZE);
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_1_LINE;
	sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (OSPI_WriteEnable(handle) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the command */
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  Erases the entire OSPI memory.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_Erase_Chip(OSPI_HandleTypeDef* handle)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the erase command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = CHIP_ERASE_CMD;
	sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode           = HAL_OSPI_DATA_NONE;
	sCommand.DummyCycles        = 0;
	sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Enable write operations */
	if (OSPI_WriteEnable(handle) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	/* Send the command */
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Configure automatic polling mode to wait for end of erase */
	if (OSPI_AutoPollingMemReady(handle, MX25R6435F_CHIP_ERASE_MAX_TIME) != OSPI_OK)
	{
		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  Reads current status of the OSPI memory.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_GetStatus(OSPI_HandleTypeDef* handle)
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
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(handle, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(handle, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
  * @brief  Return the configuration of the QSPI memory.
  * @param  pInfo : pointer on the configuration structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_GetInfo(OSPI_Info *pInfo)
{
	/* Configure the structure with the memory configuration */
	pInfo->FlashSize          = MX25R6435F_FLASH_SIZE;
	pInfo->EraseSectorSize    = MX25R6435F_SECTOR_SIZE;
	pInfo->EraseSectorsNumber = (MX25R6435F_FLASH_SIZE / MX25R6435F_SECTOR_SIZE);
	pInfo->ProgPageSize       = MX25R6435F_PAGE_SIZE;
	pInfo->ProgPagesNumber    = (MX25R6435F_FLASH_SIZE / MX25R6435F_PAGE_SIZE);

	return OSPI_OK;
}


/**
  * @brief  Configure the QSPI in memory-mapped mode
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_EnableMemoryMappedMode(OSPI_HandleTypeDef* handle)
{
	OSPI_RegularCmdTypeDef      sCommand;
	OSPI_MemoryMappedTypeDef	sMemMappedCfg;

	/* Configure the command for the read instruction */
	sCommand.OperationType         = HAL_OSPI_OPTYPE_READ_CFG;
	sCommand.FlashId               = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction           = QUAD_INOUT_READ_CMD;
	sCommand.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE;
	sCommand.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS;
	sCommand.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	sCommand.AddressMode           = HAL_OSPI_ADDRESS_4_LINES;
	sCommand.AddressSize           = HAL_OSPI_ADDRESS_24_BITS;
	sCommand.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE;
	sCommand.AlternateBytes        = MX25R6435F_ALT_BYTES_NO_PE_MODE;
	sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
	sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS;
	sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
	sCommand.DataMode              = HAL_OSPI_DATA_4_LINES;
	sCommand.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE;
	sCommand.DummyCycles           = MX25R6435F_DUMMY_CYCLES_READ_QUAD;
	sCommand.DQSMode               = HAL_OSPI_DQS_DISABLE;
	sCommand.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
	return OSPI_ERROR;
	}

	/* Configure the command for the program instruction */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
	sCommand.Instruction        = QUAD_PAGE_PROG_CMD;
	sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	sCommand.DummyCycles        = 0;

	/* Configure the command */
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
	return OSPI_ERROR;
	}

	/* Configure the memory mapped mode */
	sMemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

	if (HAL_OSPI_MemoryMapped(handle, &sMemMappedCfg) != HAL_OK)
	{
	return OSPI_ERROR;
	}


	return OSPI_OK;
}

/**
  * @brief  This function suspends an ongoing erase command.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_SuspendErase(OSPI_HandleTypeDef* handle)
{
	/* Check whether the device is busy (erase operation is in progress). */
	if (BSP_OSPI_GetStatus(handle) == OSPI_BUSY)
	{
		OSPI_RegularCmdTypeDef sCommand;

		/* Initialize the suspend command */
		sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
		sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
		sCommand.Instruction        = PROG_ERASE_SUSPEND_CMD;
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
		if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			return OSPI_ERROR;
		}

		if (BSP_OSPI_GetStatus(handle) == OSPI_SUSPENDED)
		{
			return OSPI_OK;
		}

		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  This function resumes a paused erase command.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_ResumeErase(OSPI_HandleTypeDef* handle)
{
	/* Check whether the device is in suspended state */
	if (BSP_OSPI_GetStatus(handle) == OSPI_SUSPENDED)
	{
		OSPI_RegularCmdTypeDef sCommand;

		/* Initialize the resume command */
		sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
		sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
		sCommand.Instruction        = PROG_ERASE_RESUME_CMD;
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
		if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		{
			return OSPI_ERROR;
		}

		/*
		When this command is executed, the status register write in progress bit is set to 1, and
		the flag status register program erase controller bit is set to 0. This command is ignored
		if the device is not in a suspended state.
		*/
		if (BSP_OSPI_GetStatus(handle) == OSPI_BUSY)
		{
			return OSPI_OK;
		}

		return OSPI_ERROR;
	}

	return OSPI_OK;
}

/**
  * @brief  This function enter the OSPI memory in deep power down mode.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_EnterDeepPowerDown(OSPI_HandleTypeDef* handle)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the deep power down command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = DEEP_POWER_DOWN_CMD;
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
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* ---          Memory takes 10us max to enter deep power down          --- */
	/* --- At least 30us should be respected before leaving deep power down --- */

	return OSPI_OK;
}

/**
  * @brief  This function leave the OSPI memory from deep power down mode.
  * @param  handle : pointer to OSPI_HandleTypeDef structure
  * @retval OSPI memory status
  */
uint8_t BSP_OSPI_LeaveDeepPowerDown(OSPI_HandleTypeDef* handle)
{
	OSPI_RegularCmdTypeDef sCommand;

	/* Initialize the erase command */
	sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
	sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;
	sCommand.Instruction        = NO_OPERATION_CMD;
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
	if (HAL_OSPI_Command(handle, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		return OSPI_ERROR;
	}

	/* --- A NOP command is sent to the memory, as the nCS should be low for at least 20 ns --- */
	/* ---                  Memory takes 35us min to leave deep power down                  --- */

	return OSPI_OK;
}
