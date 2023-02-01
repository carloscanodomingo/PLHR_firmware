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
*  gpio_user.c
*/

#include  "gpio_user.h"

/*
 * @brief Open general purpose inputs and outputs.
 *
 * @param[in] none
 *
 * @return error code
 */
int gpio_open(void)
{
    GPIO_init();
    GPIO_setConfig(CONFIG_LED_RED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_LED_RED, CONFIG_GPIO_LED_OFF);

    return GPIO_NO_ERROR;
}
