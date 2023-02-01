
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
*  interrupt_user.h
*/



#ifndef INTERRUPT_USER_H_
#define INTERRUPT_USER_H_

#include "source/ti/drivers/dpl/HwiP.h"
#include "ti_drivers_config.h"
#include <ti/devices/cc32xx/inc/hw_ints.h>

#define INTERRUPT_NO_ERROR              (0)
#define INTERRUPT_NOT_CREATED           (-1)
#define INTERRUPT_NOT_INTERRUPT_SIZE    (-2)
#define INTERRUPT_HAS_NOT_BEEN_CREATED  (-3)

int interrupt_config        (void);
int udma_interrupt_start    (int interrupt_num, void * handler);
int interrupt_user_disable  (int interrupt_num);

#endif /* INTERRUPT_USER_H_ */
