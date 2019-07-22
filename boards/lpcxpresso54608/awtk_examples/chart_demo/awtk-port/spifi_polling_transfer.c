/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_spifi.h"

#include "pin_mux.h"
#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_SPIFI SPIFI0
#define PAGE_SIZE (256)
#define SECTOR_SIZE (4096)
#define EXAMPLE_SPI_BAUDRATE (96000000)
#define COMMAND_NUM (6)
#define READ (0)
#define PROGRAM_PAGE (1)
#define GET_STATUS (2)
#define ERASE_SECTOR (3)
#define WRITE_ENABLE (4)
#define WRITE_REGISTER (5)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t g_buffer[PAGE_SIZE] = {0};

#if defined FLASH_W25Q
spifi_command_t command[COMMAND_NUM] = {
    {PAGE_SIZE, false, kSPIFI_DataInput, 1, kSPIFI_CommandDataQuad, kSPIFI_CommandOpcodeAddrThreeBytes, 0x6B},
    {PAGE_SIZE, false, kSPIFI_DataOutput, 0, kSPIFI_CommandDataQuad, kSPIFI_CommandOpcodeAddrThreeBytes, 0x32},
    {1, false, kSPIFI_DataInput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x05},
    {0, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeAddrThreeBytes, 0x20},
    {0, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x06},
    {1, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x31}};
#define QUAD_MODE_VAL 0x02
#elif defined FLASH_MX25R
spifi_command_t command[COMMAND_NUM] = {
    {PAGE_SIZE, false, kSPIFI_DataInput, 1, kSPIFI_CommandDataQuad, kSPIFI_CommandOpcodeAddrThreeBytes, 0x6B},
    {PAGE_SIZE, false, kSPIFI_DataOutput, 0, kSPIFI_CommandOpcodeSerial, kSPIFI_CommandOpcodeAddrThreeBytes, 0x38},
    {1, false, kSPIFI_DataInput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x05},
    {0, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeAddrThreeBytes, 0x20},
    {0, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x06},
    {1, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x01}};
#define QUAD_MODE_VAL 0x40
#else /* Use MT25Q */
spifi_command_t command[COMMAND_NUM] = {
    {PAGE_SIZE, false, kSPIFI_DataInput, 1, kSPIFI_CommandDataQuad, kSPIFI_CommandOpcodeAddrThreeBytes, 0x6B},
    {PAGE_SIZE, false, kSPIFI_DataOutput, 0, kSPIFI_CommandOpcodeSerial, kSPIFI_CommandOpcodeAddrThreeBytes, 0x38},
    {1, false, kSPIFI_DataInput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x05},
    {0, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeAddrThreeBytes, 0x20},
    {0, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x06},
    {1, false, kSPIFI_DataOutput, 0, kSPIFI_CommandAllSerial, kSPIFI_CommandOpcodeOnly, 0x61}};
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
void check_if_finish()
{
    uint8_t val = 0;
    /* Check WIP bit */
    do
    {
        SPIFI_SetCommand(EXAMPLE_SPIFI, &command[GET_STATUS]);
        while ((EXAMPLE_SPIFI->STAT & SPIFI_STAT_INTRQ_MASK) == 0U)
        {
        }
        val = SPIFI_ReadDataByte(EXAMPLE_SPIFI);
    } while (val & 0x1);
}

#if defined QUAD_MODE_VAL
void enable_quad_mode()
{
    /* Write enable */
    SPIFI_SetCommand(EXAMPLE_SPIFI, &command[WRITE_ENABLE]);

    /* Set write register command */
    SPIFI_SetCommand(EXAMPLE_SPIFI, &command[WRITE_REGISTER]);

    SPIFI_WriteDataByte(EXAMPLE_SPIFI, QUAD_MODE_VAL);

    check_if_finish();
}
#endif

int spifi_flash_init(void)
{
    spifi_config_t config = {0};
    uint32_t sourceClockFreq;

    /* attach 12 MHz clock to SPI3 */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM9);

    /* Set SPIFI clock source */
    CLOCK_AttachClk(kFRO_HF_to_SPIFI_CLK);
    sourceClockFreq = CLOCK_GetFroHfFreq();

    /* Set the clock divider */
    CLOCK_SetClkDiv(kCLOCK_DivSpifiClk, sourceClockFreq / EXAMPLE_SPI_BAUDRATE, false);

    /* Initialize SPIFI */
    SPIFI_GetDefaultConfig(&config);
    SPIFI_Init(EXAMPLE_SPIFI, &config);

#if defined QUAD_MODE_VAL
    /* Enable Quad mode */
    enable_quad_mode();
#endif

    /* Setup memory command */
    SPIFI_SetMemoryCommand(EXAMPLE_SPIFI, &command[READ]);

}
