/*
 * test_wifi_user.c
 *
 *  Created on: 02 jun. 2021
 *      Author: josemanuelcamachosalvador
 */

#include    "test_wifi_user.h"
#include    <stdio.h>
#include    <string.h>
#include    "wifi_user.h"
#include    <unistd.h>
#include    "mqtt_user.h"

/*
 *  ======== Test wifi connection START ========
 *
 * Check the connection to the wifi network cyclically.
 * If the connection is lost, it notifies the MQTT sending
 * thread to stop sending. Once the connection is recovered,
 * the MQTT service is re-established.
 *
 */

void* test_wifi (void* arg_test_wifi_thread)
{
    int wifi_status, data_send, ret;
    arg_test_wifi_thread_t arg_test_wifi;

    wifi_status = connected;
    data_send   = start;
    ret         = 0;

    memcpy(&arg_test_wifi, arg_test_wifi_thread, sizeof(arg_test_wifi_thread));

    while(1)
    {
        usleep(US_TEST_WIFI);
        wifi_status = test_wifi_connection();
        switch (wifi_status)
        {
            case connected:
                data_send = start;
                memcpy(arg_test_wifi.send_data, &data_send, sizeof(int));
                break;

            case disconnected:
                data_send = stop;
                memcpy(arg_test_wifi.send_data, &data_send, sizeof(int));
                //ret = mqtt_client_disconnect();
                //CHECK_ERROR(ret, "Error in mqtt client disconnect!!");
                while (wifi_status == disconnected)
                {
                    usleep(US_TEST_WIFI);
                    ret = wifi_connect();
                    CHECK_ERROR(ret, "Error in wifi reconnetion!!");
                    wifi_status = test_wifi_connection();
                }

                //ret = mqtt_client_connect();
                //CHECK_ERROR(ret, "Error in mqtt client connect!!");

                break;

            default:
                break;
        }
    }
}
