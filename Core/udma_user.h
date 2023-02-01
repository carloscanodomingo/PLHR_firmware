
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
*  udma_user.h
*/

#ifndef UDMA_USER_H_
#define UDMA_USER_H_

#include "hw_types.h"
#include "stdint.h"
#include "adc.h"
#include "udma.h"
#include "ti/drivers/dma/UDMACC32XX.h"

typedef enum{
    DMA_STOP_MODE_PRI,
    DMA_STOP_MODE_ALT,
    DMA_STOP_MODE_BASIC,
    DMA_STOP_NONE,

} dma_stop_mode_t;

typedef enum{
    DMA_SET_MODE_PRI,
    DMA_SET_MODE_ALT,
    DMA_SET_MODE_BASIC,
    DMA_SET_NONE,
} dma_set_mode_t;

#define UDMA_NO_ERROR       (0)

int     udma_open               (void);
int     udma_setup_transfer_pri (void * buffer_pri, uint32_t channel_id, unsigned long size);
int     udma_setup_transfer_alt (void * buffer_alt, uint32_t channel_id, unsigned long size);
void    udma_irq_handler        (void);
void    udma_adc_handler        (void);
int     dma_int_clear           (void);
int     dma_transfer_set        (dma_set_mode_t dma_set_mode, uint32_t channel_id, void * buffer, unsigned long size);
int     dma_mode_recharge       (int wr_num, int count_wr, unsigned long size, int dma_buff_next, int dma_buff_num);
int     dma_stop_mode_get       (dma_stop_mode_t * dma_stop_mode);
int     udma_disable            (uint32_t channel_id);
int     udma_init               (void);

#endif /* UDMA_USER_H_ */
