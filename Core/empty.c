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
*  empty.c
*/
/* For usleep() */
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mqueue.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/SPI.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include "rom_map.h"
#include <interrupt.h>
#include <pin.h>
#include <prcm.h>
#include "udma_user.h"
#include "adc_user.h"
#include "gpio_user.h"
#include "timer_user.h"
#include "interrupt_user.h"
#include "print_user.h"
#include "dws_user.h"
#include "wifi_user.h" //
#include "test_wifi_user.h"
#include "source/ti/drivers/dpl/HwiP.h"
#include "hw_memmap.h"
#include <stdio.h>
#include <string.h>
#include <ti/display/Display.h>
#include "source/ti/drivers/dpl/ClockP.h"
#include <pthread.h>
#include <ti/drivers/dpl/SemaphoreP.h>
#include "ti_drivers_config.h"
#include <ti/drivers/GPIO.h>
#include "uart_term.h"
#include "msg_structs.h"
#include <mqtt_user.h>
#include "global_def.h"
#include <ti/display/Display.h>
#include <source/ti/net/sntp/sntp.h>
#include <ti/net/slnetsock.h>
#include <ti/net/slnetutils.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>
#include "source\ti\net\bsd\arpa\inet.h"
#include <ti/drivers/Watchdog.h>

///////////////////////////////////////////////////
///////////////// SYSTEM DEFINE ///////////////////
///////////////////////////////////////////////////

#define USE_TIRTOS
#define SEMAPHORE_INIT_COUNT            (0)
#define SOFTTIMER_THREAD_PRIORITY       (5)
#define TEST_WIFI_THREAD_PRIORITY       (5)
#define SEND_CMD_THREAD_PRIORITY        (5)
#define DWS_STACK_SIZE                  (1024)
#define SOFTTIMER_STACK_SIZE            (1024)
#define TEST_WIFI_STACK_SIZE            (1024)
#define SEND_DATA_STACK_SIZE            (1024 * 5)
#define SEND_CMD_STACK_SIZE             (1024)
#define ADC_CHANNEL_ID                  ADC_CH_2
#define INT_ADC                         INT_ADCCH2
#define DMA_NUM_BUFFERS                 (30)
#define DMA_BUFF_SIZE                   (800)
#define DMA_BUFF_NEXT                   (2)
#define DMA_BUFF_OF                     (2^16)
#define DWS_MAX_BUFFER                  (78)
#define DWS_BUFFER_SIZE                 ((DMA_BUFF_SIZE/DWS_RATE) * (2/3))
#define DWS_SIZE                        (DWS_BUFFER_SIZE * DWS_MAX_BUFFER)
#define SNTP_PORT                       (123)
#define USECONDS_TO_SOFTTIMER_CB        (1000000)
#define SECONDS_TO_NTP_SYNC             (1800)
#define SECONDS_TO_SEND_RSSI            (4)

///////////////////////////////////////////////////
/////////////// FUNCTIONS HEADERS /////////////////
///////////////////////////////////////////////////

void            isr_dma_done                    (void);
void*           softtimer                       (void *arg_sofftimer_thread);
extern int32_t  ti_net_SlNet_initConfig         (void);
uint64_t        sntp_get_time                   (void);
void            softtimer_callback              (Timer_Handle handle, int_fast16_t status);

///////////////////////////////////////////////////
///////////////// GLOBAL VARIABLES ////////////////
///////////////////////////////////////////////////

volatile uint32_t           dma_cnt_interrupts;
volatile tBoolean           dma_finished  = false;
dma_buffer_t                dma_buffer[DMA_NUM_BUFFERS];
uint16_t                    cnt_dma_write = 0;
dma_stop_mode_t             dma_stop_mode;
static SemaphoreP_Handle    dws_semaphore;
static SemaphoreP_Handle    send_data_semaphore;
static SemaphoreP_Handle    softtimer_thread_semaphore;
static SemaphoreP_Handle    sync_semaphore;
volatile tBoolean           st_program = false;
static uint8_t              dws_buffer[DWS_SIZE];
mqd_t                       appQueue;
static SemaphoreP_Params    dws_semaphore_params, send_data_semaphore_params;
static SemaphoreP_Params    softtimer_thread_semaphore_params, sync_semaphore_params;
//00static msg_data_t           msg_data[N_MSG_DATA];
struct timespec             rtc_timer;
extern int                  inet_addr(const char *str);
uint64_t                    sntp_time;
uint64_t                    status_time_sync;
int                         send_data_status;

typedef struct __attribute__((__packed__))
{
    uint64_t    *ntp_time;
    uint64_t    *time_sync_status;

}arg_softtimer_thread_t;

///////////////////////////////////////////////////
///////////INSTALL SDK 4.4//////////////////
///////////////////////////////////////////////////

///////////////////////////////////////////////////
///////////////////// MAIN START //////////////////
///////////////////////////////////////////////////

void *mainThread(void *arg0)
{
   pthread_t            dws_thread, snd_mqqt_thread, softtimer_thread, test_wifi_thread;
   pthread_attr_t       dws_thread_attrs, send_data_thread_attrs, softtimer_thread_attrs, test_wifi_thread_attrs;
   struct sched_param   dws_thread_pri_param, send_data_thread_pri_params, softtimer_thread_pri_params, test_wifi_thread_pri_params;
   int                  dws_thread_detach_state, send_data_thread_detach_state, softtimer_thread_detach_state, test_wifi_thread_detach_state;
   int32_t              ret;
   uint16_t             mac_addr_size = SIZE_MAC_ADDR;
   tBoolean             time_sync = false;
   uint64_t             ntp_time = 0;

   mqtt_connected       = 0;
   status_time_sync     = 0;
   send_data_status     = stop;

   //------------------------------- Device init -------------------------------//
   SPI_init();
   gpio_open();
   Watchdog_init();
   SlNetIf_init(0);
   SlNetIf_add(SLNETIF_ID_1, "CC3220", (const SlNetIf_Config_t *)&SlNetIfConfigWifi, 5);
   SlNetSock_init(0);
   SlNetUtil_init(0);
   Display_init();
   display = Display_open(Display_Type_UART, NULL);
   LOG_INFO("______PROGRAM START______\n");

   //------------------------ DWS Semaphore creation --------------------------//

   SemaphoreP_Params_init(&dws_semaphore_params);
   dws_semaphore = SemaphoreP_create(SEMAPHORE_INIT_COUNT, &dws_semaphore_params);

   if (dws_semaphore == NULL)
   {
       ERR_PRINT("ERROR:\tinit of DWS SEMAPHORE");
   }

   //------------------------ SEND MQTT Semaphore creation --------------------------//

   SemaphoreP_Params_init(&send_data_semaphore_params);
   send_data_semaphore = SemaphoreP_create(SEMAPHORE_INIT_COUNT, &send_data_semaphore_params);

   if (send_data_semaphore == NULL)
   {
       ERR_PRINT("ERROR:\tinit of DWS SEMAPHORE");
   }

   //---------------------- SOFTTIMER THREAD Semaphore creation ---------------------//

   SemaphoreP_Params_init(&softtimer_thread_semaphore_params);
   softtimer_thread_semaphore = SemaphoreP_create(SEMAPHORE_INIT_COUNT, &softtimer_thread_semaphore_params);

   if (softtimer_thread_semaphore == NULL)
   {
       ERR_PRINT("ERROR:\tinit of softtimer THREAD semaphore!!");
   }

   //------------------------ Synchro Semaphore creation --------------------------//

   SemaphoreP_Params_init(&sync_semaphore_params);
   sync_semaphore = SemaphoreP_create(SEMAPHORE_INIT_COUNT, &sync_semaphore_params);

   if (sync_semaphore == NULL)
   {
       ERR_PRINT("ERROR:\tinit of DWS SEMAPHORE");
   }

    //----------------------------Initialization--------------------------//

   LOG_INFO("Start initialization...\n");

    ret = adc_init();
    CHECK_ERROR(ret, "Error in wifi ADC init!!");

    ret = udma_init();
    CHECK_ERROR(ret, "Error in UDMA init!!");

    ret = ti_net_SlNet_initConfig();
    CHECK_ERROR(ret, "Error in wifi SL NET!!");

    ret = wifi_init();
    CHECK_ERROR(ret, "Error in wifi init!!");

    ret = wifi_connect();
    CHECK_ERROR(ret, "Error in wifi connect!!");

    ret = mqtt_init();
    CHECK_ERROR(ret, "Error in MQTT init!!");

    ret = mqtt_subscribe();
    CHECK_ERROR(ret, "Error in MQTT subscribe!!");

    ret = mqtt_connect();
    CHECK_ERROR(ret, "Error in MQTT connect!!");

    //---------------------------- Get MAC ADDRESS -----------------------------//

    uint16_t config_opt = 0;
    ret = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, &config_opt, &mac_addr_size, (uint8_t *)mac_address);

    //------------------ send ready and wait command for init --------------------//

    ret = ask_for_id();
    CHECK_ERROR(ret, "Error in ask for id function!!");
    LOG_INFO("ID device: %d adquired!\n", id_device);

//    GPIO_write(CONFIG_LED_YELLOW, CONFIG_GPIO_LED_ON);

    //----------------------------- Configuration ------------------------------//

    ret = interrupt_config();
    if(ret != INTERRUPT_NO_ERROR)
    {
        ERR_PRINT("ERROR:\tconfigure INTERRUPT\n");
    }

    ret = adc_open();
    if(ret != ADC_NO_ERROR)
    {
        ERR_PRINT("ERROR:\topen ADC\n");
    }

    ret = udma_open();
    if(ret != UDMA_NO_ERROR)
    {
        ERR_PRINT("ERROR:\topen UDMA\n");
    }

    ret = adc_config(ADC_CHANNEL_ID);
    if(ret != ADC_NO_ERROR)
    {
        ERR_PRINT("ERROR:\topen UDMA\n");
    }

    //--------------------------------- START ----------------------------------//

    ret = adc_int_clear(ADC_CHANNEL_ID);
    if (ret != ADC_NO_ERROR)
    {
        ERR_PRINT("ERROR:\tclear ADC INTERRUPT\n");
    }

    ret = adc_start(ADC_CHANNEL_ID);
    if (ret != ADC_NO_ERROR)
    {
        ERR_PRINT("ERROR:\tstart conversion ADC\n");
    }

    cnt_dma_write = 0;
    ret = udma_setup_transfer_pri(&(dma_buffer[cnt_dma_write]), ADC_CHANNEL_ID, DMA_BUFF_SIZE);
    if (ret != UDMA_NO_ERROR)
    {
        ERR_PRINT("ERROR:\tclear setup transfer UDMA\n");
    }

    cnt_dma_write = 1;
    ret = udma_setup_transfer_alt(&(dma_buffer[cnt_dma_write]), ADC_CHANNEL_ID, DMA_BUFF_SIZE);
    if (ret != UDMA_NO_ERROR)
    {
        ERR_PRINT("ERROR:\tclear setup transfer UDMA\n");
    }
    cnt_dma_write = (cnt_dma_write + 1) % DMA_NUM_BUFFERS;

    ret = udma_interrupt_start(INT_ADC, &isr_dma_done);
    if (ret != INTERRUPT_NO_ERROR)
    {
        ERR_PRINT("ERROR:\tstart INTERRRUPTIONS\n");
    }

    //--------------------------- SEND DATA Thread creation --------------------------//

    static arg_thread_send_data_t               arg_thread_send_data;
    arg_thread_send_data.ntp_time               = &ntp_time;
    arg_thread_send_data.time_sync              = &time_sync;
    arg_thread_send_data.mqtt_connected         = &mqtt_connected;
    arg_thread_send_data.send_data_status       = &send_data_status;
    arg_thread_send_data.send_data_semaphore    = send_data_semaphore;
    arg_thread_send_data.msg_data               = msg_data;

    pthread_attr_init(&send_data_thread_attrs);
    send_data_thread_detach_state = PTHREAD_CREATE_DETACHED;
    ret = pthread_attr_setdetachstate(&send_data_thread_attrs, send_data_thread_detach_state);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tconfiguration of dws thread");
    }
    ret = pthread_attr_setstacksize(&send_data_thread_attrs, SEND_DATA_STACK_SIZE);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tconfiguration of dws thread stack");
    }
    send_data_thread_pri_params.sched_priority = SEND_CMD_THREAD_PRIORITY;
    pthread_attr_setschedparam(&send_data_thread_attrs, &send_data_thread_pri_params);
    ret = pthread_create(&snd_mqqt_thread, &send_data_thread_attrs, send_data, (void *) &arg_thread_send_data);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tcreation of send data thread");
    }

    //--------------------------- DWS Thread creation --------------------------//

    static arg_thread_downsampling_t    dws_arg_thread;
    dws_arg_thread.buffer_in            = (uint8_t *) dma_buffer;
    dws_arg_thread.buffer_out           = (uint8_t *) dws_buffer;
    dws_arg_thread.dma_num_buffers      = DMA_NUM_BUFFERS;
    dws_arg_thread.dws_max_buffer       = DWS_MAX_BUFFER;
    dws_arg_thread.size_buffer_in       = DMA_BUFF_SIZE;
    dws_arg_thread.dws_buffer_size      = DWS_SIZE;
    dws_arg_thread.dws_max_buffer       = DWS_BUFFER_SIZE;
    dws_arg_thread.ptr_semaphore        = dws_semaphore;
    dws_arg_thread.ptr_semaphore_mqtt   = send_data_semaphore;
    dws_arg_thread.msg_data             = msg_data;
    dws_arg_thread.ptr_sync_semaphore   = sync_semaphore;
    dws_arg_thread.ptr_sntp_time        = &sntp_time;
    dws_arg_thread.status_time_sync     = &status_time_sync;

    pthread_attr_init(&dws_thread_attrs);
    dws_thread_detach_state = PTHREAD_CREATE_DETACHED;
    ret = pthread_attr_setdetachstate(&dws_thread_attrs, dws_thread_detach_state);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tconfiguration of dws thread");
    }
    ret = pthread_attr_setstacksize(&dws_thread_attrs, DWS_STACK_SIZE);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tconfiguration of dws thread stack");
    }
    dws_thread_pri_param.sched_priority = SEND_CMD_THREAD_PRIORITY;
    pthread_attr_setschedparam(&dws_thread_attrs, &dws_thread_pri_param);
    ret = pthread_create(&dws_thread, &dws_thread_attrs, thread_downsampling, (void *) &dws_arg_thread);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tcreation of downsampling thread");
    }

    Timer_init();
    Timer_Params_init(&timer_params);
    timer_params.periodUnits = Timer_PERIOD_US;
    timer_params.period = USECONDS_TO_SOFTTIMER_CB;
    timer_params.timerMode  = Timer_CONTINUOUS_CALLBACK;
    timer_params.timerCallback = softtimer_callback;

    timer_handle = Timer_open(CONFIG_TIMER_SOFTTIMER, &timer_params);
    Timer_start(timer_handle);

//--------------------------- SOFTTIMER thread creation --------------------------//
    arg_softtimer_thread_t                  arg_softtimer_thread;
    arg_softtimer_thread.ntp_time =         &sntp_time;
    arg_softtimer_thread.time_sync_status =  &status_time_sync;

    pthread_attr_init(&softtimer_thread_attrs);
    softtimer_thread_detach_state = PTHREAD_CREATE_DETACHED;
    ret = pthread_attr_setdetachstate(&softtimer_thread_attrs, softtimer_thread_detach_state);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tconfiguration of send rssi thread!!");
    }
    ret = pthread_attr_setstacksize(&softtimer_thread_attrs, SOFTTIMER_STACK_SIZE);
    if (ret < 0)
    {
        ERR_PRINT("ERROR:\tconfiguration of send rssi thread stack!!");
    }
    softtimer_thread_pri_params.sched_priority = SOFTTIMER_THREAD_PRIORITY;
    pthread_attr_setschedparam(&softtimer_thread_attrs, &softtimer_thread_pri_params);
    ret = pthread_create(&softtimer_thread, &softtimer_thread_attrs, softtimer, (void *) &arg_softtimer_thread);
    if (ret != 0)
    {
        ERR_PRINT("ERROR:\tcreation of send rssi thread!!");
    }

    //--------------------------- TEST WIFI thread creation --------------------------//
        arg_test_wifi_thread_t          arg_test_wifi_thread;
        arg_test_wifi_thread.send_data  = &send_data_status;

        pthread_attr_init(&test_wifi_thread_attrs);
        test_wifi_thread_detach_state = PTHREAD_CREATE_DETACHED;
        ret = pthread_attr_setdetachstate(&test_wifi_thread_attrs, test_wifi_thread_detach_state);
        if (ret != 0)
        {
            ERR_PRINT("ERROR:\tconfiguration of send test wifi!!");
        }
        ret = pthread_attr_setstacksize(&test_wifi_thread_attrs, TEST_WIFI_STACK_SIZE);
        if (ret < 0)
        {
            ERR_PRINT("ERROR:\tconfiguration of send rssi thread stack!!");
        }
        test_wifi_thread_pri_params.sched_priority = TEST_WIFI_THREAD_PRIORITY;
        pthread_attr_setschedparam(&test_wifi_thread_attrs, &test_wifi_thread_pri_params);
        ret = pthread_create(&test_wifi_thread, &test_wifi_thread_attrs, test_wifi, (void *) &arg_test_wifi_thread);
        if (ret != 0)
        {
            ERR_PRINT("ERROR:\tcreation of send rssi thread!!");
        }

//________________________________________________________________________________________________________//

    dma_cnt_interrupts = 0;
    dma_finished = false;

    while (true)
    {
        if(dma_finished == false)
        {
            usleep(10000);
        }
        else
        {
            dma_finished = false;
            adc_int_clear(ADC_CHANNEL_ID);
            udma_interrupt_start(INT_ADC, &isr_dma_done);
        }
    }
}

///////////////////////////////////////////////////
/////////////////// FUNCTIONS /////////////////////
///////////////////////////////////////////////////

/*
 * @brief Interrupt routine when dma transfer is completed.
 * First, clean the interrupts. Next, dma mode used is
 * recharged for the next transfer. Finally, increment the
 * counter cnt_dma_write that stores the number of buffer
 * that have been transfered.
 *
 *
 * @return error code
 */
void isr_dma_done(void)
{
    // Establish the UDMA mode used in the last transfer.
    dma_stop_mode_get(& dma_stop_mode);
    adc_int_clear(ADC_CHANNEL_ID);

    // When udma continues active recharge the control structure used in the last transfer.
    if(dma_finished == false)
    {
        dma_cnt_interrupts++;

        switch(dma_stop_mode)
        {
            case DMA_STOP_MODE_PRI:
                cnt_dma_write = (cnt_dma_write + 1);
                dma_transfer_set(DMA_SET_MODE_PRI, ADC_CHANNEL_ID, &dma_buffer [(cnt_dma_write) % DMA_NUM_BUFFERS], DMA_BUFF_SIZE);
                SemaphoreP_post(dws_semaphore);
                break;
            case DMA_STOP_MODE_ALT:
                cnt_dma_write = (cnt_dma_write + 1);
                dma_transfer_set(DMA_SET_MODE_ALT, ADC_CHANNEL_ID, &dma_buffer [(cnt_dma_write) % DMA_NUM_BUFFERS], DMA_BUFF_SIZE);
                SemaphoreP_post(dws_semaphore);
                break;
            case DMA_STOP_MODE_BASIC:
                dma_transfer_set(DMA_SET_MODE_BASIC, ADC_CHANNEL_ID, &dma_buffer [(cnt_dma_write) % DMA_NUM_BUFFERS], DMA_BUFF_SIZE);
                break;
            case DMA_STOP_NONE:
                break;
            default:
                LOG_ERROR("DMA stop mode error!!");
        }
    }
}

void* softtimer (void *arg_sofftimer_thread)
{
    int64_t                 cnt_softtimer_thread;
    uint64_t                synchro_time;
    int16_t                 wifi_connection_status;
    uint64_t                time_sync_on;
    arg_softtimer_thread_t  arg_softtimer;
    int                     ret;

    cnt_softtimer_thread    = 0;
    time_sync_on            = 1;

    memcpy(&arg_softtimer, arg_sofftimer_thread, sizeof(arg_softtimer_thread_t));

    while (1)
    {
        SemaphoreP_pend(softtimer_thread_semaphore, SemaphoreP_WAIT_FOREVER);

        if (cnt_softtimer_thread % SECONDS_TO_SEND_RSSI == 0)
        {
            wifi_connection_status = test_wifi_connection();
            if (wifi_connection_status == 0)
            {
                ret = send_rssi();
                CHECK_ERROR(ret, "Error in send RSSI!!");
            }
        }

        if (cnt_softtimer_thread % SECONDS_TO_NTP_SYNC == 0)
        {
            synchro_time = sntp_get_time();
            LOG_INFO("Starting synchronization with NTP server...");
            memcpy(arg_softtimer.ntp_time, &synchro_time, sizeof(uint64_t));
            memcpy(arg_softtimer.time_sync_status, &time_sync_on, sizeof(arg_softtimer.time_sync_status));
        }
        cnt_softtimer_thread = cnt_softtimer_thread + 1;
    }
}

//--------------------- Get time from NTP SERVER ---------------------------//

 /* @brief This function obtains the time in seconds elapsed
 * since January 1, 1900. For this purpose it synchronizes with
 * an NTP server via the established connection.
 *
 *@param sntp_time 64-bit unsigned integer where the 32 most
 *        significant bits store the seconds and the 32 least
 *        significant bits store the fraction of seconds.
 *@param slnetsock_af_inet
 *@param sntp_port
 *@param connection address
 *
 * @return error code
 */
uint64_t sntp_get_time (void)
{
    int32_t                 ret;
    SlNetSock_AddrIn_t      ipv4addr;
    SlNetSock_Timeval_t     timeval;
    struct timespec         rtc_timer;
    uint64_t                ntp_time_stamp = 0;

    ret                         = 0;
    ipv4addr.sin_family         = SLNETSOCK_AF_INET;
    ipv4addr.sin_port           = SlNetUtil_htons(SNTP_PORT);
    inet_pton(AF_INET, NTP_SERVER, &ipv4addr.sin_addr.s_addr);

    timeval.tv_sec  = 5;
    timeval.tv_usec = 0;
#ifndef NOT_SYNC_SNTP
    ret = SNTP_getTimeByAddr((SlNetSock_Addr_t *)&ipv4addr, &timeval, &ntp_time_stamp);
    if (ret < 0)
    {
      ERR_PRINT("ERROR: %d\tfailed to get time from NTP server", ret);
    }
#endif
    // Reset the RTC timer to meas the time will pass to next buffer proccess.
    rtc_timer.tv_nsec = 0;
    rtc_timer.tv_sec  = 0;
    ret = clock_settime(CLOCK_REALTIME, &rtc_timer);
    if (ret < 0)
    {
        ERR_PRINT("ERROR: %d\tfailed to fet time from NTP server", ret);
    }

    return ntp_time_stamp;
}

/* @brief This function is the callback of the
* softimer routine. Each time it is triggered,
* it posts a semaphore for to enter the routine.
*
* @param none
*
* @return none
*/
void softtimer_callback (Timer_Handle handle, int_fast16_t status)
{
    SemaphoreP_post(softtimer_thread_semaphore);
}
