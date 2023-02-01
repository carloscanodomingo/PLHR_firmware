
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
*  timer_user.c
*/


#include  "timer_user.h"
#include <ti/drivers/GPIO.h>

/*
 * @brief Open timer.
 *
 *
 * @return error code
 */
int timer_open(void)
{
    Timer_init();

    return TIMER_NO_ERROR;
}

/*
 * @brief Start timer.
 *
 *
 * @return error code
 */
 int timer_start(void)
{

    return TIMER_NO_ERROR;
}

/*
 * @brief Configures timer
 *
 *
 * @return error code
 */
int timer_config(void)
{
    return TIMER_NO_ERROR;
}
