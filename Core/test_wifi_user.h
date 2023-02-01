/*
 * test_wifi_user.h
 *
 *  Created on: 02 jun. 2021
 *      Author: josemanuelcamachosalvador
 */
#ifndef TEST_WIFI_USER_H_
#define TEST_WIFI_USER_H_

#include "ti_drivers_config.h"
#include "global_def.h"
#define US_TEST_WIFI    (500000)

typedef struct __attribute__((__packed__))
{
    int *send_data;

} arg_test_wifi_thread_t;

enum{
    disconnected = 0,
    connected,
};

enum{
    stop = 0,
    start,
};

void* test_wifi (void *arg_test_wifi_thread);

#endif /* TEST_WIFI_USER_H_ */

