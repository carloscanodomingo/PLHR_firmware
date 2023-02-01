
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
*  print_user.c
*/

#include  "print_user.h"

static const char display_buffer_start[] = "buffer=[";
static const char display_buffer_end  [] = "];";


/*
 * @brief Print values after downsampling process.
 *
 * param[in]    - max_size
 *              - print_buffer
 *              - print_buffer_size
 *
 * @return error code
 */
#ifdef PRINT_BUFFERS
int print_values_downsampled (int max_size, uint8_t *print_buffer, int print_buffer_size)
{

    char temp_buffer[10];

    unsigned int max_size_buffer_to_uart = max_size;

    char buffer_to_uart[max_size_buffer_to_uart];

    uint16_t cnt_buffer_uart = 0, downsampled_buffer_size;

    uint8_t * current_buffer;

    unsigned int size_sample_char = 10;
    int index_num_buffer = 0;

    // Current_buffer is pointing to dws_buffer
    current_buffer = &print_buffer[0];

    // Size of the content in dws_buffer
    downsampled_buffer_size = print_buffer_size;

    // Copy to buffer_to_uart "display_buffer_start instruction
    memcpy(&buffer_to_uart[cnt_buffer_uart], display_buffer_start, strlen(display_buffer_start));
    cnt_buffer_uart = strlen(display_buffer_start);

    // Run dws_buffer for to print three 8 bits word each.
    for (index_num_buffer = 0; index_num_buffer < downsampled_buffer_size; index_num_buffer = index_num_buffer + 3)
    {
        // Count the num of chars used.
        int num_chars;

        // Get three 8 bits word
        uint16_t temp_value_right =      ((current_buffer[index_num_buffer]) | ((current_buffer[index_num_buffer + 1 ] & 0xFF) << 8)) & 0x0FFF;
        uint16_t temp_value_left =       ((current_buffer[index_num_buffer + 1 ] & 0xFF0) >> 4) | ((current_buffer[index_num_buffer + 2]) << 4);

        // Copy in buffer_to_uart three 8 bits word
        num_chars = (snprintf(temp_buffer, size_sample_char, "%u,", temp_value_right));
        memcpy(&buffer_to_uart[cnt_buffer_uart], temp_buffer, num_chars);
        cnt_buffer_uart = cnt_buffer_uart + num_chars;

        num_chars = (snprintf(temp_buffer, size_sample_char, "%u,", temp_value_left));
        memcpy(&buffer_to_uart[cnt_buffer_uart], temp_buffer, num_chars);
        cnt_buffer_uart = cnt_buffer_uart + num_chars;
    }

    buffer_to_uart[cnt_buffer_uart - 1] = '\0';

    memcpy(&buffer_to_uart[cnt_buffer_uart - 1], display_buffer_end, sizeof(display_buffer_end));

    return PRINT_NO_ERROR;
}


//int print_values (void)
//{
//
//    char temp_buffer[10];
//
//    unsigned int max_size_buffer_to_uart = DMA_BUFF_SIZE * 6;
//
//    char buffer_to_uart[max_size_buffer_to_uart];
//
//    uint16_t cnt_buffer_uart = 0;
//
//    uint16_t * current_buffer;
//
//    int i;
//    unsigned int size_sample_char = 6;
//    int index_num_buffer = 0;
//
//    memcpy(&buffer_to_uart[cnt_buffer_uart], display_buffer_start, strlen(display_buffer_start));
//    cnt_buffer_uart = strlen(display_buffer_start);
//
//    for (index_num_buffer = 0; index_num_buffer < DMA_NUM_BUFFERS; ++index_num_buffer)  //Una pasada por cada buffer
//    {
//        current_buffer = dma_buffer [index_num_buffer];
//        for(i = 0; i < DMA_BUFF_SIZE; i++) //Una pasada por cada dato del buffer
//        {
//            int num_chars;
//            uint16_t temp_value =  ((current_buffer[i] >> 2) & 0x0FFF);
//            num_chars = (snprintf(temp_buffer, size_sample_char, "%u,", temp_value));
//            if (cnt_buffer_uart + num_chars < max_size_buffer_to_uart)
//            {
//                memcpy(&buffer_to_uart[cnt_buffer_uart], temp_buffer, num_chars);
//                cnt_buffer_uart = cnt_buffer_uart + num_chars;
//            }
//            else
//            {
//                Display_printf(display, 0, 0, "Memory not allowed");
//                return 1;
//            }
//        }
//        buffer_to_uart[cnt_buffer_uart - 1] = '\0';
//
//        if (cnt_buffer_uart + 1 < max_size_buffer_to_uart)
//        {
//            if(index_num_buffer == DMA_NUM_BUFFERS - 1)
//                memcpy(&buffer_to_uart[cnt_buffer_uart - 1], display_buffer_end, sizeof(display_buffer_end));
//
//            Display_printf(display, 0, 0, "%s", buffer_to_uart);
//            cnt_buffer_uart = 0;
//        }
//        else
//        {
//            Display_printf(display, 0, 0, "Memory not allowed");
//            return 2;
//        }
//    }
//    return PRINT_VALUES_NO_ERROR;
//}
#endif
