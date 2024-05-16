/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iot_import.h"
#include "iot_export.h"

#include "iot_export_mqtt.h"

#if CONFIG_SHELL_ASYNCLOG
#include "components/shell_task.h"
int handle_shell_input2(char *inbuf, int in_buf_size, char * outbuf, int out_buf_size);
#endif

#include <os/os.h>
#include <os/str.h>
#include <components/system.h>

#define PRODUCT_KEY             "aclsemi"
#define DEVICE_NAME             "bk7256"
#define DEVICE_SECRET           "nMwWRZrjupURGSByK7qu3uCwzEYUHORu"

// These are pre-defined topics
#define TOPIC_UPDATE            "/"PRODUCT_KEY"/"DEVICE_NAME"/update"
#define TOPIC_ERROR             "/"PRODUCT_KEY"/"DEVICE_NAME"/update/error"
#define TOPIC_GET               "/"PRODUCT_KEY"/"DEVICE_NAME"/get"
#define TOPIC_DATA              "/"PRODUCT_KEY"/"DEVICE_NAME"/data"
#define TOPIC_CMD               "/"PRODUCT_KEY"/"DEVICE_NAME"/cmd"
#define TOPIC_RESP               "/"PRODUCT_KEY"/"DEVICE_NAME"/resp"

#define MSG_LEN_MAX             (2048)
#define MSG_BUF_LEN             (128)
#define DEVICE_MAC_MAXLEN            (16)

static int is_subscribed = 0;
static char mac_str[DEVICE_MAC_MAXLEN] = {0};

static int cnt = 0;
void *gpclient;
static char *msg_buf = NULL, *msg_readbuf = NULL, *user_topic = NULL, *user_topic_resp = NULL;
static char *s_host_name = NULL, *s_username = NULL, *s_password = NULL;

char *mqtt_get_mac_str()
{
    uint8_t mac[BK_MAC_ADDR_LEN] = {0};

    if (0x0 == mac_str[0]) {
        bk_get_mac(mac, MAC_TYPE_BASE);
        HAL_Snprintf(mac_str, sizeof(mac_str), "%02x%02x%02x%02x%02x%02x",
                     mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    }

    return mac_str;
}

char *mqtt_get_host_name() {
    return s_host_name;
}

char *mqtt_get_username() {
    return s_username;
}

char *mqtt_get_password() {
    return s_password;
}

beken_mutex_t g_publish_mutex = NULL;

static void mqtt_publish_data(void *pclient);
int mqtt_client_example(const char *host_name, const char *username,
                        const char *password, const char *topic);

void release_buff() {
    if (NULL != msg_buf) {
        LITE_free(msg_buf);
        msg_buf = NULL;
    }

    if (NULL != msg_readbuf) {
        LITE_free(msg_readbuf);
        msg_readbuf = NULL;
    }

    if (NULL != user_topic) {
        LITE_free(user_topic);
        user_topic = NULL;
    }

    if(NULL != user_topic_resp) {
        LITE_free(user_topic_resp);
        user_topic_resp = NULL;
    }

    if(NULL != s_host_name) {
        LITE_free(s_host_name);
        s_host_name = NULL;
    }

    if(NULL != s_username) {
        LITE_free(s_username);
        s_username = NULL;
    }

    if(NULL != s_password) {
        LITE_free(s_password);
        s_password = NULL;
    }
}

// static void wifi_service_event(void *event, void *priv_data) {
//     // if (event->type != EV_WIFI) {
//     //     return;
//     // }

//     // if (event->code != CODE_WIFI_ON_GOT_IP) {
//     //     return;
//     // }
//     log_info("wifi_service_event!");
//     mqtt_client_example();
// }



static int mqtt_publish_resp_data(void *pclient, const char* buffer, uint32_t len)
{
    int rc = 0;
    char *topic_resp = TOPIC_RESP;
    iotx_mqtt_topic_info_t topic_msg = {0};

    rtos_lock_mutex(&g_publish_mutex);

    /* Initialize topic information */
    topic_msg.qos = IOTX_MQTT_QOS0;
    topic_msg.retain = 0;
    topic_msg.dup = 0;

    topic_msg.payload = (void *)buffer;
    topic_msg.payload_len = len;

    if (NULL != user_topic_resp) {
        topic_resp = user_topic_resp;
    }

    log_info("publish topic(%s).\n", topic_resp);

    rc = IOT_MQTT_Publish(pclient, topic_resp, &topic_msg);
    if (rc < 0) {
        log_info("error occur when publish,rc(%d).\n", rc);
    }
#ifdef MQTT_ID2_CRYPTO
    log_info("packet-id=%u, publish topic msg='0x%02x%02x%02x%02x'...\n",
            (uint32_t)rc,
            buffer[0], buffer[1], buffer[2], buffer[3]);
#else
    log_info("packet-id=%u, publish topic msg=%s.\n", (uint32_t)rc, buffer);
#endif

    rtos_unlock_mutex(&g_publish_mutex);
    return rc;
}

static void mqtt_cmd_msg_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt) msg->msg;

    char buffer[MSG_BUF_LEN + 1] = {0};
    char *recv_msg = NULL;

    int resp_len = 0;
    char *mac_str = mqtt_get_mac_str();
    int mac_len = 0;

    // print topic name and topic message
    log_info("----\n");
    log_info("Topic: '%.*s' (Length: %d).\n",
                  ptopic_info->topic_len,
                  ptopic_info->ptopic,
                  ptopic_info->topic_len);
    log_info("Payload: '%.*s' (Length: %d).\n",
                  ptopic_info->payload_len,
                  ptopic_info->payload,
                  ptopic_info->payload_len);
    log_info("----\n");

    recv_msg = os_malloc(ptopic_info->payload_len + 1);
    if (NULL == recv_msg) {
        log_info("os malloc failed.\n");
        return;
    }
    memcpy(recv_msg, ptopic_info->payload, ptopic_info->payload_len);
    recv_msg[ptopic_info->payload_len] = '\0';

#if CONFIG_SHELL_ASYNCLOG
    handle_shell_input2((char *)recv_msg, ptopic_info->payload_len, buffer, MSG_BUF_LEN);
#endif

    resp_len = strlen(buffer);

    cnt++;

    mac_len = os_snprintf(buffer + resp_len, MSG_BUF_LEN - resp_len, "[%s_%ld]", mac_str, cnt);

    mqtt_publish_resp_data(pclient, buffer, resp_len + mac_len);

    os_free(recv_msg);
}

// static void mqtt_service_event(void *event, void *priv_data) {
//     log_info("mqtt_service_event!.\n");
// }

/*
 * Subscribe the topic: IOT_MQTT_Subscribe(pclient, topic, IOTX_MQTT_QOS1, mqtt_cmd_msg_handle, NULL);
 * Subscribe the topic: IOT_MQTT_Subscribe(pclient, TOPIC_CMD, IOTX_MQTT_QOS1, mqtt_cmd_msg_handle, NULL);
 */
static void mqtt_subcribe_topic(void *pclient, const char *topic) {
    int rc = -1;

    if(is_subscribed == 0) {

        if (NULL != topic) {
            /* Subscribe the specific topic */
            rc = IOT_MQTT_Subscribe(pclient, topic, IOTX_MQTT_QOS0, mqtt_cmd_msg_handle, NULL);
            if (rc < 0) {
                // IOT_MQTT_Destroy(&pclient);
                log_info("IOT_MQTT_Subscribe() topic(%s) failed, rc = %d.\n", topic, rc);
            }
        } else {
            /* Subscribe the specific topic */
            rc = IOT_MQTT_Subscribe(pclient, TOPIC_CMD, IOTX_MQTT_QOS0, mqtt_cmd_msg_handle, NULL);
            if (rc < 0) {

                log_info("IOT_MQTT_Subscribe() TOPIC_CMD failed, rc = %d.\n", rc);
            }
        }

        is_subscribed = 1;
    }
}

/*
 * Publish the topic: IOT_MQTT_Publish(pclient, TOPIC_DATA, &topic_msg);
 */
static void mqtt_publish_data(void *pclient) {
    iotx_mqtt_topic_info_t topic_msg;
    char msg_pub[MSG_BUF_LEN] = {0};
    int rc = -1;

    /* Initialize topic information */
    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));

    topic_msg.qos = IOTX_MQTT_QOS0;
    topic_msg.retain = 0;
    topic_msg.dup = 0;

    /* Generate topic message */
    int msg_len = snprintf(msg_pub, sizeof(msg_pub), "{\"attr_name\":\"temperature\", \"attr_value\":\"%d\"}", cnt);
    if (msg_len < 0) {
        log_info("Error occur! Exit program.\n");
    }

    topic_msg.payload = (void *)msg_pub;
    topic_msg.payload_len = msg_len;

    rc = IOT_MQTT_Publish(pclient, TOPIC_DATA, &topic_msg);
    if (rc < 0) {
        log_info("error occur when publish.\n");
    }
#ifdef MQTT_ID2_CRYPTO
    log_info("packet-id=%u, publish topic msg='0x%02x%02x%02x%02x'...\n",
            (uint32_t)rc,
            msg_pub[0], msg_pub[1], msg_pub[2], msg_pub[3]);
#else
    log_info("packet-id=%u, publish topic msg=%s.\n", (uint32_t)rc, msg_pub);
#endif

}



void event_handle_mqtt(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;
    iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;

    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_UNDEF:
            log_info("undefined event occur.\n");
            break;

        case IOTX_MQTT_EVENT_DISCONNECT:
            log_info("MQTT disconnect.\n");
            break;

        case IOTX_MQTT_EVENT_RECONNECT:
            log_info("MQTT reconnect.\n");
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
            log_info("subscribe success, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
            log_info("subscribe wait ack timeout, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
            log_info("subscribe nack, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
            log_info("unsubscribe success, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
            log_info("unsubscribe timeout, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
            log_info("unsubscribe nack, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
            log_info("publish success, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
            log_info("publish timeout, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_NACK:
            log_info("publish nack, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_RECVEIVED:
            log_info("topic message arrived but without any related handle:\n");
            log_info("    topic=%.*s,\n",
                          topic_info->topic_len,
                          topic_info->ptopic);
            log_info("    topic_msg=%.*s.\n",
                          topic_info->payload_len,
                          topic_info->payload);
            break;

        default:
            log_info("Should NOT arrive here.\n");
            break;
    }
}

/*
 * initialization parameter: mqtt_params
 */
int mqtt_client_example(const char *host_name, const char *username,
                        const char *password, const char *topic)
{
    int rc = 0;
    int topic_len = 0;
    iotx_conn_info_pt pconn_info;
    iotx_mqtt_param_t mqtt_params;

    if (msg_buf != NULL) {
        return rc;
    }

    if(kNoErr != rtos_init_mutex(&g_publish_mutex)){
        log_info("init mutex failed.\n");
        rc = -1;
        return rc;
    }

    do{
        if (NULL != topic) {
            topic_len = strlen(topic);
            if (NULL == (user_topic = (char *)LITE_malloc(topic_len + 1))) {
                log_info("not enough memory.\n");
                rc = -1;
                break;
            }
            memset(user_topic, 0x0, topic_len + 1);
            memcpy(user_topic, topic, topic_len);

            if (NULL == (user_topic_resp = (char *)LITE_malloc(topic_len + 6))) {
                log_info("not enough memory.\n");
                rc = -1;
                break;
            }
            memset(user_topic_resp, 0x0, topic_len + 6);
            os_snprintf(user_topic_resp, topic_len + 6, "%s/%s", user_topic, "resp");
        }

        if(NULL != host_name) {
            int host_name_len = strlen(host_name);
            if (NULL == (s_host_name = (char *)LITE_malloc(host_name_len + 1))) {
                log_info("not enough memory.\n");
                rc = -1;
                break;
            }
            memset(s_host_name, 0x0, host_name_len + 1);
            memcpy(s_host_name, host_name, host_name_len);
        }

        if(NULL != username) {
            int username_len = strlen(username);
            if (NULL == (s_username = (char *)LITE_malloc(strlen(username) + 1))) {
                log_info("not enough memory.\n");
                rc = -1;
                break;
            }
            memset(s_username, 0x0, username_len + 1);
            memcpy(s_username, username, username_len);
        }

        if(NULL != password) {
            int pwd_len = strlen(password);
            if (NULL == (s_password = (char *)LITE_malloc(pwd_len + 1))) {
                log_info("not enough memory.\n");
                rc = -1;
                break;
            }
            memset(s_password, 0x0, pwd_len + 1);
            memcpy(s_password, password, pwd_len);
        }

        if (NULL == (msg_buf = (char *)LITE_malloc(MSG_LEN_MAX))) {
            log_info("not enough memory.\n");
            rc = -1;
            break;
        }

        if (NULL == (msg_readbuf = (char *)LITE_malloc(MSG_LEN_MAX))) {
            log_info("not enough memory.\n");
            rc = -1;
            break;
        }

        /* Device AUTH */
        if (0 != IOT_SetupConnInfo(PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, (void **)&pconn_info)) {
            log_info("AUTH request failed!\n");
            rc = -1;
            break;
        }


        /* Initialize MQTT parameter */
        memset(&mqtt_params, 0x0, sizeof(mqtt_params));

        mqtt_params.port = pconn_info->port;
        mqtt_params.host = pconn_info->host_name;
        mqtt_params.client_id = pconn_info->client_id;
        mqtt_params.username = pconn_info->username;
        mqtt_params.password = pconn_info->password;
        mqtt_params.pub_key = pconn_info->pub_key;

        mqtt_params.request_timeout_ms = 3000;
        mqtt_params.clean_session = 0;
        mqtt_params.keepalive_interval_ms = 60000;
        mqtt_params.pread_buf = msg_readbuf;
        mqtt_params.read_buf_size = MSG_LEN_MAX;
        mqtt_params.pwrite_buf = msg_buf;
        mqtt_params.write_buf_size = MSG_LEN_MAX;

        mqtt_params.handle_event.h_fp = event_handle_mqtt;
        mqtt_params.handle_event.pcontext = NULL;


        /* Construct a MQTT client with specify parameter */
        gpclient = IOT_MQTT_Construct(&mqtt_params);
    }while(0);


    if (NULL == gpclient || 0 != rc) {
        log_info("MQTT construct failed.\n");
        rc = -1;
        release_buff();
    } else{
        mqtt_subcribe_topic(gpclient, user_topic);
        mqtt_publish_data(gpclient);
    }

    return rc;
}

// void deinit_mqtt_client(void) {
//     if(NULL != gpclient) {
//         if(NULL != user_topic) {
//             IOT_MQTT_Unsubscribe(gpclient, user_topic);
//         } else {
//             IOT_MQTT_Unsubscribe(gpclient, TOPIC_CMD);
//         }

//         rtos_delay_milliseconds(200);

//         IOT_MQTT_Destroy(&gpclient);
//         gpclient = NULL;

//         release_buff();

//         rtos_deinit_mutex(&g_publish_mutex);
//         g_publish_mutex = NULL;

//         is_subscribed = 0;
//     }
// }

// normal case for mqtt subscribe & publish
void test_mqtt_start(const char *host_name, const char *username,
                     const char *password, const char *topic)
{
    int ret = 0;
    if(NULL == gpclient) {
        ret = mqtt_client_example(host_name, username, password, topic);
        log_info("test_mqtt_start ret: %d.\n", ret);
    } else {
        // deinit_mqtt_client();
    }
}


int mqtt_cmd_msg_send(char *topic, char *msg)
{
    int rc = 0;
    // char *topic_resp = TOPIC_RESP;
    iotx_mqtt_topic_info_t topic_msg = {0};

    if (NULL == gpclient) {
        log_warning("gpclient is null, please start mqtt first.\n");
        return -1;
    }

    if (NULL == topic || NULL == msg) {
        log_warning("invalid para, topic/msg is null.\n");
        return -1;
    }

    rtos_lock_mutex(&g_publish_mutex);

    /* Initialize topic information */
    topic_msg.qos = IOTX_MQTT_QOS0;
    topic_msg.retain = 0;
    topic_msg.dup = 0;

    topic_msg.payload = (void *)msg;
    topic_msg.payload_len = strlen(msg);

    log_info("publish topic(%s).\n", topic);

    rc = IOT_MQTT_Publish(gpclient, topic, &topic_msg);
    if (rc < 0) {
        log_info("error occur when publish,rc(%d).\n", rc);
    }
#ifdef MQTT_ID2_CRYPTO
    log_info("packet-id=%u, publish topic msg='0x%02x%02x%02x%02x'...\n",
            (uint32_t)rc,
            msg[0], msg[1], msg[2], msg[3]);
#else
    log_info("packet-id=%u, publish topic msg=%s.\n", (uint32_t)rc, msg);
#endif

    rtos_unlock_mutex(&g_publish_mutex);
    return rc;
}
