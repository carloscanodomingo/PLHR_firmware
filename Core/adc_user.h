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
*  adc_user.h
*/

#ifndef ADC_USER_H_
#define ADC_USER_H_

#include "adc.h"
#include <stdint.h>
#include "ti/drivers/ADC.h"
#include "ti_drivers_config.h"

#define ADC_NO_ERROR        0

int adc_dma_enable      (uint32_t channel_id);
int adc_dma_disable     (uint32_t channel_id);
int adc_int_enable      (uint32_t channel_id);
int adc_int_register    (uint32_t channel_id, void (*adc_irq) (void));
int adc_start           (uint32_t channel_id);
int adc_stop            (uint32_t channel_id);
int udma_enable         (void);
int adc_int_clear       (uint32_t channel_id);
int adc_open            (void);
int adc_config          (uint32_t channel_id);
int adc_init            (void);

#endif /* ADC_USER_H_ */
