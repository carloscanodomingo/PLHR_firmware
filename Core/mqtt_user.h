
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
*  mqtt_user.h
*/

#ifndef MQTT_USER_H_
#define MQTT_USER_H_

#include <ti/net/mqtt/mqttclient.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include <ti/drivers/net/wifi/netcfg.h>
#include <stdio.h>
#include "msg_structs.h"

#define MQTTCLIENT_MAX_SIMULTANEOUS_SUB_TOPICS 4

// MQTT events for event callback
enum
{
    MQTT_EVENT_CONNACK,
    MQTT_EVENT_SUBACK,
    MQTT_EVENT_PUBACK,
    MQTT_EVENT_UNSUBACK,
    MQTT_EVENT_SERVER_DISCONNECT,
    MQTT_EVENT_CLIENT_DISCONNECT,
    MQTT_EVENT_DESTROY,
    MQTT_EVENT_MAX
};

enum{
    TYPE_CMD_DEFAULT_ID = 0,
    TYPE_DATA_DEFAULT_ID,
    TYPE_CMD,
    TYPE_DATA
};

typedef void (*MQTT_IF_TopicCallback_f)(char* topic, char* payload, uint8_t qos);
typedef void (*MQTT_IF_EventCallback_f)(int32_t event);

typedef struct MQTT_IF_ClientParams
{
    char                *clientID;
    char                *username;
    char                *password;
    uint16_t            keepaliveTime;
    bool                cleanConnect;
    bool                mqttMode31;     // false 3.1.1 (default) : true 3.1
    bool                blockingSend;
    MQTTClient_Will     *willParams;
} MQTT_IF_ClientParams_t;

typedef struct MQTT_IF_initParams
{
    unsigned int stackSize;
    uint8_t threadPriority;
} MQTT_IF_InitParams_t;

typedef struct __attribute__((__packed__))
{
    uint64_t            *ntp_time;
    tBoolean            *time_sync;
    int                 *mqtt_connected;
    int                 *send_data_status;
    SemaphoreP_Handle   send_data_semaphore;
    msg_data_t          *msg_data;

}arg_thread_send_data_t;

uint16_t            id_device;
int                 mqtt_connected;
char                client_id[13];
static const char   topic_1[]               = "CC32xx/cmd/s2d/0";
//static const char   topic_cmd_pub[]         = "CC32xx/cmd/d2s/255";
//static char         topic_data_pub[]        = "CC32xx/data/";
//static char         topic_cmd_pub_device[]  = "CC32xx/cmd/d2s/";

int                 MQTT_IF_Init                    (MQTT_IF_InitParams_t initParams);
int                 MQTT_IF_Deinit                  (MQTTClient_Handle mqtt_client_handle);
MQTTClient_Handle   MQTT_IF_Connect                 (MQTT_IF_ClientParams_t mqttClientParams, MQTTClient_ConnParams mqttConnParams, MQTT_IF_EventCallback_f mqttCB);
int                 MQTT_IF_Disconnect              (MQTTClient_Handle mqttClientHandle);
int                 MQTT_IF_Subscribe               (MQTTClient_Handle mqttClientHandle, const char* topic, unsigned int qos, MQTT_IF_TopicCallback_f topicCB);
int                 MQTT_IF_Unsubscribe             (MQTTClient_Handle mqttClientHandle, char* topic);
int                 MQTT_IF_Publish                 (MQTTClient_Handle mqttClientHandle, const char* topic, char* payload, unsigned short payloadLen, int flags);
int                 test_mqtt_connection            (void);
void*               send_data                       (void *arg_send_data_thread);
int                 mqtt_init                       (void);
int                 mqtt_connect                    (void);
void                mqtt_event_callback             (int32_t event);
void                cmd_zero                        (char* topic, char* p, uint8_t qos);
int32_t             SetClientIdNamefromMacAddress   (void);
int                 mqtt_client_disconnect          (void);
int                 mqtt_client_connect             (void);
int                 mqtt_subscribe                  (void);
int                 mqtt_disconnect                 (void);
void                watchdog_callback               (void);
int                 mqtt_publish                    (uint16_t type_publish, char* payload, unsigned short len);
int                 send_rssi                       (void);
int                 ask_for_id                      (void);

#endif /* MQTT_USER_H_ */
