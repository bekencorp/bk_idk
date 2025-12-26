#ifndef _MQTT_CLIENT_CORE_H__
#define _MQTT_CLIENT_CORE_H__

#include "MQTTPacket.h"

#include <common/bk_include.h>
#include "bk_list.h"
#include "bk_compiler.h"

#define MAX_PACKET_ID           65535 /* according to the MQTT specification - do not change! */

#define MQTT_LOCK_NEVER_TIMEOUT                (0xFFFFFFFFU)
#define MQTT_LOCK_WAIT_FOREVER                 (0xFFFFFFFFU)
#define MQTT_LOCK_NO_WAIT                      (0)

#define MQTT_DEFUALT_BUF_SIZE                  (512)
#define MQTT_DEF_CMD_TIMEOUT_MS                (6000)


#if !defined(MAX_MESSAGE_HANDLERS)
#define MAX_MESSAGE_HANDLERS 5 /* redefinable - how many subscriptions do you want? */
#endif

enum returnCode { BUFFER_OVERFLOW = -2, FAILURE = -1, SUCCESS = 0 };
enum { PAHO_BUFFER_OVERFLOW = -2, PAHO_FAILURE = -1, PAHO_SUCCESS = 0 };


#define MQTT_OK                 ( 0)
#define MQTT_ERR                (-1)
#define MQTT_DIS_PKT_SD_FAILD   (-5000)
#define MQTT_NET_IS_DISCON      (-6600)   ///mqtt net is disconnect
#define MQTT_NET_SD_TIMEOUT     (-6601)
#define MQTT_NET_SD_LEN_UNDER_G (-6602)



#define TMQTT_LOG               bk_printf
#if 1
__ALIGN4 enum QoS
{
	QOS0,
	QOS1,
	QOS2
};
#else
enum QoS { QOS0, QOS1, QOS2, SUBFAIL = 0x80 };
#endif

typedef enum{
	MQTT_UNDEF_EV = 0x0,
	MQTT_OFFLINE_EV    = 0x01,

	MQTT_PUBACK_EV,
	MQTT_SUBACK_EV,
	MQTT_CONNACK_EV,
	
	MQTT_NET_DISCONNECT_EV = 0x10,
}MQTT_EVNT_T;
typedef struct tmqtt_client_netport tmqtt_client_netport;

struct tmqtt_netport_api
{
	int (*mqtt_net_connect)(tmqtt_client_netport *np, unsigned char *host, int port,unsigned int timeout);
	int (*mqtt_read) (tmqtt_client_netport *np, unsigned char *msg, int mlen, unsigned int timeout);
	int (*mqtt_write) (tmqtt_client_netport *np, unsigned char *msg, int mlen,unsigned int timeout);
	void (*disconnect) (tmqtt_client_netport *np);
};

struct tmqtt_client_netport
{
	int socket;
	void *session;
	struct tmqtt_netport_api *net_api;
};

typedef struct MQTTMessage
{
    enum QoS qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} MQTTMessage;

typedef struct MessageData
{
    MQTTMessage *message;
    MQTTString *topicName;
} MessageData;

typedef struct mqtt_client_session mqtt_client_session;

/** Message handlers are indexed by subscription topic **/
struct sub_msg_handlers
{
	const char* topicFilter;
	void (*callback) (mqtt_client_session *,MessageData*);
	enum QoS qos;
}; 	

struct mqtt_client_session
{
	LIST_HEADER_T node;

	void *lock;
	
    unsigned int next_packetid;
	
    unsigned int command_timeout_ms;

    unsigned net_is_connected : 1;
    unsigned is_connected : 1;
    unsigned cleansession : 1;
	
	
	unsigned char *buf,*readbuf;
	unsigned short buf_size,readbuf_size;
	
	unsigned int keepAliveInterval;
	unsigned int tick_ping;
	
	tmqtt_client_netport netport;
	
	void (*connect_callback)(mqtt_client_session *);
    void (*online_callback)(mqtt_client_session *);
    void (*offline_callback)(mqtt_client_session *,MQTT_EVNT_T);
	void (*mqtt_notice_cb)(mqtt_client_session *,MQTT_EVNT_T);
	
	struct{
		unsigned short sub_topic_num;
		struct sub_msg_handlers *messageHandlers;
	}sub;
	
   void (*defaultMessageHandler)(mqtt_client_session *, MessageData *);

   LIST_HEADER_T msg_hd;
} ;





extern void mqtt_core_handler(void);

extern int mqtt_client_subscribe(struct mqtt_client_session *cs,char *topic);
extern int mqtt_client_publish(mqtt_client_session *client, enum QoS qos, const char *topic, const char *msg_str);

extern int matt_client_connect(mqtt_client_session* cs, MQTTPacket_connectData* options);
extern int mqtt_client_disconnect(mqtt_client_session* cs);

extern int mqtt_client_session_init(mqtt_client_session* cs);
extern int mqtt_client_session_init2(mqtt_client_session* cs,unsigned char *send_buf,
					unsigned int sdb_len,unsigned char *read_buf,unsigned int rdb_len);
extern int mqtt_client_session_deinit(mqtt_client_session* cs);


extern int mqtt_net_connect(mqtt_client_session* cs,char *host,int port);
extern int mqtt_net_disconnect(mqtt_client_session* cs);

#endif

