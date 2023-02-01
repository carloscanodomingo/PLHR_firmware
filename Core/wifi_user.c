
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
*           wifi_user.c
*/
#include "wifi_user.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <mqueue.h>
#include <unistd.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include "uart_term.h"
#include <ti/display/Display.h>
#include "global_def.h"
#include "mqtt_user.h"

Display_Handle  display;
struct timespec absolute_time_conn;
struct timespec absolute_time_dconn;

typedef enum
{
    /* Choosing this number to avoid overlap with host-driver's error codes. */
    DEVICE_NOT_IN_STATION_MODE= -0x7F0,
    DEVICE_NOT_IN_AP_MODE = DEVICE_NOT_IN_STATION_MODE - 1,
    DEVICE_NOT_IN_P2P_MODE = DEVICE_NOT_IN_AP_MODE - 1,

    STATUS_CODE_MAX = -0xBB8
} e_NetAppStatusCodes;

//*****************************************************************************
//                 GLOBAL VARIABLES
//*****************************************************************************

/* Station IP address                                                        */
unsigned long g_ulStaIp = 0;
/* Network Gateway IP address                                                */
unsigned long g_ulGatewayIP = 0;
/* Connection SSID                                                           */
unsigned char g_ucConnectionSSID[SL_WLAN_SSID_MAX_LENGTH + 1];
/* Connection BSSID                                                          */
unsigned char g_ucConnectionBSSID[SL_WLAN_BSSID_LENGTH ];
/* SimpleLink Status                                                         */
volatile unsigned long g_ulStatus = 0;
/* Connection time delay index                                               */
volatile unsigned short g_usConnectIndex;


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************
void SimpleLinkHttpServerEventHandler(
    SlNetAppHttpServerEvent_t *pSlHttpServerEvent,
    SlNetAppHttpServerResponse_t *
    pSlHttpServerResponse)
{
}

void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
                                         SlNetAppResponse_t *pNetAppResponse)
{
}

void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{
}

//*****************************************************************************
//!
//! On Successful completion of Wlan Connect, This function triggers connection
//! status to be set.
//!
//! \param[in]  pSlWlanEvent    - pointer indicating Event type
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    SlWlanEventDisconnect_t* pEventData = NULL;

    switch(pSlWlanEvent->Id)
    {
    case SL_WLAN_EVENT_CONNECT:
        GPIO_write(CONFIG_LED_RED, CONFIG_GPIO_LED_ON);
        SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);

        memcpy(g_ucConnectionSSID, pSlWlanEvent->Data.Connect.SsidName,
               pSlWlanEvent->Data.Connect.SsidLen);
        memcpy(g_ucConnectionBSSID, pSlWlanEvent->Data.Connect.Bssid,
               SL_WLAN_BSSID_LENGTH);

        UART_PRINT(
            "[WLAN EVENT] STA Connected to the AP: %s , BSSID: "
            "%x:%x:%x:%x:%x:%x\n\r", g_ucConnectionSSID,
            g_ucConnectionBSSID[0], g_ucConnectionBSSID[1],
            g_ucConnectionBSSID[2],
            g_ucConnectionBSSID[3], g_ucConnectionBSSID[4],
            g_ucConnectionBSSID[5]);
        break;

    case SL_WLAN_EVENT_DISCONNECT:
        GPIO_write(CONFIG_LED_RED, CONFIG_GPIO_LED_OFF);
        CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
        CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_ACQUIRED);

        pEventData = &pSlWlanEvent->Data.Disconnect;

        /* If the user has initiated 'Disconnect' request, 'reason_code' */
        /* is SL_WLAN_DISCONNECT_USER_INITIATED                          */
        if(SL_WLAN_DISCONNECT_USER_INITIATED == pEventData->ReasonCode)
        {
            UART_PRINT("Device disconnected from the AP on application's request \n\r");
        }
        else
        {
            UART_PRINT("WARNING:\tConnection to AP lost! \n\r");

        }

        break;

    case SL_WLAN_EVENT_STA_ADDED:
        /* when device is in AP mode and any client connects to it.      */
        SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
        break;

    case SL_WLAN_EVENT_STA_REMOVED:
        /* when device is in AP mode and any client disconnects from it. */
        CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
        CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);
        break;

    default:
        UART_PRINT("[WLAN EVENT] Unexpected event %d\n\r", pSlWlanEvent->Id);
        break;
    }
}

//*****************************************************************************
//
//! The Function Handles the Fatal errors
//!
//! \param[in]  slFatalErrorEvent - Pointer to Fatal Error Event info
//!
//! \return     None
//!
//*****************************************************************************
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
    switch(slFatalErrorEvent->Id)
    {
    case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
        UART_PRINT(
            "[ERROR] - FATAL ERROR: Abort NWP event detected: "
            "AbortType=%d, AbortData=0x%x\n\r",
            slFatalErrorEvent->Data.DeviceAssert.Code,
            slFatalErrorEvent->Data.DeviceAssert.Value);
        break;

    case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
        UART_PRINT("[ERROR] - FATAL ERROR: Driver Abort detected. \n\r");
        break;

    case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
        UART_PRINT(
            "[ERROR] - FATAL ERROR: No Cmd Ack detected "
            "[cmd opcode = 0x%x] \n\r",
            slFatalErrorEvent->Data.NoCmdAck.Code);
        break;

    case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
        UART_PRINT("[ERROR] - FATAL ERROR: Sync loss detected n\r");
        break;

    case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
        UART_PRINT(
            "[ERROR] - FATAL ERROR: Async event timeout detected "
            "[event opcode =0x%x]  \n\r",
            slFatalErrorEvent->Data.CmdTimeout.Code);
        break;

    default:
        UART_PRINT("[ERROR] - FATAL ERROR: Unspecified error detected \n\r");
        break;
    }
}

//*****************************************************************************
//
//! This function handles network events such as IP acquisition, IP leased, IP
//! released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    switch(pNetAppEvent->Id)
    {
    case SL_NETAPP_EVENT_IPV4_ACQUIRED:
    case SL_NETAPP_EVENT_IPV6_ACQUIRED:
        SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_ACQUIRED);
        UART_PRINT("[NETAPP EVENT] IP acquired by the device\n\r");
        break;

    case SL_NETAPP_EVENT_DHCPV4_LEASED:
        SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);
        g_ulStaIp = (pNetAppEvent)->Data.IpLeased.IpAddress;

        UART_PRINT("[NETAPP EVENT] IP Leased to Client: IP=%d.%d.%d.%d , ",
                   SL_IPV4_BYTE(g_ulStaIp,
                                3),
                   SL_IPV4_BYTE(g_ulStaIp,
                                2),
                   SL_IPV4_BYTE(g_ulStaIp, 1), SL_IPV4_BYTE(g_ulStaIp, 0));
        break;

    case SL_NETAPP_EVENT_DHCPV4_RELEASED:
        CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

        UART_PRINT("[NETAPP EVENT] IP Released for Client: "
                   "IP=%d.%d.%d.%d , ", SL_IPV4_BYTE(g_ulStaIp,
                                                     3),
                   SL_IPV4_BYTE(g_ulStaIp,
                                2),
                   SL_IPV4_BYTE(g_ulStaIp, 1), SL_IPV4_BYTE(g_ulStaIp, 0));
        break;

    default:
        UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                   pNetAppEvent->Id);
        break;
    }
}

//*****************************************************************************
//
//! This function handles resource request
//!
//! \param[in]  pNetAppRequest  - Contains the resource requests
//! \param[in]  pNetAppResponse - Should be filled by the user with the
//!                               relevant response information
//!
//! \return     None
//!
//*****************************************************************************
void SimpleLinkNetAppRequestHandler(SlNetAppRequest_t *pNetAppRequest,
                                    SlNetAppResponse_t *pNetAppResponse)
{
    /* Unused in this application                                            */
}

//*****************************************************************************
//
//! This function handles HTTP server events
//!
//! \param[in]  pServerEvent     - Contains the relevant event information
//! \param[in]  pServerResponse  - Should be filled by the user with the
//!                                relevant response information
//!
//! \return None
//!
//****************************************************************************
void SimpleLinkHttpServerCallback(SlNetAppHttpServerEvent_t *pHttpEvent,
                                  SlNetAppHttpServerResponse_t *pHttpResponse)
{
    /* Unused in this application                                            */
}

//*****************************************************************************
//
//! This function handles General Events
//!
//! \param[in]  pDevEvent - Pointer to General Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    /* Most of the general errors are not FATAL. are to be handled           */
    /* appropriately by the application.                                     */
    UART_PRINT("[GENERAL EVENT] - ID=[%d] Sender=[%d]\n\n",
               pDevEvent->Data.Error.Code,
               pDevEvent->Data.Error.Source);
}

//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]  pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    /* This application doesn't work w/ socket - Events are not expected     */
    switch(pSock->Event)
    {
    case SL_SOCKET_TX_FAILED_EVENT:
        switch(pSock->SocketAsyncEvent.SockTxFailData.Status)
        {
        case SL_ERROR_BSD_ECLOSE:
            UART_PRINT(
                "[SOCK ERROR] - close socket (%d) operation "
                "failed to transmit all queued packets\n\r",
                pSock->SocketAsyncEvent.SockTxFailData.Sd);
            break;
        default:
            UART_PRINT(
                "[SOCK ERROR] - TX FAILED  :  socket %d , "
                "reason (%d) \n\n",
                pSock->SocketAsyncEvent.SockTxFailData.Sd,
                pSock->SocketAsyncEvent.SockTxFailData.Status);
            break;
        }
        break;
    case SL_SOCKET_ASYNC_EVENT:
    {
        UART_PRINT("[SOCK ERROR] an event received on socket %d\r\n",
                   pSock->SocketAsyncEvent.SockAsyncData.Sd);
        switch(pSock->SocketAsyncEvent.SockAsyncData.Type)
        {
        case SL_SSL_NOTIFICATION_CONNECTED_SECURED:
            UART_PRINT("[SOCK ERROR] SSL handshake done");
            break;
        case SL_SSL_NOTIFICATION_HANDSHAKE_FAILED:
            UART_PRINT("[SOCK ERROR] SSL handshake failed with error %d\r\n",
                       pSock->SocketAsyncEvent.SockAsyncData.Val);
            break;
        case SL_SSL_ACCEPT:
            UART_PRINT(
                "[SOCK ERROR] Recoverable error occurred "
                "during the handshake %d\r\n",
                pSock->SocketAsyncEvent.SockAsyncData.Val);
            break;
        case SL_OTHER_SIDE_CLOSE_SSL_DATA_NOT_ENCRYPTED:
            UART_PRINT("[SOCK ERROR] Other peer terminated the SSL layer.\r\n");
            break;
        case SL_SSL_NOTIFICATION_WRONG_ROOT_CA:
            UART_PRINT("[SOCK ERROR] Used wrong CA to verify the peer.\r\n");
            break;
        default:
            break;
        }
        break;
    }
    default:
        UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n", pSock->Event);
        break;
    }
}

void Network_IF_setDisplay(void * arg_display)
{

    display = (Display_Handle) arg_display;
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************

//*****************************************************************************
//
//! This function initializes the application variables
//!
//! \param  None
//!
//! \return 0 on success, negative error-code on error
//
//*****************************************************************************
void InitializeAppVariables(void)
{
    g_ulStatus = 0;
    g_ulStaIp = 0;
    g_ulGatewayIP = 0;

    memset(g_ucConnectionSSID, 0, sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID, 0, sizeof(g_ucConnectionBSSID));
}

//*****************************************************************************
//
//! The function initializes a CC32xx device and triggers it to start operation
//!
//! \param[in]  uiMode  - device mode in which device will be configured
//!
//! \return 0 : success, -ve : failure
//
//*****************************************************************************
long Network_IF_InitDriver(uint32_t uiMode)
{
    sl_RegisterEventHandler(SL_EVENT_HDL_WLAN , (void *)SimpleLinkWlanEventHandler);

    long lRetVal = -1;

    /* Reset CC32xx Network State Machine                                    */
    InitializeAppVariables();

    /* Following function configure the device to default state by cleaning  */
    /* the persistent settings stored in NVMEM (viz. connection profiles     */
    /* & policies, power policy etc) Applications may choose to skip this    */
    /* step if the developer is sure that the device is in its default state */
    /* at start of application. Note that all profiles and persistent        */
    /* settings that were done on the device will be lost.                   */
    lRetVal = sl_Start(NULL, NULL, NULL);

    if(lRetVal < 0)
    {
        UART_PRINT("Failed to start the device \n\r");
    }

    switch(lRetVal)
    {
    case ROLE_STA:
        UART_PRINT("Device configured in STATION MODE\n\r");
        break;
    case ROLE_AP:
        UART_PRINT("Device came up in Access-Point mode\n\r");
        break;
    case ROLE_P2P:
        UART_PRINT("Device came up in P2P mode\n\r");
        break;
    default:
        ERR_PRINT("Error:unknown mode\n\r");
        break;
    }

    if(uiMode != lRetVal)
    {
        UART_PRINT("Switching Networking mode on application request\n\r");

        /* Switch to AP role and restart                                     */
        lRetVal = sl_WlanSetMode(uiMode);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(SL_STOP_TIMEOUT);
        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        if(lRetVal == uiMode)
        {
            switch(lRetVal)
            {
            case ROLE_STA:
                UART_PRINT("Device came up in Station mode\n\r");
                break;
            case ROLE_AP:
                UART_PRINT("Device came up in Access-Point mode\n\r");
                /* If the device is in AP mode, we need to wait for this */
                /* event before doing anything.                          */
                while(!IS_IP_ACQUIRED(g_ulStatus))
                {
                    usleep(1000);
                }
                break;
            case ROLE_P2P:
                UART_PRINT("Device came up in P2P mode\n\r");
                break;
            default:
                UART_PRINT("Error:unknown mode\n\r");
                break;
            }
        }
        else
        {
            UART_PRINT("could not configure correct networking mode\n\r");
            LOOP_FOREVER();
        }
    }
    else
    {
        if(lRetVal == ROLE_AP)
        {
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
                usleep(1000);
            }
        }
    }
    return(0);
}

//*****************************************************************************
//
//! The function de-initializes a CC32xx device
//!
//! \param  None
//!
//! \return On success, zero is returned. On error, other
//
//*****************************************************************************
long Network_IF_DeInitDriver(void)
{
    long lRetVal = -1;

    UART_PRINT("SL Disconnect...\n\r");

    /* Disconnect from the AP                                                */
    lRetVal = Network_IF_DisconnectFromAP();

    /* Stop the simplelink host                                              */
    lRetVal = sl_Stop(SL_STOP_TIMEOUT);

    /* Reset the state to uninitialized                                      */
    Network_IF_ResetMCUStateMachine();
    return(lRetVal);
}

//*****************************************************************************
//
//! Connect to an Access Point using the specified SSID
//!
//! \param[in]  pcSsid          - is a string of the AP's SSID
//! \param[in]  SecurityParams  - is Security parameter for AP
//!
//! \return On success, zero is returned. On error, -ve value is returned
//
//*****************************************************************************
long Network_IF_ConnectAP(char *pcSsid,
                          SlWlanSecParams_t SecurityParams,
                          SlWlanSecParamsExt_t eap_params)
{
    long lRetVal;
    unsigned long ulIP = 0;
    unsigned long ulSubMask = 0;
    unsigned long ulDefGateway = 0;
    unsigned long ulDns = 0;

    g_usConnectIndex = 0;

    /* Disconnect from the AP                                                */
    Network_IF_DisconnectFromAP();

    CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
    CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_ACQUIRED);

    /* Continue only if SSID is not empty                                    */
    if(pcSsid != NULL)
    {
        /* This triggers the CC32xx to connect to a specific AP.             */
        lRetVal =
            sl_WlanConnect((signed char *) pcSsid, strlen((const char *) pcSsid),
                           NULL, &SecurityParams, &eap_params);
        ASSERT_ON_ERROR(lRetVal);

        /* Wait forever to check if connection to desire AP succeeds     */
        GPIO_write(CONFIG_LED_RED, CONFIG_GPIO_LED_ON);
        while(1)
        {
            GPIO_toggle(CONFIG_LED_RED);
            sleep(1);

            if(IS_CONNECTED(g_ulStatus) && IS_IP_ACQUIRED(g_ulStatus))
            {
                GPIO_write(CONFIG_LED_RED, CONFIG_GPIO_LED_ON);
                break;
            }
        }
    }
    else
    {
        UART_PRINT("Empty SSID, Could not connect\n\r");
        return(-1);
    }

    UART_PRINT("\n\rDevice has connected to %s\n\r", pcSsid);

    /* Get IP address                                                        */
    lRetVal = Network_IF_IpConfigGet(&ulIP, &ulSubMask, &ulDefGateway, &ulDns);
    ASSERT_ON_ERROR(lRetVal);

    /* Send the information                                                  */
    UART_PRINT("Device IP Address is %d.%d.%d.%d \n\r\n\r",
               SL_IPV4_BYTE(ulIP, 3), SL_IPV4_BYTE(ulIP, 2), SL_IPV4_BYTE(ulIP,
                                                                          1),
               SL_IPV4_BYTE(ulIP, 0));
    return(0);
}


int wifi_init(void)
{
    int32_t ret;

    pthread_t spawn_thread = (pthread_t) NULL;
    pthread_attr_t pattrs_spawn;
    struct sched_param pri_param;

    pthread_attr_init(&pattrs_spawn);
    pri_param.sched_priority = NETWORK_TASK_PRIORITY;
    ret = pthread_attr_setschedparam(&pattrs_spawn, &pri_param);
    ret |= pthread_attr_setstacksize(&pattrs_spawn, SL_TASKSTACKSIZE);
    ret |= pthread_attr_setdetachstate(&pattrs_spawn, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&spawn_thread, &pattrs_spawn, sl_Task, NULL);
    if(ret != 0)
    {
        LOG_ERROR("could not create simplelink task\n\r");
    }

    Network_IF_ResetMCUStateMachine();

    Network_IF_DeInitDriver();

    ret = Network_IF_InitDriver(ROLE_STA);
    if(ret < 0)
    {
        LOG_ERROR("Failed to start SimpleLink Device\n\r");
    }

    SetClientIdNamefromMacAddress();

    return ret;
}

int wifi_connect(void)
{

    SlWlanSecParams_t security_params;
    SlWlanSecParamsExt_t eap_params;

    int ret = 0;
    uint8_t wlan_set_param  = 0;  // 1 means disable the server authentication

    security_params.Type    = SECURITY_TYPE;
    security_params.Key     = SECURITY_KEY;
    security_params.KeyLen  = strlen(SECURITY_KEY);

    eap_params.User         = USERNAME;
    eap_params.UserLen      = strlen(USERNAME);
    eap_params.EapMethod    = SECURITY_ENT;
    eap_params.AnonUser     = ANONYMOUS;
    eap_params.AnonUserLen  = NULL;

    ret = sl_WlanProfileDel(SL_WLAN_DEL_ALL_PROFILES);
    if(ret < 0)
    {
      LOG_ERROR("Failed to delete Wlan profiles!\n\r");
    }

    ret = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, SL_WLAN_GENERAL_PARAM_DISABLE_ENT_SERVER_AUTH, 1, &wlan_set_param);
    if (ret<0)
    {
      LOG_ERROR("WlanSet error!");
    }

    ret = Network_IF_ConnectAP(SSID_NAME, security_params, eap_params);
    if(ret < 0)
    {
      LOG_ERROR("ERROR: connection to an AP failed!\n\r");
    }

    ret = sl_WlanProfileAdd(SSID_NAME, strlen(SSID_NAME), 0, &security_params, &eap_params, ENT_CONECTION_PRIORITY, NULL);
    if(ret < 0)
    {
      LOG_ERROR("Add profile failed!\n\r");
    }

    ret = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(0,0,0,0),NULL, 0);
    if (ret<0)
    {
        LOG_ERROR("ERROR: %d\tWlan policy error!!", ret);
    }

    ret = sl_WlanPolicySet(SL_WLAN_POLICY_PM , SL_WLAN_ALWAYS_ON_POLICY, NULL,0);
    if (ret<0)
    {
      LOG_ERROR("ERROR: %d\tWlan policy error!!", ret);
    }

    ret = sl_DeviceStatStart(NULL);
    CHECK_ERROR (ret, "Device stats start failed");

    return ret;
}

//*****************************************************************************
//
//! Disconnects from an Access Point
//!
//! \param  none
//!
//! \return 0 disconnected done, other already disconnected
//
//*****************************************************************************
long Network_IF_DisconnectFromAP(void)
{
    long lRetVal = 0;

    if(IS_CONNECTED(g_ulStatus))
    {
        lRetVal = sl_WlanDisconnect();
        if(0 == lRetVal)
        {
            while(IS_CONNECTED(g_ulStatus))
            {
                usleep(1000);
            }
            return(lRetVal);
        }
        else
        {
            return(lRetVal);
        }
    }
    else
    {
        return(lRetVal);
    }
}

//*****************************************************************************
//
//! Get the IP Address of the device.
//!
//! \param[in]  pulIP               - IP Address of Device
//! \param[in]  pulSubnetMask       - Subnetmask of Device
//! \param[in]  pulDefaultGateway   - Default Gateway value
//! \param[in]  pulDNSServer        - DNS Server
//!
//! \return On success, zero is returned. On error, -1 is returned
//
//*****************************************************************************
long Network_IF_IpConfigGet(unsigned long *pulIP,
                            unsigned long *pulSubnetMask,
                            unsigned long *pulDefaultGateway,
                            unsigned long *pulDNSServer)
{
    unsigned short usDHCP = 0;
    long lRetVal = -1;
    unsigned short len = sizeof(SlNetCfgIpV4Args_t);
    SlNetCfgIpV4Args_t ipV4 = { 0 };

    /* get network configuration                                             */
    lRetVal =
        sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &usDHCP, &len,
                     (unsigned char *) &ipV4);
    ASSERT_ON_ERROR(lRetVal);

    *pulIP = ipV4.Ip;
    *pulSubnetMask = ipV4.IpMask;
    *pulDefaultGateway = ipV4.IpGateway;
    *pulDefaultGateway = ipV4.IpDnsServer;

    return(lRetVal);
}

//*****************************************************************************
//
//!  This function obtains the server IP address using a DNS lookup
//!
//! \param[in]  pcHostName        The server hostname
//! \param[out] pDestinationIP    This parameter is filled with host IP address
//!
//! \return On success, +ve value is returned. On error, -ve value is returned
//
//*****************************************************************************
long Network_IF_GetHostIP(char* pcHostName,
                          unsigned long * pDestinationIP)
{
    long lStatus = 0;

    lStatus =
        sl_NetAppDnsGetHostByName((signed char *) pcHostName, strlen(
                                      pcHostName), pDestinationIP, SL_AF_INET);
    ASSERT_ON_ERROR(lStatus);

    UART_PRINT("Get Host IP succeeded.\n\rHost: %s IP: %d.%d.%d.%d \n\r\n\r",
               pcHostName, SL_IPV4_BYTE(*pDestinationIP,
                                        3), SL_IPV4_BYTE(*pDestinationIP,
                                                         2),
               SL_IPV4_BYTE(*pDestinationIP, 1),
               SL_IPV4_BYTE(*pDestinationIP, 0));

    return(lStatus);
}

//*****************************************************************************
//
//! Reset state from the state machine
//!
//! \param  None
//!
//! \return none
//!
//*****************************************************************************
void Network_IF_ResetMCUStateMachine()
{
    g_ulStatus = 0;
}

//*****************************************************************************
//
//! Return the current state bits
//!
//! \param  None
//!
//! \return none
//!
//
//*****************************************************************************
unsigned long Network_IF_CurrentMCUState()
{
    return(g_ulStatus);
}

//*****************************************************************************
//
//! Sets a state from the state machine
//!
//! \param[in]  cStat   - Status of State Machine defined in e_StatusBits
//!
//! \return none
//!
//*****************************************************************************
void Network_IF_SetMCUMachineState(char cStat)
{
    SET_STATUS_BIT(g_ulStatus, cStat);
}

//*****************************************************************************
//
//! Unsets a state from the state machine
//!
//! \param[in]  cStat   - Status of State Machine defined in e_StatusBits
//!
//! \return none
//!
//*****************************************************************************
void Network_IF_UnsetMCUMachineState(char cStat)
{
    CLR_STATUS_BIT(g_ulStatus, cStat);
}

int test_wifi_connection (void)
{
    int32_t ret;

    if (IS_CONNECTED(g_ulStatus))
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}


int wifi_deinit(void)
{
    int32_t ret;
    Network_IF_ResetMCUStateMachine();
    ret = Network_IF_DeInitDriver();
    CHECK_ERROR(ret, "Sl stop in wifi deinit function");

    return ret;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
