
/*
* TIC-019 University of Almería
*
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
*
*  udma_user.c
*/

#include "rom_map.h"
#include "udma_user.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "interrupt.h"
#include "pin.h"
#include "string.h"
#include "hw_adc.h"
#include "udma.h"
#include "hw_types.h"

#define ADC_BASE_FIFO_DATA      0x4402E874
#define CTL_TBL_SIZE            (64)
#define UDMA_CHANNEL_SELECT     UDMA_CH16_ADC_CH2
#define UDMA_ARBITR             UDMA_ARB_1

void udma_irq_fault_handler (void);
void udma_irq_sw_handler    (void);

#pragma DATA_ALIGN(gpCtlTbl, 1024)
tDMAControlTable gpCtlTbl[CTL_TBL_SIZE];
static UDMACC32XX_Handle udma;

/*
 * @brief Initialites UDMA with the functions imported of
 * UDMACC32XX.h.
 *
 *
 * @param[in] none
 *
 * @return error code
 */
int udma_init(void)
{

    UDMACC32XX_init();

    return UDMA_NO_ERROR;
}

/*
 * @brief Open UDMA with the functions imported of
 * UDMACC32XX.h.
 *
 *
 * @param[in] none
 *
 * @return error code
 */
int udma_open(void)
{

    udma = UDMACC32XX_open();

    return UDMA_NO_ERROR;
}

/*
 * @brief Configures UDMA transfer.
 *
 *
 *
 * @return error code
 */
int  udma_setup_transfer_pri(void * buffer_pri, uint32_t channel_id, unsigned long size)
{
    dma_transfer_set(DMA_SET_MODE_PRI, channel_id, buffer_pri, size);

    return UDMA_NO_ERROR;
}
/*
 * @brief Configures UDMA transfer.
 *
 *
 *
 * @return error code
 */
int  udma_setup_transfer_alt(void * buffer_alt, uint32_t channel_id, unsigned long size)
{
    dma_transfer_set(DMA_SET_MODE_ALT, channel_id, buffer_alt, size);

    return UDMA_NO_ERROR;
}
int udma_enable(void)
{
    MAP_uDMAEnable();
    return UDMA_NO_ERROR;
}

int udma_disable(uint32_t channel_id)
{
    //ToDo
    uDMAChannelTransferSet(UDMA_CHANNEL_SELECT | UDMA_PRI_SELECT, UDMA_MODE_STOP, (void*)(ADC_BASE_FIFO_DATA + channel_id), NULL, 0);
    uDMAChannelTransferSet(UDMA_CHANNEL_SELECT | UDMA_ALT_SELECT, UDMA_MODE_STOP, (void*)(ADC_BASE_FIFO_DATA + channel_id), NULL, 0);

    return UDMA_NO_ERROR;
}

void udma_irq_fault_handler(void)
{
    MAP_uDMAIntClear(MAP_uDMAIntStatus());
    while(1);
}

/*
 * @brief Interrupt routine for UDMA. Only for verification.
 *
 *
 * @return none
 */
void udma_irq_sw_handler(void)
{
    unsigned long uiIntStatus;
    uiIntStatus = MAP_uDMAIntStatus();
    MAP_uDMAIntClear(uiIntStatus);
}

/*
 * @brief Clear the interrupt that have trigger the interrupt routine.
 *
 *
 * @return error code
 */
int dma_int_clear (void)
{

    return UDMA_NO_ERROR;
}

/*
 * @brief Get the last structure (primary or alternative)
 * that have been used by DMA and recharge it.
 *
 *
 * @return error code
 */

int    dma_stop_mode_get (dma_stop_mode_t * dma_stop_mode)
{
    *dma_stop_mode = DMA_STOP_NONE;
    if (uDMAChannelModeGet(UDMA_CHANNEL_SELECT | UDMA_PRI_SELECT) == UDMA_MODE_STOP)
    {
        *dma_stop_mode = DMA_STOP_MODE_PRI;
    }
    else if (uDMAChannelModeGet(UDMA_CHANNEL_SELECT | UDMA_ALT_SELECT) == UDMA_MODE_STOP)
    {
       *dma_stop_mode = DMA_STOP_MODE_ALT;
    }

    return UDMA_NO_ERROR;
}

/*
 * @brief Set the UDMA transfer with the structure needly.
 *
 *
 *@param[in] mode - str-ucture set in transfer.
 *                -> UDMA_PRI_MODE
 *                -> UDMA_ALT_MODE
 *           mem -  pointer to memory adrres where udma data are saved.
 *
 *
 * @return error code
 */
int dma_transfer_set(dma_set_mode_t dma_set_mode, uint32_t channel_id, void * buffer, unsigned long size)
{
    switch (dma_set_mode)
    {
    case DMA_SET_MODE_PRI:
        uDMAChannelAssign(UDMA_CHANNEL_SELECT | UDMA_PRI_SELECT);
        uDMAChannelControlSet(UDMA_CHANNEL_SELECT | UDMA_PRI_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARBITR);
        uDMAChannelTransferSet(UDMA_CHANNEL_SELECT | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG, (void*)(ADC_BASE_FIFO_DATA + channel_id), buffer, size);
        uDMAChannelEnable(UDMA_CHANNEL_SELECT | UDMA_PRI_SELECT);
        break;

    case DMA_SET_MODE_ALT:
        uDMAChannelAssign(UDMA_CHANNEL_SELECT  | UDMA_ALT_SELECT);
        uDMAChannelControlSet(UDMA_CHANNEL_SELECT  | UDMA_ALT_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARBITR);
        uDMAChannelTransferSet(UDMA_CHANNEL_SELECT | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG, (void*)(ADC_BASE_FIFO_DATA + channel_id), buffer, size);
        uDMAChannelEnable(UDMA_CHANNEL_SELECT  | UDMA_ALT_SELECT);
        break;
    case DMA_SET_MODE_BASIC:
        uDMAChannelAssign(UDMA_CHANNEL_SELECT );
        uDMAChannelControlSet(UDMA_CHANNEL_SELECT, UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARBITR);
        uDMAChannelTransferSet(UDMA_CHANNEL_SELECT, UDMA_MODE_BASIC, (void*)(ADC_BASE_FIFO_DATA + channel_id), buffer, size);
        uDMAChannelEnable(UDMA_CHANNEL_SELECT );
        break;

    case DMA_SET_NONE:

        break;
    }

    return UDMA_NO_ERROR;
}
