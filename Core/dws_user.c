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
*  dws_user.c
*/

#include    "dws_user.h"
#include    <stdio.h>
#include    <ti/drivers/Timer.h>
#include    "source/ti/drivers/dpl/ClockP.h"
#include    <ti/drivers/dpl/SemaphoreP.h>
#include    <source/ti/posix/ccs/time.h>

#define EPOCH_OFFSET            2208988800  //in seconds
#define SECONDS_TO_NSECONDS     1000000000
#define MSECONDS_TO_NSECONDS    1000000
#define MAX_UINT32              0xFFFFFFFF

/*
 *  ======== thread_downsampling START ========
 *
 *  Reduce the sampling rate of ADC samples. When UDMA fills
 *  a buffer, a semaphore post in done. While semaphore is
 *  posted this thread will be activated.
 *
 */
void* thread_downsampling(void* arg_thread)
{
    uint16_t        * ptr_a, * ptr_b, * buffer_downsampling, n_msg_data, time_sync_off;
    uint16_t        index_buffer, cnt_buffer_out, cnt_buffer_in;
    uint16_t        values[8];
    uint32_t        downsampled_data_right, downsampled_data_left, fraction, nanoseconds;
    uint64_t        data_buffer_count, data_buffer_sync;
    uint64_t        time_synchro, time_sync_status;
    uint64_t        seconds, time_synchro_us;
    msg_data_t      * ptr_msg_data;
    struct timespec rtc_timer;
    int             ret;

    // Get parameters for the thread
    arg_thread_downsampling_t dws_struct;
    memcpy(&dws_struct, arg_thread, sizeof(arg_thread_downsampling_t));

    // Initialization of variables
    cnt_buffer_in       = 0;
    cnt_buffer_out      = 0;
    data_buffer_count   = 0;
    n_msg_data          = 0;
    ret                 = 0;
    time_sync_off       = 0;
    time_synchro_us     = 0;
    data_buffer_sync    = 0;

    dma_buffer_t * dma_buffer = (dma_buffer_t *) (dws_struct.buffer_in);
    buffer_downsampling = (uint16_t *)dma_buffer;

    // ptr_msg_data points to msg_data struct wich will be filled.
    ptr_msg_data = &(dws_struct.msg_data)[n_msg_data];

        while(true)
        {
            // Wait until the DMA transfer is completed
            SemaphoreP_pend(dws_struct.ptr_semaphore, SemaphoreP_WAIT_FOREVER);

            unsigned int index_ptr_a;
            for (index_ptr_a = 0; index_ptr_a < 8; index_ptr_a++)
            {
                values[index_ptr_a] = buffer_downsampling[index_ptr_a];
            }

            // Time synchronization
            memcpy(&time_sync_status, dws_struct.status_time_sync, sizeof(time_sync_status));

            if(time_sync_status != 0)
            {
                // Get the RTC timer to meas the time will pass to next buffer proccess.
                ret = clock_gettime(CLOCK_REALTIME, &rtc_timer);
                CHECK_ERROR(ret, "Error getting rtc timer in DWS THREAD!!");

                memcpy(&time_synchro, dws_struct.ptr_sntp_time, sizeof(time_synchro));
                // The seconds value is stored in the upper 32 bits
                seconds = (0xFFFFFFFF00000000 & time_synchro) >> 32;
                seconds = ((seconds + (uint64_t)rtc_timer.tv_sec - EPOCH_OFFSET) * SECONDS_TO_NSECONDS) + (uint64_t)rtc_timer.tv_nsec;
                // The seconds fraction is stored in the lower 32 bits
                fraction = 0x00000000FFFFFFFF & time_synchro;
                nanoseconds = (uint32_t)(((double)fraction / MAX_UINT32) * SECONDS_TO_NSECONDS);
                time_synchro_us = seconds + nanoseconds;

                // Copy in the data message the number of buffer with the last synchronization
                memcpy(&(ptr_msg_data->p.h.corr_buffer_id), &data_buffer_count, sizeof(data_buffer_count));
                data_buffer_sync = data_buffer_count;

                memcpy(dws_struct.status_time_sync, &time_sync_off , sizeof(time_sync_off));
            }

            memcpy(&(ptr_msg_data->p.h.corr_buffer_id), &data_buffer_sync, sizeof(data_buffer_sync));
            memcpy(&(ptr_msg_data->p.h.rtc_value), &time_synchro_us, sizeof(ptr_msg_data->p.h.rtc_value));

            //Oversample the data direct in  ptr -> buffer_downsampling, remove trash and copy in dws_buffer
            index_buffer = 0;
            while (index_buffer < ((dws_struct.size_buffer_in) / (DWS_RATE_FIX * 2)))
            {
                // ptr_a takes the address of buffer_downsampling in the position specified.
                ptr_a = &buffer_downsampling[index_buffer * (DWS_RATE_FIX * 2)];


                // Remove and mask unused bits stored in the direction [0-7] of ptr. Sum the data processed and downsample.
                downsampled_data_right =  (((uint16_t) (ptr_a[0] >> 2) & 0x0FFF)  +
                                          ( (uint16_t) (ptr_a[1] >> 2) & 0x0FFF) +
                                          ( (uint16_t) (ptr_a[2] >> 2) & 0x0FFF) +
                                          ( (uint16_t) (ptr_a[3] >> 2) & 0x0FFF) +
                                          ( (uint16_t) (ptr_a[4] >> 2) & 0x0FFF) +
                                          ( (uint16_t) (ptr_a[5] >> 2) & 0x0FFF) +
                                          ( (uint16_t) (ptr_a[6] >> 2) & 0x0FFF) +
                                          ( (uint16_t) (ptr_a[7] >> 2) & 0x0FFF));
                downsampled_data_right = downsampled_data_right >> 3;

                // Remove and mask unused bits stored in the direction [0-7] of ptr. Sum the data processed and downsample.
                ptr_b = &buffer_downsampling[DWS_RATE_FIX + index_buffer * (DWS_RATE_FIX * 2)];
                downsampled_data_left =  (((uint16_t) (ptr_b[0] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[1] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[2] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[3] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[4] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[5] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[6] >> 2) & 0x0FFF) +
                                         ( (uint16_t) (ptr_b[7] >> 2) & 0x0FFF)) / DWS_RATE_FIX;

                // Copy downsampled_data_right and downsampled_data_left in a 32 bit variable.
                uint32_t downsampled_data = (downsampled_data_right & 0xFFF) | (downsampled_data_left << 12);

                memcpy(&(ptr_msg_data->p.p[cnt_buffer_out].p[index_buffer * 3]), &downsampled_data, 3);

                index_buffer ++;
            }

            memcpy(&(ptr_msg_data->p.p[cnt_buffer_out].buffer_id), &data_buffer_count, sizeof(data_buffer_count));

            // Increment counter of data buffer id;
            data_buffer_count ++;

            // Increment counter of the exit buffer that will write next time.
            cnt_buffer_out = (cnt_buffer_out + 1);

           if (cnt_buffer_out == DATA_BUFFER_MAX)
           {
               cnt_buffer_out = 0;
               n_msg_data = (n_msg_data + 1) % N_MSG_DATA;
               ptr_msg_data = &(dws_struct.msg_data)[n_msg_data];
               SemaphoreP_post(dws_struct.ptr_semaphore_mqtt);
           }

            // Increment counter of dma_buffer that will proccess next time.
            cnt_buffer_in = (cnt_buffer_in + 1) % dws_struct.dma_num_buffers;

            // Buffer_downsampling points to dma_buffer which needs proccess.
            buffer_downsampling =  (uint16_t *) (((uint8_t *) dma_buffer) + cnt_buffer_in * (dws_struct.size_buffer_in) * sizeof(uint16_t));
        }
}
/*
 *  ======== thread_downsampling END ========
 *
 */
