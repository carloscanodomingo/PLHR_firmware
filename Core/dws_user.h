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
*  dws_user.h
*/
#ifndef DWS_USER_H_
#define DWS_USER_H_

#include "ti_drivers_config.h"
#include <ti/drivers/dpl/SemaphoreP.h>
#include <unistd.h>
#include <ti/net/mqtt/mqttclient.h>
#include <mqtt_user.h>
#include "global_def.h"
#include "gpio_user.h"
#include "msg_structs.h"

#define DWS_NO_ERROR            (0)
#define DMA_DIVIDE_CNT          (2)
#define DWS_RATE_FIX            (8)
#define SND_MQTT_PACKET_SIZE    (104)

typedef struct __attribute__((__packed__))
{
    uint8_t           *buffer_in;           // Buffer to proccess.
    uint8_t           *buffer_out;          // Buffer after proccess.
    uint8_t           dma_num_buffers;      // Number of buffer to proccess.
    uint8_t           dws_max_buffer;       // Number of buffer after proccess.
    uint16_t          size_buffer_in;       // Size if buffer to proccess.
    uint16_t          size_buffer_out;      // Size of exit's buffer.
    uint16_t          dws_buffer_size;      // Size of buffer after proccess.
    uint64_t          *status_time_sync;
    uint64_t          *ptr_sntp_time;       // Pointer at real timer.
    msg_data_t        *msg_data;
    SemaphoreP_Handle ptr_sync_semaphore;
    SemaphoreP_Handle ptr_semaphore;        // Pointer at semaphore.
    SemaphoreP_Handle ptr_semaphore_mqtt;

} arg_thread_downsampling_t;

typedef struct
{
    SemaphoreP_Handle   ptr_semaphore_mqtt;
    uint8_t             mqtt_will_qos;
    MQTTClient_Handle   mqtt_client_handle;
    const char          * topic;
    uint8_t             * buffer_out;

} arg_thread_snd_mqtt_t;

void* thread_downsampling(void* arguments);

#endif /* DWS_USER_H_ */
