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
*  adc_user.c
*/
#include "adc_user.h"

#include <stdint.h>
#include "hw_types.h"
#include "pin.h"
#include "hw_memmap.h"
#include "rom_map.h"

/*
 * @brief Configures ADC in specific channel defined by user.
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 *
 * @return error code
 */
int adc_dma_enable(uint32_t channel_id)
{
    ADCDMAEnable(ADC_BASE, channel_id);

    return ADC_NO_ERROR;
}

/*
 * @brief Disable ADC in specific channel defined by user.
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 *
 * @return error code
 */
int adc_dma_disable(uint32_t channel_id)
{
    ADCDMADisable(ADC_BASE, channel_id);

    return ADC_NO_ERROR;
}

/*
 * @brief Configures interrupts for ADC in specific channel
 * defined by user.
 *
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 *
 * @return error code
 */
int adc_int_register(uint32_t channel_id, void (*adc_irq) (void))
{
    ADCIntRegister(ADC_BASE, channel_id, adc_irq);

    return ADC_NO_ERROR;
}

/*
 * @brief Enables ADC's interrupts with ADC_DMA_DONE flag.
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 * @param[in] adc_irq - Interupt's routine.
 *
 * @return error code
 */
int adc_int_enable(uint32_t channel_id)
{
    //ADCIntEnable(ADC_BASE, channel_id,  ADC_DMA_DONE); //it was in adc_int_star.
    ADCIntDisable(ADC_BASE, channel_id,  ADC_FIFO_OVERFLOW | ADC_FIFO_UNDERFLOW | ADC_FIFO_EMPTY | ADC_FIFO_FULL); //it was in adc_int_star.
    return ADC_NO_ERROR;
}

/*
 * @brief Clears ADC's interrupts for a specific channel.
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 *
 * @return error code
 */
int adc_int_clear(uint32_t channel_id)
{
    uint16_t adc_int_status = ADCIntStatus(ADC_BASE, channel_id);
    ADCIntClear(ADC_BASE, channel_id, ADC_DMA_DONE);

    return ADC_NO_ERROR;
}

/*
 * @brief Start ADC conversion.
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 *
 * @return error code
 */
int adc_start(uint32_t channel_id)
{
    ADCChannelEnable(ADC_BASE, channel_id);
    ADCEnable(ADC_BASE);

    return ADC_NO_ERROR;
}

/*
 * @brief Stop ADC conversion.
 *
 * @param[in] channel_id - Channel used for ADC conversion.
 *
 * @return error code
 */
int adc_stop(uint32_t channel_id)
{
    ADCDisable(ADC_BASE);
    //ADCIntDisable(ADC_BASE, channel_id, ADC_DMA_DONE | ADC_FIFO_FULL | ADC_FIFO_EMPTY | ADC_FIFO_UNDERFLOW | ADC_FIFO_OVERFLOW);

    return ADC_NO_ERROR;
}

/*
 * @brief Open ADC peripheral with functions located in
 * ADCCC32XX.h.
 *
 *
 * @param[in] none.
 *
 * @return error code
 */
int adc_open(void)
{
    ADC_Params param;
    ADC_init();
    ADC_Handle adcch0 = ADC_open(CC3220SF_LAUNCHXL_ADC0, &param);

    return ADC_NO_ERROR;
}

/*
 * @brief Configurates ADC peripheral with functions located
 * in adc.h
 *
 *
 * @param[in] none.
 *
 * @return error code
 */
int adc_config(uint32_t channel_id)
{
    ADCChannelDisable(ADC_BASE, channel_id);
    ADCDMAEnable(ADC_BASE, channel_id);
    ADCIntEnable(ADC_BASE, channel_id, ADC_DMA_DONE);
    ADCChannelEnable(ADC_BASE, channel_id);

    return ADC_NO_ERROR;
}

/*
 * @brief Open ADC peripheral with functions located in
 * ADCCC32XX.h.
 *
 *
 * @param[in] none.
 *
 * @return error code
 */
int adc_init(void)
{
    ADC_init();

    return ADC_NO_ERROR;
}
