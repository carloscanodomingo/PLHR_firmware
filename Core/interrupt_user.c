
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
*  interrupt_user.c
*/

#include  "interrupt_user.h"

#define MAX_NUM_INT  1

typedef struct {
    int             interrupt_num;
    HwiP_Handle     hwiHandle;
}int_handler_t;

HwiP_Params hwiParams;
int_handler_t int_handler[MAX_NUM_INT];

/*
 * @brief Configures interrupts.
 *
 *
 * @return error code
 */
int interrupt_config(void)
{
    HwiP_Params_init(&hwiParams);
    int i;
    for (i = 0; i < MAX_NUM_INT; ++i)
        int_handler[i].interrupt_num = 0;

    return INTERRUPT_NO_ERROR;
}

/*
 * @brief Sarts interrupts.
 *
 *
 * @return error code
 */
int udma_interrupt_start(int interrupt_num, void * handler)
{
    HwiP_Handle hwiHandle = HwiP_create(interrupt_num, handler, &hwiParams);
   if (hwiHandle == NULL) {
       return INTERRUPT_NOT_CREATED;
   }
   int i = 0;
   while(i < MAX_NUM_INT)
   {
       if (int_handler[i].interrupt_num == 0)
       {
           int_handler[i].interrupt_num = interrupt_num;
           int_handler[i].hwiHandle = hwiHandle;
           break;
       }
       i++;
   }

   if (i == MAX_NUM_INT)
       return INTERRUPT_NOT_INTERRUPT_SIZE;

    return INTERRUPT_NO_ERROR;
}

/*
 * @brief Disables interrupts.
 *
 *
 * @return error code
 */
int interrupt_user_disable(int interrupt_num)
{
    int i = 0;
    while(i < MAX_NUM_INT)
    {
        if (int_handler[i].interrupt_num == interrupt_num)
        {
            HwiP_disableInterrupt(int_handler[i].interrupt_num);
            HwiP_delete(int_handler[i].hwiHandle);
            break;
        }
        i++;
    }
    if (i == MAX_NUM_INT)
        return INTERRUPT_HAS_NOT_BEEN_CREATED;

    return INTERRUPT_NO_ERROR;
}
