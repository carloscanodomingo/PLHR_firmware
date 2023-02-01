
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
*  mqtt_user.c
*/

#include "mqtt_user.h"
#include <unistd.h>
#include <stdlib.h>
#include <ti/drivers/Power.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include "msg_structs.h"
#include <ti/net/mqtt/mqttclient.h>
#include "debug_if.h"
#include "global_def.h"
#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogCC32XX.h>

#define MQTT_WILL_TOPIC                 "cc32xx_will_topic"
#define MQTT_WILL_MSG                   "will_msg_works"
#define MQTT_WILL_QOS                   MQTT_QOS_0
#define MQTT_WILL_RETAIN                false
#define MQTT_CLIENT_PASSWORD            NULL
#define MQTT_CLIENT_USERNAME            NULL
#define MQTT_CLIENT_KEEPALIVE           (65535)
#define MQTT_CLIENT_CLEAN_CONNECT       true
#define MQTT_CLIENT_MQTT_V3_1           true
#define MQTT_CLIENT_BLOCKING_SEND       true
#define MQTT_CONNECTION_FLAGS           MQTTCLIENT_NETCONN_IP4
#define MQTT_CONNECTION_PORT_NUMBER     (1883)
#define MQTT_EVENT_RECV MQTT_EVENT_MAX  // event for when receiving data from subscribed topics
#define MQTTAPPTHREADSIZE               (2048)
#define MQTT_MODULE_TASK_PRIORITY       (2)
#define MQTT_MODULE_TASK_STACK_SIZE     (1024)
#define WATCHDOG_TIMER                  (12000)
#define DEFAULT_ID_DEVICE               "255"
#define TOPIC_RCV_CMD                   "CC32xx/cmd/s2d/"
#define TOPIC_PUB_CMD                   "CC32xx/cmd/d2s/"
#define TOPIC_PUB_DATA                  "CC32xx/data/"

enum{
    MQTT_STATE_IDLE,
    MQTT_STATE_INITIALIZED,
    MQTT_STATE_CONNECTED
};

enum{
    disconnected = 0,
    connected,
};

enum{
    stop = 0,
    start,
};

MQTTClient_Handle   mqtt_client_handle;

// structure for linked list elements to manage the topics
struct Node
{
    MQTTClient_SubscribeParams topicParams;
    MQTT_IF_TopicCallback_f topicCB;
    struct Node* next;
};

struct msgQueue
{
    int     event;
    char*   topic;
    char*   payload;
    uint8_t qos;
};

static struct mqttContext
{
    mqd_t msgQueue;
    pthread_mutex_t *moduleMutex;
    MQTTClient_Handle mqttClient;
    MQTT_IF_EventCallback_f eventCB;
    struct Node* head;
    int moduleState;
    uint8_t clientDisconnectFlag;
} mMQTTContext = {NULL, NULL, NULL, NULL, NULL, MQTT_STATE_IDLE, 0};

MQTTClient_Will mqttWillParams =
{
     MQTT_WILL_TOPIC,
     MQTT_WILL_MSG,
     MQTT_WILL_QOS,
     MQTT_WILL_RETAIN
};

MQTT_IF_ClientParams_t mqtt_client_params =
{
     client_id,
     MQTT_CLIENT_USERNAME,
     MQTT_CLIENT_PASSWORD,
     MQTT_CLIENT_KEEPALIVE,
     MQTT_CLIENT_CLEAN_CONNECT,
     MQTT_CLIENT_MQTT_V3_1,
     MQTT_CLIENT_BLOCKING_SEND,
     &mqttWillParams
};

MQTTClient_ConnParams mqtt_connection_params =
{
     MQTT_CONNECTION_FLAGS,
     CONNECTION_ADRRES,
     MQTT_CONNECTION_PORT_NUMBER,
     0,
     0,
     0,
     NULL
};

MQTT_IF_InitParams_t mqttInitParams =
{
     MQTT_MODULE_TASK_STACK_SIZE,
     MQTT_MODULE_TASK_PRIORITY
};

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};

Watchdog_Handle watchdog_handle;
Watchdog_Params watchdog_params;

// Callback invoked by the internal MQTT library to notify the application of MQTT events
void MQTTClientCallback(int32_t event, void *metaData, uint32_t metaDateLen, void *data, uint32_t dataLen)
{
    int status;
    struct msgQueue queueElement;

    switch((MQTTClient_EventCB)event)
    {
        case MQTTClient_OPERATION_CB_EVENT:
        {
            switch(((MQTTClient_OperationMetaDataCB *)metaData)->messageType)
            {
                case MQTTCLIENT_OPERATION_CONNACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: CONNACK\r\n");
                    queueElement.event = MQTT_EVENT_CONNACK;
                    break;
                }

                case MQTTCLIENT_OPERATION_EVT_PUBACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: PUBACK\r\n");
                    queueElement.event = MQTT_EVENT_PUBACK;
                    break;
                }

                case MQTTCLIENT_OPERATION_SUBACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: SUBACK\r\n");
                    queueElement.event = MQTT_EVENT_SUBACK;
                    break;
                }

                case MQTTCLIENT_OPERATION_UNSUBACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: UNSUBACK\r\n");
                    queueElement.event = MQTT_EVENT_UNSUBACK;
                    break;
                }
            }
            break;
        }

        case MQTTClient_RECV_CB_EVENT:
        {
            LOG_TRACE("MQTT CLIENT CB: RECV CB\r\n");

            MQTTClient_RecvMetaDataCB *receivedMetaData;
            char *topic;
            char *payload;

            receivedMetaData = (MQTTClient_RecvMetaDataCB *)metaData;

            // copying received topic data locally to send over msg queue
            topic = (char*)malloc(receivedMetaData->topLen+1);
            memcpy(topic, receivedMetaData->topic, receivedMetaData->topLen);
            topic[receivedMetaData->topLen] = 0;

            payload = (char*)malloc(dataLen+1);
            memcpy(payload, data, dataLen);
            payload[dataLen] = 0;

            queueElement.event =   MQTT_EVENT_RECV;
            queueElement.topic =   topic;
            queueElement.payload = payload;
            queueElement.qos = receivedMetaData->qos;

            break;
        }
        case MQTTClient_DISCONNECT_CB_EVENT:
        {
            LOG_TRACE("MQTT CLIENT CB: DISCONNECT\r\n");

            queueElement.event = MQTT_EVENT_SERVER_DISCONNECT;

            break;
        }
    }

    // signaling the MQTTAppThead of events received from the internal MQTT module
    status = mq_send(mMQTTContext.msgQueue, (char*)&queueElement, sizeof(struct msgQueue), 0);
    if(status < 0){
        LOG_ERROR("msg queue send error %d", status);
    }
}

// Helper function used to compare topic strings and accounts for MQTT wild cards
int MQTTHelperTopicMatching(char *s, char *p)
{
    char *s_next = (char*)-1, *p_next;

    for(; s_next; s=s_next+1, p=p_next+1)
    {
        int len;

        if(s[0] == '#')
            return 1;

        s_next = strchr(s, '/');
        p_next = strchr(p, '/');

        len = ((s_next) ? (s_next - s) : (strlen(s))) + 1;
        if(s[0] != '+')
        {
            if(memcmp(s, p, len) != 0)
                return 0;
        }
    }
    return (p_next)?0:1;
}

// This is the application thread for the MQTT module that signals
void *MQTTAppThread(void *threadParams)
{
    int ret;
    struct msgQueue queueElement;

    while(1)
    {
        mq_receive(mMQTTContext.msgQueue, (char*)&queueElement, sizeof(struct msgQueue), NULL);

        ret = 0;

        if(queueElement.event == MQTT_EVENT_RECV)
        {
            LOG_TRACE("MQTT APP THREAD: RECV TOPIC = %s", queueElement.topic);

            pthread_mutex_lock(mMQTTContext.moduleMutex);

            if(mMQTTContext.head != NULL){

                struct Node* tmp = mMQTTContext.head;

                // check what queueElement.topic to invoke the appropriate topic callback event for the user
                while(ret == 0){
                    ret = MQTTHelperTopicMatching(tmp->topicParams.topic, queueElement.topic);
                    if(ret == 1){

                        LOG_DEBUG("TOPIC MATCH %s\r\n", queueElement.topic);

                        tmp->topicCB(queueElement.topic, queueElement.payload, queueElement.qos);
                        break;
                    }
                    tmp = tmp->next;
                    if(tmp == NULL){
                        LOG_INFO("Cannot invoke CB for topic not in topic list\r\n");
                        LOG_INFO("TOPIC: %s \tPAYLOAD: %s\r\n", queueElement.topic, queueElement.payload);
                        break;
                    }
                }
            }

            pthread_mutex_unlock(mMQTTContext.moduleMutex);

            free(queueElement.topic);
            free(queueElement.payload);
        }
        // when MQTT_IF_Deinit is called we must close the message queue and terminate the MQTTAppThread
        else if(queueElement.event == MQTT_EVENT_DESTROY)
        {
            LOG_TRACE("MQTT APP THREAD: DESTROY\r\n");

            mMQTTContext.eventCB(queueElement.event);

            ret = mq_close(mMQTTContext.msgQueue);
            if(ret < 0){
                LOG_ERROR("msg queue close error %d", ret);
            }
            pthread_exit(0);
        }
        else if(queueElement.event == MQTT_EVENT_SERVER_DISCONNECT){

            LOG_TRACE("MQTT APP THREAD: DISCONNECT\r\n");

            int tmp;    // tmp is to avoid invoking the eventCB while mutex is still locked to prevent deadlock
            pthread_mutex_lock(mMQTTContext.moduleMutex);

            // checks if the disconnect event came because the client called disconnect or the server disconnected
            if(mMQTTContext.clientDisconnectFlag == 1){
                tmp = 1;
                mMQTTContext.clientDisconnectFlag = 0;
            }
            else{
                tmp = 0;
            }
            pthread_mutex_unlock(mMQTTContext.moduleMutex);

            if(tmp == 1){
                mMQTTContext.eventCB(MQTT_EVENT_CLIENT_DISCONNECT);
            }
            else{
                mMQTTContext.eventCB(queueElement.event);
            }
        }
        else if(queueElement.event == MQTT_EVENT_CONNACK){

            LOG_TRACE("MQTT APP THREAD: CONNACK\r\n");

            pthread_mutex_lock(mMQTTContext.moduleMutex);

            // in case the user re-connects to a server this checks if there is a list of topics to
            // automatically subscribe to the topics again
            if(mMQTTContext.head != NULL){

                struct Node* curr = mMQTTContext.head;
                struct Node* prev;
                struct Node* tmp;

                // iterate through the linked list until the end is reached
                while(curr != NULL){

                    tmp = curr;

                    ret = MQTTClient_subscribe(mMQTTContext.mqttClient, &curr->topicParams, 1);
                    // if subscribing to a topic fails we must remove the node from the list since we are no longer subscribed
                    if(ret < 0){

                        LOG_ERROR("SUBSCRIBE FAILED: %s", curr->topicParams.topic);

                        // if the node to remove is the head update the head pointer
                        if(curr == mMQTTContext.head){
                            mMQTTContext.head = curr->next;
                            curr = curr->next;
                        }
                        else if(curr->next != NULL){
                            prev->next = curr->next;
                            curr = curr->next->next;
                        }
                        else{
                            prev->next = curr->next;
                            curr = curr->next;
                        }

                        // delete the node from the linked list
                        free(tmp->topicParams.topic);
                        free(tmp);
                    }
                    else{
                        prev = curr;
                        curr = curr->next;
                    }
                }
            }
            pthread_mutex_unlock(mMQTTContext.moduleMutex);
            // notify the user of the connection event
            mMQTTContext.eventCB(queueElement.event);
        }
        else{

            LOG_TRACE("MQTT APP THREAD: OTHER\r\n");
            // if the module received any other event nothing else needs to be done except call it
            mMQTTContext.eventCB(queueElement.event);
        }
    }
}

// this thread is for the internal MQTT library and is required for the library to function
void *MQTTContextThread(void *threadParams)
{
    int ret;

    LOG_TRACE("CONTEXT THREAD: RUNNING\r\n");

    MQTTClient_run((MQTTClient_Handle)threadParams);

    LOG_TRACE("CONTEXT THREAD: MQTTClient_run RETURN\r\n");

    pthread_mutex_lock(mMQTTContext.moduleMutex);

    ret = MQTTClient_delete(mMQTTContext.mqttClient);
    if(ret < 0){
        LOG_ERROR("client delete error %d", ret);
    }

    LOG_TRACE("CONTEXT THREAD: MQTT CLIENT DELETED")

    mMQTTContext.moduleState = MQTT_STATE_INITIALIZED;

    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    LOG_TRACE("CONTEXT THREAD EXIT\r\n");

    pthread_exit(0);

    return(NULL);
}

/**
 \brief  Create the infrastructure for the MQTT_IF (mqtt interface) module

 \param[in] initParams: parameters to set stack size and thread priority for module

 \return Success 0 or Failure -1

 \sa MQTT_IF_Deinit()
 */
int MQTT_IF_Init(MQTT_IF_InitParams_t initParams)
{
    int ret;
    mq_attr attr;
    pthread_attr_t threadAttr;
    struct sched_param schedulingparams;
    pthread_t mqttAppThread = (pthread_t) NULL;

    if(mMQTTContext.moduleState != MQTT_STATE_IDLE)
    {
        LOG_ERROR("library only supports 1 active init call\r\n");
        return -1;
    }

    // The mutex is to protect data in the mMQTTContext structure from being manipulated or accessed at the wrong time
    mMQTTContext.moduleMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(mMQTTContext.moduleMutex == NULL)
    {
        LOG_ERROR("malloc failed: mutex\r\n");
        while (1);
    }

    ret = pthread_mutex_init(mMQTTContext.moduleMutex, (const pthread_mutexattr_t *)NULL);
    if (ret != 0)
    {
        LOG_ERROR("mutex init failed\r\n");
        while (1);
    }

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    // Initializing the message queue for the MQTT module
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(struct msgQueue);
    mMQTTContext.msgQueue = mq_open("msgQueue", O_CREAT, 0, &attr);
    if(((int)mMQTTContext.msgQueue) <= 0){
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (int)mMQTTContext.msgQueue;
    }

    pthread_attr_init(&threadAttr);
    schedulingparams.sched_priority = initParams.threadPriority;
    ret = pthread_attr_setschedparam(&threadAttr, &schedulingparams);
    ret |= pthread_attr_setstacksize(&threadAttr, initParams.stackSize);
    ret |= pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    ret |= pthread_create(&mqttAppThread, &threadAttr, MQTTAppThread, NULL);
    if(ret == 0){
        mMQTTContext.moduleState = MQTT_STATE_INITIALIZED;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return ret;
}

/**
 \brief  Destroys the infrastructure created from calling MQTT_IF_Init

 \param[in] mqttClientHandle: handle for the mqtt client module instance

 \return Success 0 or Failure -1

 \sa MQTT_IF_Init()
 */
int MQTT_IF_Deinit(MQTTClient_Handle mqtt_client_handle)
{
    int ret;
    struct msgQueue queueElement;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_INITIALIZED){
        if(mMQTTContext.moduleState == MQTT_STATE_CONNECTED){
            LOG_ERROR("call disconnect before calling deinit\r\n");
            pthread_mutex_unlock(mMQTTContext.moduleMutex);
            return -1;
        }
        else if(mMQTTContext.moduleState == MQTT_STATE_IDLE){
            LOG_ERROR("init has not been called\r\n");
            pthread_mutex_unlock(mMQTTContext.moduleMutex);
            return -1;
        }
    }

    queueElement.event = MQTT_EVENT_DESTROY;

    // since user called MQTT_IF_Deinit send the signal to the app thread so it may terminate itself
    ret = mq_send(mMQTTContext.msgQueue, (const char*)&queueElement, sizeof(struct msgQueue), 0);
    if(ret < 0){
        LOG_ERROR("msg queue send error %d", ret);
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    struct Node* tmp = mMQTTContext.head;

    // in case the user did not unsubscribe to topics this loop will free any memory that was allocated
    while(tmp != NULL){
        free(tmp->topicParams.topic);
        mMQTTContext.head = tmp->next;
        free(tmp);
        tmp = mMQTTContext.head;
    }

    // setting the MQTT module state back so that user can call init if they wish to use it again
    mMQTTContext.moduleState = MQTT_STATE_IDLE;
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return 0;
}

/**
 \brief  Connect will set all client and connection parameters and initiate a connection to the broker.
         It will also register the event callback defined in the user's application and the create an
         internal context thread for the internal MQTT library

 \param[in] mqttClientParams: params for the mqtt client parameters
 \param[in] mqttConnParams: params for the mqtt connection parameters
 \param[in] mqttCB: the callback that is registered when for MQTT operation events (e.g. CONNACK, SUBACK...)

 \return Success 0 or Failure -1

 \sa MQTT_IF_Disconnect()
 */
MQTTClient_Handle MQTT_IF_Connect(MQTT_IF_ClientParams_t mqtt_client_params, MQTTClient_ConnParams mqtt_connection_params, MQTT_IF_EventCallback_f mqttCB)
{
    int ret;
    pthread_attr_t threadAttr;
    struct sched_param priParam;
    pthread_t mqttContextThread = (pthread_t) NULL;
    MQTTClient_Params clientParams;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    // if the user has not called init this will return error since they're trying to connect without intializing the module
    if(mMQTTContext.moduleState != MQTT_STATE_INITIALIZED){

        if(mMQTTContext.moduleState == MQTT_STATE_CONNECTED){
            LOG_ERROR("mqtt library is already connected to a broker\r\n");
        }
        else{
            LOG_ERROR("must call init before calling connect\r\n");
        }
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (MQTTClient_Handle)-1;
    }

    // copying the event callback the user registered for the module context
    mMQTTContext.eventCB = mqttCB;

    // preparing the data for MQTTClient_create
    clientParams.blockingSend = mqtt_client_params.blockingSend;
    clientParams.clientId = mqtt_client_params.clientID;
    clientParams.connParams = &mqtt_connection_params;
    clientParams.mqttMode31 = mqtt_client_params.mqttMode31;

    mMQTTContext.mqttClient = MQTTClient_create(MQTTClientCallback, &clientParams);
    if(mMQTTContext.mqttClient == NULL){
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (MQTTClient_Handle)-1;
    }

    pthread_attr_init(&threadAttr);
    priParam.sched_priority = 2;
    ret = pthread_attr_setschedparam(&threadAttr, &priParam);
    ret |= pthread_attr_setstacksize(&threadAttr, MQTTAPPTHREADSIZE);
    ret |= pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    ret |= pthread_create(&mqttContextThread, &threadAttr, MQTTContextThread, NULL);
    if(ret != 0){
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (MQTTClient_Handle)-1;
    }

    // if the user included additional parameters for the client MQTTClient_set will be called
    if(mqtt_client_params.willParams != NULL){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_WILL_PARAM, mqtt_client_params.willParams, sizeof(MQTTClient_Will));
    }

    if(mqtt_client_params.username != NULL){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_USER_NAME, mqtt_client_params.username, strlen(mqtt_client_params.username));
    }

    if(mqtt_client_params.password != NULL){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_PASSWORD, mqtt_client_params.password, strlen(mqtt_client_params.password));
    }

    if(mqtt_client_params.cleanConnect == false){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_CLEAN_CONNECT, &mqtt_client_params.cleanConnect, sizeof(mqtt_client_params.cleanConnect));
    }

    if(mqtt_client_params.keepaliveTime > 0){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_KEEPALIVE_TIME, &mqtt_client_params.keepaliveTime, sizeof(mqtt_client_params.keepaliveTime));
    }

    ret = MQTTClient_connect(mMQTTContext.mqttClient);
    if(ret < 0){
        UART_PRINT("connect failed: %d\r\n", ret);
    }
    else{
        mMQTTContext.moduleState = MQTT_STATE_CONNECTED;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    if(ret < 0){
        return (MQTTClient_Handle)ret;
    }
    else{
        return mMQTTContext.mqttClient;
    }
}

/**
 \brief  Instructs the internal MQTT library to close the MQTT connection to the broker

 \param[in] mqttClientHandle: handle for the mqtt client module instance

 \return Success 0 or Failure -1

 \sa MQTT_IF_Connect()
 */
int MQTT_IF_Disconnect(MQTTClient_Handle mqtt_client_handle)
{
    int ret;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_CONNECTED){
        LOG_ERROR("not connected to broker\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    mMQTTContext.clientDisconnectFlag = 1;

    ret = MQTTClient_disconnect(mqtt_client_handle);
    if(ret < 0){
        LOG_ERROR("disconnect error %d", ret);
    }
    else{
        mMQTTContext.moduleState = MQTT_STATE_INITIALIZED;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return ret;
}

/**
 \brief  Subscribes to the topics specified by the caller in subscriptionInfo. Topic subscriptions are agnostic to the
 \ connection status. Meaning if you subscribe to topics then disconnect, the MQTT_IF module will still hold the topics
 \ so on re-connect the original topics are subscribed to again automatically.

 \param[in] mqttClientHandle: handle for the mqtt client module instance
 \param[in] subscriptionInfo: data structure containing all the data required to subscribe
 \param[in] numOfTopics: number of topics stored in subscriptionInfo

 \return Success 0 or Failure -1

 \sa MQTT_IF_Unsubscribe()
 */
int MQTT_IF_Subscribe(MQTTClient_Handle mqtt_client_handle, const char* topic, unsigned int qos, MQTT_IF_TopicCallback_f topicCB)
{
    int ret = 0;
    struct Node* topicEntry = NULL;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState == MQTT_STATE_IDLE){
        LOG_ERROR("user must call MQTT_IF_Init before using the MQTT module\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    // preparing the topic node to add it to the linked list that tracks all the subscribed topics
    topicEntry = (struct Node*)malloc(sizeof(struct Node));
    if(topicEntry == NULL){
        LOG_ERROR("malloc failed: list entry\n\r");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    topicEntry->topicParams.topic = (char*)malloc((strlen(topic)+1)*sizeof(char));
    if(topicEntry->topicParams.topic == NULL){
        LOG_ERROR("malloc failed: topic\n\r");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    strcpy(topicEntry->topicParams.topic, topic);
    topicEntry->topicParams.qos = qos;
    topicEntry->topicCB = topicCB;

    // adding the topic node to the linked list
    topicEntry->next = mMQTTContext.head;
    mMQTTContext.head = topicEntry;

    if(mMQTTContext.moduleState == MQTT_STATE_CONNECTED){
        ret = MQTTClient_subscribe(mMQTTContext.mqttClient, &topicEntry->topicParams, 1);
        // if the client fails to subscribe to the node remove the topic from the list
        if(ret < 0){
            LOG_ERROR("subscribe failed %d. removing topic from list", ret);
            free(topicEntry->topicParams.topic);
            free(topicEntry);
        }
    }

    pthread_mutex_unlock(mMQTTContext.moduleMutex);
    return ret;
}

/**
 \brief  Unsubscribes to the topics specified by the caller in subscriptionInfo

 \param[in] mqttClientHandle: handle for the mqtt client module instance
 \param[in] subscriptionInfo: data structure containing all the data required to subscribe
 \param[in] numOfTopics: number of topics stored in subscriptionInfo

 \return Success 0 or Failure -1

 \sa MQTT_IF_Subscribe()
 */
int MQTT_IF_Unsubscribe(MQTTClient_Handle mqtt_client_handle, char* topic)
{
    int ret = 0;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_CONNECTED){
        LOG_ERROR("not connected to broker\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    MQTTClient_UnsubscribeParams unsubParams;
    unsubParams.topic = topic;

    ret = MQTTClient_unsubscribe(mMQTTContext.mqttClient, &unsubParams, 1);
    if(ret < 0){
        LOG_ERROR("unsub failed\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return ret;
    }
    else{
        // since unsubscribe succeeded the topic linked list must be updated to remove the topic node
        struct Node* curr = mMQTTContext.head;
        struct Node* prev;

        while(curr != NULL){

            // compare the current topic to the user passed topic
            ret = MQTTHelperTopicMatching(curr->topicParams.topic, topic);
            // if there is a match update the node pointers and remove the node
            if(ret == 1){

                if(curr == mMQTTContext.head){
                    mMQTTContext.head = curr->next;
                }
                else{
                    prev->next = curr->next;
                }
                free(curr->topicParams.topic);
                free(curr);
                pthread_mutex_unlock(mMQTTContext.moduleMutex);
                return ret;
            }
            prev = curr;
            curr = curr->next;
        }
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
    }

    LOG_ERROR("topic does not exist\r\n");
    pthread_mutex_unlock(mMQTTContext.moduleMutex);
    return -1;
}

/**
 \brief  Publishes to the topic specified by the user

 \param[in] mqttClientHandle: handle for the mqtt client module instance
 \param[in] topic: topic user wants to publish to
 \param[in] payload: payload to publish to topic
 \param[in] payloadLen: length of payload passed by the user
 \param[in] flags QOS define MQTT_PUBLISH_QOS_0, MQTT_PUBLISH_QOS_1 or MQTT_PUBLISH_QOS_2
             use MQTT_PUBLISH_RETAIN is message should be retained

 \return Success 0 or Failure -1

 */
//\param[in] flags QOS define MQTT_PUBLISH_QOS_0, MQTT_PUBLISH_QOS_1 or MQTT_PUBLISH_QOS_2
//use MQTT_PUBLISH_RETAIN if message should be retained
int MQTT_IF_Publish(MQTTClient_Handle mqtt_client_params, const char* topic, char* payload, unsigned short payloadLen, int flags)
{
    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_CONNECTED){
        LOG_ERROR("not connected to broker\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return MQTTClient_publish(mqtt_client_params,
                              (char*)topic,
                              strlen((char*)topic),
                              (char*)payload,
                              payloadLen,
                              flags);
}

int test_mqtt_connection (void)
{
    int32_t ret = -1;

    return ret;
}

void* send_data(void *arg_send_data_thread)
{
    uint8_t                 msg_op;
    uint16_t                data_buffer_max;
    uint16_t                buffer_size;
    uint16_t                msg_size;
    uint32_t                tick_watchdog;
    uint32_t                msg_data_id;
    int32_t                 ret;
    int                     n_msg_data;
    int                     mqtt_connected;
    int                     send_data_status;
    arg_thread_send_data_t  arg_send_data;
    msg_data_t              *ptr_msg_data;

    msg_data_id         = 0;
    n_msg_data          = 0;
    mqtt_connected      = 0;

    Watchdog_Params_init(&watchdog_params);
    watchdog_params.callbackFxn = (Watchdog_Callback) watchdog_callback;
    watchdog_params.debugStallMode = Watchdog_DEBUG_STALL_ON;
    watchdog_params.resetMode = Watchdog_RESET_ON;

    watchdog_handle = Watchdog_open(CONFIG_WATCHDOG_0, &watchdog_params);
    if (watchdog_handle == NULL)
    {
        LOG_ERROR("Error opening watchdog!!");
    }
    tick_watchdog = Watchdog_convertMsToTicks(watchdog_handle, WATCHDOG_TIMER);

    if (tick_watchdog != NULL)
    {
        Watchdog_setReload(watchdog_handle, tick_watchdog);
    }
    else
    {
        LOG_ERROR("Error in convert tick watchdog!!");
    }

    memcpy(&arg_send_data, arg_send_data_thread, sizeof(arg_thread_send_data_t));
    memcpy(&mqtt_connected, &arg_send_data.mqtt_connected, sizeof(int));

    while(true)
    {
        SemaphoreP_pend(arg_send_data.send_data_semaphore, SemaphoreP_WAIT_FOREVER);
#ifdef IGNORE_IMPAR_PACKETS
        if (n_msg_data % 2 == 0)
        {
#endif
        // ptr_msg_data points to msg_data struct wich will be filled.
        ptr_msg_data = &((arg_send_data.msg_data)[n_msg_data]);

        data_buffer_max = (uint16_t)DATA_BUFFER_MAX;
        buffer_size = sizeof(data_buffer_t);
        msg_size = sizeof(msg_data_t);
        msg_op = (uint8_t)DATA;

        memcpy(&(ptr_msg_data->p.h.data_buffer_num), &data_buffer_max, sizeof(data_buffer_max));
        memcpy(&(ptr_msg_data->p.h.buffer_size), &buffer_size, sizeof(buffer_size));
        memcpy(&(ptr_msg_data->h.id_device), &id_device, sizeof(id_device));
        memcpy(&(ptr_msg_data->h.msg_op), &msg_op, sizeof(msg_op));
        memcpy(&(ptr_msg_data->h.msg_size), &msg_size, sizeof(msg_size));
        memcpy(&(ptr_msg_data->p.h.data_id), &msg_data_id, sizeof(msg_data_id));

//        GPIO_toggle(CONFIG_LED_GREEN);

#ifdef PRINT_MSG_DATA
        uint16_t n_buffer_print, n_data_print;
        uint32_t msg_data_id, data_to_print_right, data_to_print_left;

        UART_PRINT ("\ndata = [");
        for (n_buffer_print = 0; n_buffer_print < DATA_BUFFER_MAX; ++n_buffer_print)
        {
            for (n_data_print = 0; n_data_print < (DATA_BUFFER_L / 3); ++n_data_print)
            {
                memcpy(&data_to_print_right, &msg_data[n_msg_data].p.p[n_buffer_print].p[n_data_print * 3], 3);
                data_to_print_right = (data_to_print_right & 0xFFF);
                UART_PRINT ("%d, ", data_to_print_right);
                memcpy(&data_to_print_left, &msg_data[n_msg_data].p.p[n_buffer_print].p[n_data_print  * 3], 3);
                data_to_print_left = ((data_to_print_left >> 12) & 0xFFF);
                UART_PRINT ("%d, ", data_to_print_left);
            }
        }
        UART_PRINT ("];\n");
#endif

#ifndef NOT_SEND_DATA

        send_data_status = stop;

        while (send_data_status == stop)
        {
            memcpy(&send_data_status, arg_send_data.send_data_status, sizeof(int));
            usleep(1000);
        }

        ret = mqtt_publish((uint16_t)TYPE_DATA, (char *) ptr_msg_data, (unsigned short)sizeof(msg_data_t));
        if (ret < 0)
        {
            UART_PRINT("ERROR: %d\tConnection to broker lost!", ret);
        }
        else
        {
            Watchdog_clear(watchdog_handle);
        }

#endif
#ifdef IGNORE_IMPAR_PACKETS
        }
#endif
        msg_data_id ++;
        n_msg_data = (n_msg_data + 1) % N_MSG_DATA;
    }
}

int mqtt_init(void)
{
    int ret = 0;
    ret = MQTT_IF_Init(mqttInitParams);
    if(ret < 0)
    {
       ERR_PRINT("ERROR:\tfail to init MQTT\n");
    }

    return ret;
}

int mqtt_connect(void)
{
    int ret = 0;
#ifndef BROKER_NOT_CONNECTED

    mqtt_client_handle = MQTT_IF_Connect(mqtt_client_params, mqtt_connection_params, mqtt_event_callback);

    if(mqtt_client_handle < 0)
    {
        ERR_PRINT("ERROR:\tfail to connect MQTT\n");
    }
    else
    {
        LOG_INFO("Broker connected succesfully!\n");
    }

    // Wait for mqtt connection
    while (mqtt_connected == 0)
    {
        usleep(100000);
    }

#endif

    return ret;
}

int mqtt_disconnect(void)
{
    int ret = 0;
#ifndef BROKER_NOT_CONNECTED

    ret = MQTT_IF_Disconnect(mqtt_client_handle);
    CHECK_ERROR(ret, "Fail to disconnect MQTT!!")

    mqtt_connected = 0;
#endif

    return ret;
}


void mqtt_event_callback(int32_t event)
{
    switch(event){
        case MQTT_EVENT_CONNACK:
        {
            LOG_INFO("MQTT_EVENT_CONNACK\r\n");
            mqtt_connected = 1;
            break;
        }
        case MQTT_EVENT_CLIENT_DISCONNECT:
        {
            LOG_INFO("MQTT_EVENT_CLIENT_DISCONNECT\r\n");
            mqtt_connected = 0;
            break;
        }
        case MQTT_EVENT_SERVER_DISCONNECT:
        {
            LOG_INFO("MQTT_EVENT_SERVER_DISCONNECT\r\n");
            mqtt_connected = 0;
            break;
        }
        case MQTT_EVENT_DESTROY:
        {
            LOG_INFO("MQTT_EVENT_DESTROY\r\n");
            break;
        }
        default:
        {
            LOG_INFO("Unknown MQTT event\r\n");
            break;
        }
    }
}

void cmd_zero(char* topic, char* p, uint8_t qos)
{
    char                *ptr_payload;
    msg_reception_t     msg_reception;
    cmd_resp_id_t       cmd_resp_id;
    uint16_t            cmd_id_reception;

    ptr_payload   = p;

    memcpy(&msg_reception, ptr_payload, sizeof(msg_reception_t));

    if (msg_reception.id_device_reception != 0)
    {
        //check if device sender is server. If not ignore
        return;
    }

    switch (msg_reception.msg_op_reception)
    {
    case CMD:
        // get cmd_id
        ptr_payload = (p + sizeof(msg_header_t));
        memcpy(&cmd_id_reception, ptr_payload, sizeof(cmd_header_t));

        switch(cmd_id_reception)
        {
        case CMD_READY:

            break;

        case CMD_RESP_ID:
            ptr_payload = (ptr_payload + sizeof(cmd_header_t));
            memcpy(&cmd_resp_id, ptr_payload, sizeof(cmd_resp_id_t));

            // check mac address
            if (cmd_resp_id.mac_addr[0] != mac_address[0] ||
                cmd_resp_id.mac_addr[1] != mac_address[1] ||
                cmd_resp_id.mac_addr[2] != mac_address[2] ||
                cmd_resp_id.mac_addr[3] != mac_address[3] ||
                cmd_resp_id.mac_addr[4] != mac_address[4] ||
                cmd_resp_id.mac_addr[5] != mac_address[5])
            {
                // mac error!
                return;
            }
            id_device = cmd_resp_id.cmd_resp_id;

            break;

        default:

            break;
        }

        break;

    case DATA:

        break;

    case CONFIG:

        break;

    default:
        // MSG_OP error!
        return;
    }
}

int32_t SetClientIdNamefromMacAddress(void)
{
    int32_t ret = 0;
    uint8_t Client_Mac_Name[2];
    uint8_t Index;
    uint16_t macAddressLen = SIZE_MAC_ADDR;
    uint8_t macAddress[SIZE_MAC_ADDR];

    /*Get the device Mac address */
    ret = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, 0, &macAddressLen,
                       &macAddress[0]);
    client_id[0] = '\0';
    /*When ClientID isn't set, use the mac address as ClientID               */
    if(client_id[0] == '\0')
    {
        /*6 bytes is the length of the mac address                           */
        for(Index = 0; Index < SIZE_MAC_ADDR; Index++)
        {
            /*Each mac address byte contains two hexadecimal characters      */
            /*Copy the 4 MSB - the most significant character                */
            Client_Mac_Name[0] = (macAddress[Index] >> 4) & 0xf;
            /*Copy the 4 LSB - the least significant character               */
            Client_Mac_Name[1] = macAddress[Index] & 0xf;

            if(Client_Mac_Name[0] > 9)
            {
                /*Converts and copies from number that is greater than 9 in  */
                /*hexadecimal representation (a to f) into ascii character   */
                client_id[2 * Index] = Client_Mac_Name[0] + 'a' - 10;
            }
            else
            {
                /*Converts and copies from number 0 - 9 in hexadecimal       */
                /*representation into ascii character                        */
                client_id[2 * Index] = Client_Mac_Name[0] + '0';
            }
            if(Client_Mac_Name[1] > 9)
            {
                /*Converts and copies from number that is greater than 9 in  */
                /*hexadecimal representation (a to f) into ascii character   */
                client_id[2 * Index + 1] = Client_Mac_Name[1] + 'a' - 10;
            }
            else
            {
                /*Converts and copies from number 0 - 9 in hexadecimal       */
                /*representation into ascii character                        */
                client_id[2 * Index + 1] = Client_Mac_Name[1] + '0';
            }
        }
    }
    return(ret);
}

int mqtt_client_disconnect (void)
{
    int ret = 0;

    ret = MQTT_IF_Disconnect(mqtt_client_handle);
    CHECK_ERROR(ret, "Error in MQTT disconnect!!");

    ret = MQTT_IF_Deinit(mqtt_client_handle);
    CHECK_ERROR(ret, "Error in MQTT deinit!!");

    return ret;
}

int mqtt_client_connect (void)
{
    int ret = 0;

    ret = mqtt_init();
    CHECK_ERROR(ret, "Error in MQTT reinit!!");

    ret = mqtt_subscribe();
    CHECK_ERROR(ret, "Error in MQTT subscribe!!");

    ret = mqtt_connect();
    CHECK_ERROR(ret, "Error in MQTT reconnet!!");

    return ret;
}

int mqtt_subscribe (void)
{
    int ret = 0;

    ret = MQTT_IF_Subscribe(mqtt_client_handle, topic_1, MQTT_QOS_0, cmd_zero);
    if(ret != 0)
    {
        ERR_PRINT("ERROR:\tsuscribe topic MQTT\n");
    }

    return ret;
}

void watchdog_callback(void)
{
    LOG_INFO("Reset device");
    while(1);
}

int mqtt_publish (uint16_t type_publish, char* payload, unsigned short len)
{
    int ret;

    char id_device_string[sizeof(id_device)];
    sprintf(id_device_string, "%d", id_device);
    char topic_pub_cmd_default[sizeof(TOPIC_PUB_CMD) + sizeof(DEFAULT_ID_DEVICE) - 2];
    char topic_pub_data_default[sizeof(TOPIC_PUB_DATA) + sizeof(DEFAULT_ID_DEVICE) - 2];
    char topic_pub_cmd[sizeof(TOPIC_PUB_CMD) + sizeof(id_device) - 2];
    char topic_pub_data[sizeof(TOPIC_PUB_DATA) + sizeof(id_device) - 2];

    switch (type_publish)
    {
        case TYPE_CMD_DEFAULT_ID:
            strcpy(topic_pub_cmd_default, TOPIC_PUB_CMD);
            strcat(topic_pub_cmd_default, (const char *)DEFAULT_ID_DEVICE);
            ret = MQTT_IF_Publish(mqtt_client_handle, topic_pub_cmd_default, payload, len, MQTT_QOS_0);
            break;

        case TYPE_DATA_DEFAULT_ID:
            strcpy(topic_pub_data_default, TOPIC_PUB_DATA);
            strcat(topic_pub_data_default, (const char *)DEFAULT_ID_DEVICE);
            ret = MQTT_IF_Publish(mqtt_client_handle, topic_pub_data_default, payload, len, MQTT_QOS_0);
            break;

        case TYPE_CMD:
            strcpy(topic_pub_cmd, TOPIC_PUB_CMD);
            strcat(topic_pub_cmd, id_device_string);
            ret = MQTT_IF_Publish(mqtt_client_handle, topic_pub_cmd, payload, len, MQTT_QOS_0);
            break;

        case TYPE_DATA:
            strcpy(topic_pub_data, TOPIC_PUB_DATA);
            strcat(topic_pub_data, (const char *)id_device_string);
            ret = MQTT_IF_Publish(mqtt_client_handle, topic_pub_data, payload, len, MQTT_QOS_0);
            break;

        default:
            LOG_ERROR("Error in switch topic type!!");
            return 0;
    }

    return ret;
}

int send_rssi(void)
{
    int ret;
    uint16_t                length_stat;
    SlDeviceGetStat_t       rssi_status;
    msg_cmd_t               msg_cmd_rssi_status;

    length_stat             = sizeof(SlDeviceGetStat_t);
    ret                     = 0;

    ret = sl_DeviceStatGet(SL_DEVICE_STAT_WLAN_RX, length_stat, &rssi_status);
    if (ret < 0)
    {
        ERR_PRINT("ERROR:%d\tfail to get rssi status!!", ret);
    }
    uint16_t rssi_cmd_len  = (sizeof(cmd_rssi_t) + sizeof(cmd_header_t) + sizeof(msg_header_t));
    msg_cmd_rssi_status.h.id_device         = id_device;
    msg_cmd_rssi_status.h.msg_op            = CMD;
    msg_cmd_rssi_status.h.msg_size          = rssi_cmd_len;
    msg_cmd_rssi_status.p.h.cmd_id          = CMD_RSSI_STATUS;

    memcpy(&msg_cmd_rssi_status.p.p.cmd_rssi.avg_data_ctrl_rssi, &rssi_status.AvarageDataCtrlRssi, sizeof(rssi_status.AvarageDataCtrlRssi));
    memcpy(&msg_cmd_rssi_status.p.p.cmd_rssi.avg_msg_mnt_rssi, &rssi_status.AvarageMgMntRssi, sizeof(rssi_status.AvarageMgMntRssi));
    memcpy(&msg_cmd_rssi_status.p.p.cmd_rssi.rcv_fcs_err_pkt_num, &rssi_status.ReceivedValidPacketsNumber, sizeof(rssi_status.ReceivedValidPacketsNumber));

#ifdef PRINT_RSSI_CMD
    int16_t avg_data_ctrl_rssi      = 0;
    int16_t avg_msg_mnt_rssi        = 0;
    uint32_t rcv_fcs_err_pkt_num    = 0;

    memcpy(&avg_data_ctrl_rssi, &msg_cmd_rssi_status.p.p.cmd_rssi.avg_data_ctrl_rssi, sizeof(msg_cmd_rssi_status.p.p.cmd_rssi.avg_data_ctrl_rssi));
    memcpy(&avg_msg_mnt_rssi, &msg_cmd_rssi_status.p.p.cmd_rssi.avg_msg_mnt_rssi, sizeof(msg_cmd_rssi_status.p.p.cmd_rssi.avg_msg_mnt_rssi));
    memcpy(&rcv_fcs_err_pkt_num, &msg_cmd_rssi_status.p.p.cmd_rssi.rcv_fcs_err_pkt_num, sizeof(&msg_cmd_rssi_status.p.p.cmd_rssi.rcv_fcs_err_pkt_num));

    UART_PRINT ("\nRSSI STATUS:");
    UART_PRINT ("avg_data_ctrl_rssi:\t %d", avg_data_ctrl_rssi);
    UART_PRINT ("avg_msg_mnt_rssi:\t %d", avg_msg_mnt_rssi);
    UART_PRINT ("rcv_fcs_err_pkt_num:\t %d", rcv_fcs_err_pkt_num);
    UART_PRINT ("__________________________________________");
#endif

#ifndef BROKER_NOT_CONNECTED
    ret = mqtt_publish(TYPE_CMD, (char *)&msg_cmd_rssi_status, sizeof(msg_cmd_rssi_status));
#endif

    return ret;
}

int ask_for_id (void)
{
    int ret     = 0;
    msg_cmd_t   msg_cmd;
    uint16_t    ready_cmd_len  = (sizeof(cmd_ready_t) +
                                  sizeof(cmd_header_t) +
                                  sizeof(msg_header_t));

    id_device                   = 255;
    msg_cmd.h.id_device         = id_device;
    msg_cmd.h.msg_op            = CMD;
    msg_cmd.h.msg_size          = ready_cmd_len;
    msg_cmd.p.h.cmd_id          = CMD_READY;

    memcpy(&msg_cmd.p.p.cmd_ready.mac_addr, &mac_address, SIZE_MAC_ADDR);
    msg_cmd.p.p.cmd_ready.time_per_sample = (1 / (F_SAMPLE / DWS_RATE)) * 10 ^ 9;

//    GPIO_write(CONFIG_LED_GREEN, CONFIG_GPIO_LED_ON);
    LOG_INFO("Waiting for id...\n");

    while (id_device == 255)
    {
//        GPIO_toggle(CONFIG_LED_YELLOW);
//        GPIO_toggle(CONFIG_LED_GREEN);
#ifndef BROKER_NOT_CONNECTED

        ret = mqtt_publish((uint16_t)TYPE_CMD_DEFAULT_ID, (char *) &msg_cmd, (unsigned short)ready_cmd_len);
        CHECK_ERROR(ret, "Error in publish mqtt!!");

#endif
        usleep(750000);
#ifdef BROKER_NOT_RESPONSE
        id_device = 257;
#endif
    }

    return ret;
}
