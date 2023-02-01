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
*  global_def.h
*/

#include <ti/display/Display.h>
#include <ti/drivers/Timer.h>

///////////////////////////////////////////////////
//////////////////// TYPEDEFS /////////////////////
///////////////////////////////////////////////////
#define DMA_BUFF_SIZE       (800)
#define SIZE_MAC_ADDR       (6)
#define SERVER_ADDRESS      "192.168.117.19"
#define PC_ADDRESS          "192.168.117.19"
#define GOOGLE_TIME_SERVER  "216.239.35.0"
#define CONNECTION_ADRRES   SERVER_ADDRESS
#define NTP_SERVER          SERVER_ADDRESS
#define DWS_RATE            (8)
#define F_SAMPLE            (62500)
#define DISPLAY

//#define BROKER_NOT_CONNECTED
//#define BROKER_NOT_RESPONSE
//#define PRINT_MSG_DATA
//#define PRINT_RSSI_CMD
//#define NOT_SEND_DATA
//#define PRINT_SNTP_TIME
//#define NOT_SYNC_SNTP
//#define TEST_MQTT_CONNECTED
//#define IGNORE_IMPAR_PACKETS

#ifdef DISPLAY

Display_Handle display;

#define UART_PRINT(str, ...) Display_printf(display, 0, 0, str, ##__VA_ARGS__);
#define DBG_PRINT(str, ...)  Display_printf(display, 0, 0, str, ##__VA_ARGS__);
#define ERR_PRINT(str, ...)  Display_printf(display, 0, 0, str, ##__VA_ARGS__);
#define LOG_ERROR(str, ...)  Display_printf(display, 0, 0, str, ##__VA_ARGS__);
#define LOG_INFO(str, ...)  Display_printf(display, 0, 0, str, ##__VA_ARGS__);
#define LOG_TRACE(str, ...)  Display_printf(display, 0, 0, str, ##__VA_ARGS__);

//#define Display_printf(display, 0, 0, str, ##__VA_ARGS__) doanything(void);
#elif defined(NO_DISPLAY)

//#define report_display(str, ...) Display_printf(display, 0, 0, str, __VA_ARGS__);
#define UART_PRINT(str, ...)
#define DBG_PRINT(str, ...)
#define ERR_PRINT(str, ...)
#define LOG_ERROR(str, ...)
#define LOG_INFO(str, ...)
#define LOG_TRACE(str, ...)
#endif
#define CHECK_ERROR(error_code, error_location) \
    { \
        if(error_code < 0) \
        { \
            ERR_PRINT("ERROR: %d", error_code); \
            ERR_PRINT(error_location); \
        } \
    }

typedef uint16_t    dma_buffer_t[DMA_BUFF_SIZE];
Timer_Handle        timer0;
uint8_t             mac_address[SIZE_MAC_ADDR];
