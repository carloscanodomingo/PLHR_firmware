
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
*  msg_struct.h
*/


#ifndef MSG_STRUCTS_H_
#define MSG_STRUCTS_H_

#include <stdint.h>
#include "global_def.h"

#define DATA_BUFFER_L           (150)
#define DATA_BUFFER_MAX         (78)
#define N_MSG_DATA              (10)

typedef enum {
    CMD_READY    = 1,
    CMD_RESP_ID,
    CMD_SYNC,
    CMD_RSSI_STATUS,

}cmd_enum_t;

typedef enum {
    CMD = 1,
    DATA,
    CONFIG,

}msg_op_t;

typedef struct __attribute__((__packed__))
{
    uint8_t     id_device;
    uint8_t     msg_op;
    uint16_t    msg_size;

}msg_header_t;

/* ---------------- STRUCT FOR MSG CMD ---------------- */

typedef struct __attribute__((__packed__))
{
    uint64_t rtc_value;
    uint64_t curr_buff_id;

}cmd_sync_t;

typedef struct __attribute__((__packed__))
{
    int16_t avg_data_ctrl_rssi;
    int16_t avg_msg_mnt_rssi;
    uint32_t rcv_fcs_err_pkt_num;

}cmd_rssi_t;

typedef struct __attribute__((__packed__))
{
    uint8_t     mac_addr[SIZE_MAC_ADDR];
    uint64_t    time_per_sample;

}cmd_ready_t;

typedef struct __attribute__((__packed__))
{
    uint8_t     mac_addr[SIZE_MAC_ADDR];
    uint16_t    cmd_resp_id;

}cmd_resp_id_t;

typedef union __attribute__((__packed__))
{
    cmd_ready_t     cmd_ready;
    cmd_sync_t      cmd_sync;
    cmd_resp_id_t   cmd_res_id;
    cmd_rssi_t      cmd_rssi;
    uint16_t        cmd_257;
    uint16_t        cmd_3;

}cmd_type_t;

typedef struct __attribute__((__packed__))
{
    uint16_t    cmd_id;

}cmd_header_t;

typedef struct __attribute__((__packed__))
{
    cmd_header_t    h;
    cmd_type_t      p;

}cmd_payload_t;

typedef struct __attribute__((__packed__))
{
    msg_header_t    h;
    cmd_payload_t   p;

}msg_cmd_t;

/* ---------------- STRUCT FOR MSG MQTT ---------------- */

typedef struct __attribute__((__packed__))
{
    uint64_t       buffer_id;
    uint8_t        p[DATA_BUFFER_L];

}data_buffer_t;

typedef struct __attribute__((__packed__))
{
    uint32_t data_id;
    uint16_t buffer_size;
    uint16_t data_buffer_num;
    uint64_t rtc_value;
    uint64_t corr_buffer_id;

}data_header_t;

typedef struct __attribute__((__packed__))
{
    data_header_t       h;
    data_buffer_t       p[DATA_BUFFER_MAX];

}data_payload_t;

typedef struct __attribute__((__packed__))
{
    msg_header_t        h;
    data_payload_t      p;

}msg_data_t;

/* ---------------- STRUCT FOR MSG RECEPTION ---------------- */

typedef struct __attribute__((__packed__))
{
    uint8_t     id_device_reception;
    uint8_t     msg_op_reception;
    uint16_t    len_reception;

}msg_reception_t;

/* ------------ CREATE STATIC MEMORY FOR MSG MQTT ------------ */
static msg_data_t        msg_data[N_MSG_DATA];

#endif /* MSG_STRUCTS_H_ */
