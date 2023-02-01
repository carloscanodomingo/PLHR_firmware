
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
*  print_user.h
*/


#ifndef PRINT_USER_H_
#define PRINT_USER_H_

#include <ti/display/Display.h>
#include "ti_drivers_config.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define PRINT_NO_ERROR  (0)
#define PRINT_BUFFERS

int print_values_downsampled (int max_size, uint8_t *print_buffer, int print_buffer_size);

#endif /* PRINT_USER_H_ */
