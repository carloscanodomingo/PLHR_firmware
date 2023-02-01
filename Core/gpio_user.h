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
*  gpio_user.h
*/

#ifndef GPIO_USER_H_
#define GPIO_USER_H_

#define GPIO_NO_ERROR       (0)

#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

int     gpio_open(void);

#endif /* GPIO_USER_H_ */
