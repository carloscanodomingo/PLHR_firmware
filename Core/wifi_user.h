
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
*           wifi_user.h
*/


#ifndef WIFI_USER_H_
#define WIFI_USER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

#define SSID_NAME               "eduroam"
#define SECURITY_TYPE           SL_WLAN_SEC_TYPE_WPA_ENT
#define SECURITY_KEY            "plhrCC32"
#define SECURITY_ENT            SL_WLAN_ENT_EAP_METHOD_TTLS_MSCHAPv2
#define USERNAME                "tic019CC"
#define ANONYMOUS               NULL
#define ENT_CONECTION_PRIORITY  15
#define SL_STOP_TIMEOUT         (200)
#define SL_TASKSTACKSIZE        (2048)
#define NETWORK_TASK_PRIORITY   (15)
#define CLR_STATUS_BIT_ALL(status_variable)  (status_variable = 0)
#define SET_STATUS_BIT(status_variable, bit) (status_variable |= (1 << (bit)))
#define CLR_STATUS_BIT(status_variable, bit) (status_variable &= ~(1 << (bit)))
#define CLR_STATUS_BIT_ALL(status_variable)  (status_variable = 0)
#define GET_STATUS_BIT(status_variable, bit) (0 != \
                                              (status_variable & (1 << (bit))))
#define IS_CONNECTED(status_variable)        GET_STATUS_BIT( \
        status_variable, \
        STATUS_BIT_CONNECTION)
#define IS_IP_ACQUIRED(status_variable)      GET_STATUS_BIT( \
        status_variable, \
        STATUS_BIT_IP_ACQUIRED)
#define WIFI_NO_ERROR                       (0)
#define LOOP_FOREVER() \
    { \
        while(1) {; } \
    }
#define ASSERT_ON_ERROR(error_code) \
    { \
        if(error_code < 0) \
        { \
            ERR_PRINT("ERROR!"); \
            return error_code; \
        } \
    }

long            Network_IF_InitDriver           (uint32_t uiMode);
long             Network_IF_DeInitDriver        (void);
long             Network_IF_ConnectAP           (char * pcSsid,
                                                     SlWlanSecParams_t SecurityParams,
                                                     SlWlanSecParamsExt_t eap_params);
long             Network_IF_DisconnectFromAP();
long             Network_IF_IpConfigGet         (unsigned long *aucIP,
                                                     unsigned long *aucSubnetMask,
                                                     unsigned long *aucDefaultGateway,
                                                     unsigned long *aucDNSServer);
long            Network_IF_GetHostIP            (char* pcHostName,
                                                     unsigned long * pDestinationIP);
unsigned long   Network_IF_CurrentMCUState      (void);
void            Network_IF_UnsetMCUMachineState (char stat);
void            Network_IF_SetMCUMachineState   (char stat);
void            Network_IF_ResetMCUStateMachine (void);
void            Network_IF_setDisplay           (void * arg_display);
int             wifi_init                       (void);
int             wifi_connect                    (void);
int             test_wifi_connection            (void);
int             wifi_deinit                     (void);

/* Status bits - used to set/reset the corresponding bits in given a variable */
typedef enum
{
    /* If this bit is set: Network Processor is powered up                    */
    STATUS_BIT_NWP_INIT = 0,

    /* If this bit is set: the device is connected to the AP or client is     */
    /* connected to device (AP)                                               */
    STATUS_BIT_CONNECTION,

    /* If this bit is set: the device has leased IP to any connected client.  */
    STATUS_BIT_IP_LEASED,

    /* If this bit is set: the device has acquired an IP                      */
    STATUS_BIT_IP_ACQUIRED,

    /* If this bit is set: the SmartConfiguration process is started from     */
    /* SmartConfig app                                                        */
    STATUS_BIT_SMARTCONFIG_START,

    /* If this bit is set: the device (P2P mode) found any p2p-device in scan */
    STATUS_BIT_P2P_DEV_FOUND,

    /* If this bit is set: the device (P2P mode) found any p2p-negotiation    */
    /* request                                                                */
    STATUS_BIT_P2P_REQ_RECEIVED,

    /* If this bit is set: the device(P2P mode) connection to client (or      */
    /* reverse way) is failed                                                 */
    STATUS_BIT_CONNECTION_FAILED,

    /* If this bit is set: the device has completed the ping operation        */
    STATUS_BIT_PING_DONE,

    /* If this bit is set: the device has acquired an IPv6 address.           */
    STATUS_BIT_IPV6L_ACQUIRED,

    /* If this bit is set: the device has acquired an IPv6 address.           */
    STATUS_BIT_IPV6G_ACQUIRED,
    STATUS_BIT_AUTHENTICATION_FAILED,
    STATUS_BIT_RESET_REQUIRED,
}e_StatusBits;

#ifdef __cplusplus
}
#endif

#endif // __WIFI_USER__H__
