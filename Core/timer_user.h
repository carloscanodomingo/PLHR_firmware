
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
*  timer_user.h
*/

#ifndef TIMER_USER_H_
#define TIMER_USER_H_

#include <ti/drivers/Timer.h>
#include <stddef.h>
#include "ti_drivers_config.h"

#define TIMER_NO_ERROR          (0)
#define ADC_CONVERSION_RATE     (62500)
#define MICROSECONDS            (1000000)

Timer_Handle    timer_handle;
Timer_Params    timer_params;

int     timer_open      (void);
int     timer_config    (void);
int     timer_start     (void);
int     timerCallback   (Timer_Handle myHandle);

void timerLEDCallback(Timer_Handle myHandle);

#endif /* TIMER_USER_H_ */
