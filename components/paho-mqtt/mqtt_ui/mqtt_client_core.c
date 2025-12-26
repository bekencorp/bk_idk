#include "mqtt_client_core.h"
#include "mqtt_client_com_port.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#include <stdio.h>
#include <string.h>
#include "os/os.h"
#include "os/mem.h"

#define PUB_SD		  1

typedef struct mqtt_msg_queue{
	LIST_HEADER_T node;
	unsigned type : 8;
	unsigned len : 24;
	char *msg;
}mqtt_msg_queue_t;

static LIST_HEAD_DEFINE( mqtt_hd );

static int get_mqtt_next_packet_id(mqtt_client_session* cs)
{
	cs->next_packetid = (cs->next_packetid == MAX_PACKET_ID) ? 1 : cs->next_packetid + 1;
    return cs->next_packetid;
}

static void mqtt_client_notice_discon(mqtt_client_session* cs)
{
	if (cs->offline_callback)
	{
		cs->offline_callback(cs,MQTT_OFFLINE_EV);
	}
}

static int mqtt_send_packet(mqtt_client_session* cs, int length, unsigned int timeout)
{
    int rc = FAILURE,sent = 0;

    while (sent < length )
    {
        rc = cs->netport.net_api->mqtt_write(&cs->netport, &cs->buf[sent], length - sent,timeout);
        if (rc < 0)  // there was an error writing the data
        {
			break;
        }
        sent += rc;
		if(rc == 0)
		{
			break;	

		}
    }
	
    if (sent == length)
    {
        rc = SUCCESS;
    }
    else
    {
        rc = FAILURE;
    }
	
    return rc;
}

static int mqtt_client_send_disconnect_packet(mqtt_client_session* cs)
{
	int rc = PAHO_FAILURE;
	int len = 0;

	len = MQTTSerialize_disconnect(cs->buf, cs->buf_size);
    if (len > 0)
	{
		int irq_level = rtos_enter_critical();

		if(!list_empty( &cs->node))
		{
			list_del_init( &cs->node );
		}
		rtos_exit_critical(irq_level);
	
        rc = mqtt_send_packet(cs, len,cs->command_timeout_ms);            // send the disconnect packet
        if(rc == SUCCESS)
		{
			return MQTT_OK;
        }
		else
		{ 
			return MQTT_DIS_PKT_SD_FAILD;
        }
    }
	
	return MQTT_ERR;
}

static int decodePacket(mqtt_client_session* cs, int* value, int timeout)
{
    unsigned char i;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
		
        rc = cs->netport.net_api->mqtt_read(&cs->netport, &i, 1, timeout);
        if (rc != 1){
			TMQTT_LOG("decodePacket READ failed\r\n");
            goto exit;
        }
        *value += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);
	
exit:
    return len;
}

static int mqtt_read_packet(mqtt_client_session *cs, unsigned int timeout)
{
    int rc = PAHO_FAILURE;
    MQTTHeader header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
	rc = cs->netport.net_api->mqtt_read(&cs->netport, cs->readbuf, 1,timeout);
    if (rc != 1){
		TMQTT_LOG("mqtt_read_packet READ failed\r\n");
        goto exit;
    }
    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    decodePacket(cs, &rem_len, timeout);
    len += MQTTPacket_encode(cs->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
	rc = cs->netport.net_api->mqtt_read(&cs->netport, cs->readbuf + len, rem_len,timeout);
    if (rem_len > 0 && (rc != rem_len)){
		TMQTT_LOG("mqtt_read_packet READ2 failed\r\n");
        goto exit;
    }

    header.byte = cs->readbuf[0];
    rc = header.bits.type;

exit:
    return rc;
}

/**
 * This function subscribe specified mqtt topic.
 *
 * @param c the pointer of MQTT context structure
 * @param topicFilter topic filter name
 * @param qos requested QoS
 *
 * @return the error code, 0 on subscribe successfully.
 */
static int MQTTSubscribe(mqtt_client_session* cs, const char *topicFilter, enum QoS qos)
{
    int rc = PAHO_FAILURE;
    int len = 0;
    int qos_sub = qos;
    MQTTString topic = MQTTString_initializer;
    topic.cstring = (char *)topicFilter;

    if (!cs->is_connected)
	{
		TMQTT_LOG("mqtt client session(%x) is_connected:%d\n", cs, cs->is_connected);
        goto _exit;
    }
	
    len = MQTTSerialize_subscribe(cs->buf, cs->buf_size, 0, get_mqtt_next_packet_id(cs), 1, &topic, &qos_sub);
    if (len <= 0)
	{
		TMQTT_LOG("%d %s error:%d\n", __LINE__,__FUNCTION__,len);
		goto _exit;
    }
	
    if ((rc = mqtt_send_packet(cs, len,cs->command_timeout_ms)) != PAHO_SUCCESS) // send the subscribe packet
    {
    	TMQTT_LOG("%d %s error\n", __LINE__,__FUNCTION__);
    	goto _exit;             // there was a problem
    }
	
    rc = mqtt_read_packet(cs,(cs->command_timeout_ms > 5000)?cs->command_timeout_ms:5000);
    if (rc < 0)
    {
        TMQTT_LOG("MQTTPacket_readPacket MQTTConnect fail\n");
        goto _exit;
    }

    if (rc == SUBACK)      // wait for suback
    {
        int count = 0, grantedQoS = -1;
        unsigned short mypacketid;

        if (MQTTDeserialize_suback(&mypacketid, 1, &count, &grantedQoS, cs->readbuf, cs->readbuf_size) == 1)
            rc = grantedQoS; // 0, 1, 2 or 0x80

        if (rc != 0x80)
        {
            rc = 0;
        }
    }
    else
        rc = PAHO_FAILURE;

_exit:
    return rc;
}

static int mqtt_pub_local_send(mqtt_client_session *cs, const void *data, int len)
{
	mqtt_msg_queue_t *mqt = (mqtt_msg_queue_t*)data;

	if(mqt == NULL)
		return MQTT_ERR;

	mqt->node.next = &mqt->node;
	mqt->node.prev = &mqt->node;
	
	///rtos_enter_critical();
	list_add_tail(&mqt->node,&cs->msg_hd);
	///rtos_exit_critical();
	
    return MQTT_OK;
}

/**
 * This function publish message to specified mqtt topic.
 * [MQTTMessage] + [payload] + [topic] + '\0'
 *
 * @param c the pointer of MQTT context structure
 * @param topicFilter topic filter name
 * @param message the pointer of MQTTMessage structure
 *
 * @return the error code, 0 on subscribe successfully.
 */
static int MQTTPublish(mqtt_client_session *c, const char *topicName, MQTTMessage *message)
{
    int ret = MQTT_ERR;
    int msg_len;
    char *data = 0;
	mqtt_msg_queue_t *mqt = NULL;

	if(mqtt_senssion_lock(c->lock,c->command_timeout_ms) != MQTT_OK)
	{
		return MQTT_ERR;
	}
	
    if (!c->is_connected)
	{
        goto exit;
    }
	
    msg_len = sizeof(MQTTMessage) + message->payloadlen + strlen(topicName) + 1;
    mqt = os_malloc(sizeof(mqtt_msg_queue_t) + msg_len);
    if (!mqt)
	{
		TMQTT_LOG("MQTTPublish malloc error\r\n");
        goto exit;
    }
	
	mqt->type = PUB_SD;
	mqt->len = msg_len;
	data = ((char*)mqt) + sizeof(mqtt_msg_queue_t);
	data[msg_len - 1] = '\0';
	mqt->msg = data;
	
    memcpy(data, message, sizeof(MQTTMessage));
    memcpy(data + sizeof(MQTTMessage), message->payload, message->payloadlen);
    strcpy(data + sizeof(MQTTMessage) + message->payloadlen, topicName);

    ret = mqtt_pub_local_send(c, mqt, msg_len);
    if (ret != MQTT_OK)
    {
    	TMQTT_LOG("MQTTPublish MQTT_local_send error\r\n");
		goto exit;
    }
	mqtt_senssion_unlock(c->lock);
	
	return MQTT_OK;
exit:
	mqtt_senssion_unlock(c->lock) ;
    if (mqt)
	{
        os_free(mqt);
    }
    return MQTT_ERR;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char *topicFilter, MQTTString *topicName)
{
    char *curf = topicFilter;
    char *curn = topicName->lenstring.data;
    char *curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {
            // skip until we meet the next separator, or end of string
            char *nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
            curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
    };

    return (curn == curn_end) && (*curf == '\0');
}

static void NewMessageData(MessageData *md, MQTTString *aTopicName, MQTTMessage *aMessage)
{
    md->topicName = aTopicName;
    md->message = aMessage;
}

static int deliverMessage(struct mqtt_client_session *cs, MQTTString *topicName, MQTTMessage *message)
{
    int i;
    int rc = PAHO_FAILURE;
	struct sub_msg_handlers *sub;

	BK_ASSERT(cs);
	sub = cs->sub.messageHandlers;
    // we have to find the right message handler - indexed by topic
    for (i = 0; i < cs->sub.sub_topic_num; ++i)
    {
        if (sub[i].topicFilter != 0 && (MQTTPacket_equals(topicName, (char *)sub[i].topicFilter) ||
                isTopicMatched((char *)sub[i].topicFilter, topicName)))
        {
            if (sub[i].callback != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                sub[i].callback(cs,&md);
                rc = PAHO_SUCCESS;
            }
        }
    }

    if (rc == PAHO_FAILURE && cs->defaultMessageHandler != NULL)
    {
        MessageData md;
        NewMessageData(&md, topicName, message);
        cs->defaultMessageHandler(cs, &md);
        rc = PAHO_SUCCESS;
    }

    return rc;
}

static int MQTT_cycle(struct mqtt_client_session *cs)
{
    // read the socket, see what work is due
    int packet_type = mqtt_read_packet(cs,cs->command_timeout_ms);

    int len = 0;
    int rc = PAHO_SUCCESS;

    if (packet_type == -1)
    {
        rc = PAHO_FAILURE;
        goto exit;
    }

    switch (packet_type)
    {
    case CONNACK:
    case PUBACK:
    case SUBACK:
		TMQTT_LOG("packet_type:%x\r\n",packet_type);
		if(packet_type == PUBACK)
		{
			cs->tick_ping = mqtt_get_time();
			cs->mqtt_notice_cb(cs,MQTT_PUBACK_EV);
		}
        break;
    case PUBLISH:
    {
        MQTTString topicName;
        MQTTMessage msg;
        int intQoS;
        if (MQTTDeserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
                                    (unsigned char **)&msg.payload, (int *)&msg.payloadlen, cs->readbuf, cs->readbuf_size) != 1)
        {
            goto exit;
        }
		cs->tick_ping = mqtt_get_time();
        msg.qos = (enum QoS)intQoS;
        deliverMessage(cs, &topicName, &msg);
        if (msg.qos != QOS0)
        {
            if (msg.qos == QOS1)
                len = MQTTSerialize_ack(cs->buf, cs->buf_size, PUBACK, 0, msg.id);
            else if (msg.qos == QOS2)
                len = MQTTSerialize_ack(cs->buf, cs->buf_size, PUBREC, 0, msg.id);
            if (len <= 0)
                rc = PAHO_FAILURE;
            else
                rc = mqtt_send_packet(cs, len,cs->command_timeout_ms);
            if (rc == PAHO_FAILURE)
                goto exit; // there was a problem
        }
        break;
    }
    case PUBREC:
    {
        unsigned short mypacketid;
        unsigned char dup, type;
        if (MQTTDeserialize_ack(&type, &dup, &mypacketid, cs->readbuf, cs->readbuf_size) != 1)
            rc = PAHO_FAILURE;
        else if ((len = MQTTSerialize_ack(cs->buf, cs->buf_size, PUBREL, 0, mypacketid)) <= 0)
            rc = PAHO_FAILURE;
        else if ((rc = mqtt_send_packet(cs, len,cs->command_timeout_ms)) != PAHO_SUCCESS) // send the PUBREL packet
            rc = PAHO_FAILURE; // there was a problem
        if (rc == PAHO_FAILURE)
            goto exit; // there was a problem
        break;
    }
    case PUBCOMP:
        break;
    case PINGRESP:
        cs->tick_ping = mqtt_get_time();
		TMQTT_LOG("PINGRESP-->%dmS\r\n\n",cs->tick_ping);
        break;
    }

exit:
    return rc;
}

static int mqtt_client_subscribe_handler(mqtt_client_session* cs)
{
	int i;
	int rc;
	struct sub_msg_handlers *sub;
	const char *topic;
	
	if(cs == NULL)
	{
		return MQTT_ERR;
	}
	
	sub = cs->sub.messageHandlers;
	for (i = 0; i < cs->sub.sub_topic_num; i++)
    {
        topic = sub[i].topicFilter;

        if(topic == NULL)
            continue;

        rc = MQTTSubscribe(cs, topic, QOS2);
        TMQTT_LOG("Subscribe #%d %s %s!\n", i, topic, (rc < 0) ? ("fail") : ("OK"));

        if (rc != 0)
        {
        	TMQTT_LOG("%s error\r\n",__FUNCTION__);
            goto _mqtt_disconnect;
        }
    }

	return MQTT_OK;
_mqtt_disconnect:
	
	return MQTT_ERR;
}

static int mqtt_send_publish(struct mqtt_client_session *cs,mqtt_msg_queue_t *mqt)
{
    MQTTMessage message;
    MQTTString topic = MQTTString_initializer;
	char *data = NULL;
	int len,rc;
	
	if(cs == NULL || mqt == NULL)
		return MQTT_ERR;
	data = mqt->msg;
	if(data == NULL)
		return MQTT_ERR;

	memcpy(&message,data,sizeof(MQTTMessage));
    message.payload = data + sizeof(MQTTMessage);
    topic.cstring = (char *)data + sizeof(MQTTMessage) + message.payloadlen;

	if (message.qos == QOS1 || message.qos == QOS2)
	{
		message.id = get_mqtt_next_packet_id(cs);
	}

    len = MQTTSerialize_publish(cs->buf, cs->buf_size, 0, message.qos, message.retained, message.id,
                                topic, (unsigned char *)message.payload, message.payloadlen);
    if (len <= 0)
    {
        goto exit;
    }

    if ((rc = mqtt_send_packet(cs, len,cs->command_timeout_ms)) != PAHO_SUCCESS) // send the subscribe packet
    {
        TMQTT_LOG("mqtt_send_publish sendPacket rc: %d", rc);
        goto exit;
    }
	
	return MQTT_OK;
exit:
	TMQTT_LOG("mqtt_send_publish sendPacket failed\r\n");
	return MQTT_ERR;
}

void mqtt_core_handler(void)
{
    int res;
    fd_set readset;
    struct timeval timeout;
	struct mqtt_client_session *cs = NULL;
	LIST_HEADER_T *node;
	int len, rc;
	mqtt_msg_queue_t *mqt = NULL;

	int irq_level = rtos_enter_critical();
	if(list_empty( &mqtt_hd))
	{
		rtos_exit_critical(irq_level);
		return;
	}
	node = mqtt_hd.next;
	cs = list_entry(node,struct mqtt_client_session,node);
	rtos_exit_critical(irq_level);
	if(cs == NULL)
	{
		return;
	}
	
	if(mqtt_senssion_lock(cs->lock,MQTT_LOCK_NO_WAIT) != MQTT_OK)
	{
		return;
	}
	
	if((cs->is_connected == 0)
		||(cs->net_is_connected == 0))
	{
		irq_level = rtos_enter_critical();
		if(!list_empty( &cs->node))
		{
			list_del_init( &cs->node );
		}
		rtos_exit_critical(irq_level);
		goto exit;
	}
	
    irq_level = rtos_enter_critical();
	if(!list_empty( &cs->msg_hd ))
	{
		node = cs->msg_hd.next;
		mqt = list_entry(node,struct mqtt_msg_queue,node);
		list_del_init(&mqt->node);
		node = NULL;
	}
	else
	{
		mqt = NULL;
	}
	rtos_exit_critical(irq_level);

	if(mqt != NULL)
	{
		if(mqt->type == PUB_SD)
		{
			mqtt_send_publish(cs,mqt);
		}
		
		if(mqt != NULL)
		{
			os_free(mqt);
			mqt = NULL;
		}
	}

    timeout.tv_sec = 0;
    timeout.tv_usec = 2000;
	do
	{
	    FD_ZERO(&readset);
	    FD_SET(cs->netport.socket, &readset);

	    /* int select(maxfdp1, readset, writeset, exceptset, timeout); */
	    res = select(cs->netport.socket + 1, &readset, NULL, NULL, &timeout);
	    if (res == 0)
	    {

	    	if(cs->tick_ping + cs->keepAliveInterval <= rtos_get_time())
			{
		        len = MQTTSerialize_pingreq(cs->buf, cs->buf_size);
		        rc = mqtt_send_packet(cs, len,cs->command_timeout_ms);
		        if (rc != 0)
		        {
		            TMQTT_LOG("[%d] send ping rc: %d \n", rtos_get_time(), rc);
					mqtt_client_send_disconnect_packet(cs);
					mqtt_net_disconnect(cs);
		            break;
		        }
				cs->tick_ping = rtos_get_time();
				break;
	    	}
			else
			{
				break;
	    	}
	   } /* res == 0: timeount for ping. */
	   else if (res < 0)
	   {
	        TMQTT_LOG("select res: %d\n", res);
			mqtt_client_send_disconnect_packet(cs);
			mqtt_net_disconnect(cs);
	        break;
	   }
	   
	    if (FD_ISSET(cs->netport.socket, &readset))
	    {
	    	int rc_t;
	        ////TMQTT_LOG("sock FD_ISSET\r\n");
	        rc_t = MQTT_cycle(cs);
	        ///TMQTT_LOG("sock FD_ISSET rc_t : %d\r\n", rc_t);
	        if (rc_t < 0)  
			{
				mqtt_client_send_disconnect_packet(cs);
				mqtt_net_disconnect(cs);
            	break;
	        }
	    }
	}while(0);
		
exit:
	mqtt_senssion_unlock(cs->lock);
}

int matt_client_connect(mqtt_client_session* cs, MQTTPacket_connectData* options)
{
	int ret = MQTT_OK;
	int len;
	int rc = FAILURE;

	cs->command_timeout_ms = (cs->command_timeout_ms == 0) ? 5000 : cs->command_timeout_ms; 
	if(mqtt_senssion_lock(cs->lock,cs->command_timeout_ms) != MQTT_OK)
	{
		return MQTT_ERR;
	}

    ///  MQTTConnackData data;
	if (!cs->net_is_connected)
	{
		ret = MQTT_NET_IS_DISCON;
		goto exit;
	}
	if(cs->is_connected) /* don't send connect packet again if we are already connected */
	{
		ret = MQTT_OK;
		goto exit;
	}
	cs->keepAliveInterval = options->keepAliveInterval;
	cs->cleansession = options->cleansession;

	TMQTT_LOG("netport.socket:%d\r\n",cs->netport.socket);
	
	len = MQTTSerialize_connect(cs->buf, cs->buf_size, options);
	if (len <= 0)
    {
    	TMQTT_LOG("MQTTSerialize_connect failed\r\n");
        goto exit;
    }
	
	rc = mqtt_send_packet(cs, len, cs->command_timeout_ms);
	if (rc != SUCCESS)  /// send the connect packet
	{
		TMQTT_LOG("sendPacket failed\r\n");
		ret = rc;
		goto exit;
	}

	rc = mqtt_read_packet(cs,(cs->command_timeout_ms > 7000) ? cs->command_timeout_ms:7000);
    if (rc < 0)
    {
        TMQTT_LOG("%s MQTTPacket_readPacket fail\n", __FUNCTION__);
        goto exit;
    }

    if (rc == CONNACK)
    {
        unsigned char sessionPresent, connack_rc;

        if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, cs->readbuf, cs->readbuf_size) == 1)
        {
            ret = connack_rc;
			cs->is_connected = 1;
			cs->tick_ping = mqtt_get_time();
			if (cs->connect_callback)
		    {
		        cs->connect_callback(cs);
		    }
        }
        else
        {
            ret = -1;
			goto exit;
        }
    }
    else
	{
        ret = -1;
		TMQTT_LOG("%s MQTT type != CONNACK\n", __FUNCTION__);
		goto exit;
    }
	ret = mqtt_client_subscribe_handler( cs );
	if (cs->online_callback)
	{
		cs->online_callback(cs);
	}
	int irq_level = rtos_enter_critical();
	list_add_tail(&cs->node,&mqtt_hd);
	rtos_exit_critical(irq_level);
	
	mqtt_senssion_unlock(cs->lock) ;

	return ret;
exit:
	if(cs->is_connected == 0)
	{
		mqtt_net_disconnect( cs );
	}
	
	mqtt_senssion_unlock(cs->lock) ;
	return ret;
}

///mqtt protocol disconnect,but it is don't care net connnet status
int mqtt_client_disconnect(mqtt_client_session* cs)
{
    int rc = PAHO_FAILURE;
    int len = 0;
	unsigned int wait_time = cs->command_timeout_ms;

	wait_time = (wait_time > 50)?wait_time:50;
	
	if(mqtt_senssion_lock(cs->lock,wait_time) != MQTT_OK)
	{
		return MQTT_ERR;
	}
	
	int irq_level = rtos_enter_critical();
	if(!list_empty( &cs->node))
	{
		list_del_init( &cs->node );
	}
	rtos_exit_critical(irq_level);

    len = MQTTSerialize_disconnect(cs->buf, cs->buf_size);
    if (len > 0)
	{
        rc = mqtt_send_packet(cs, len,cs->command_timeout_ms);            // send the disconnect packet
        if(rc == len)
		{
			///delay for tcp send ok
			
        }
    }
	cs->is_connected = 0;

	mqtt_senssion_unlock(cs->lock) ;
	mqtt_client_notice_discon(cs);

    return MQTT_OK;
}

/**
 * This function publish message to specified mqtt topic.
 * This is just hanging onto the chain
 * @param c the pointer of MQTT context structure
 * @param qos MQTT QOS type, only support QOS1
 * @param topic topic filter name
 * @param msg_str the pointer of MQTTMessage structure
 *
 * @return the error code, 0 on subscribe successfully.
 */
int mqtt_client_publish(mqtt_client_session *client, enum QoS qos, const char *topic, const char *msg_str)
{
    MQTTMessage message;

    if (qos != QOS1)
    {
        TMQTT_LOG("Not support Qos(%d) config, only support Qos(%d).", qos, QOS1);
        return PAHO_FAILURE;
    }

    message.qos = qos;
	message.dup = 0;
	message.id = 0;
    message.retained = 0;
    message.payload = (void *)msg_str;
    message.payloadlen = strlen(message.payload);

    return MQTTPublish(client, topic, &message);
}

int mqtt_client_subscribe(struct mqtt_client_session *cs,char *topic)
{
	int rc;
	
	if(mqtt_senssion_lock(cs->lock,cs->command_timeout_ms) != MQTT_OK)
	{
		return MQTT_ERR;
	}
	
	rc = MQTTSubscribe(cs, topic, QOS2);
    TMQTT_LOG("Subscribe %s %s!\n",  topic, (rc < 0) ? ("fail") : ("OK"));

    if (rc != 0)
    {
    	TMQTT_LOG("%s error\r\n",__FUNCTION__);
		goto exit;
    }

	mqtt_senssion_unlock(cs->lock);
	
	return MQTT_OK;
exit:
	mqtt_senssion_unlock(cs->lock);
	return MQTT_ERR;
}


int mqtt_client_session_init(mqtt_client_session* cs)
{
	if(cs == NULL)
	{
		return MQTT_ERR;
	}

	memset(cs,0,sizeof(struct mqtt_client_session));
	cs->command_timeout_ms = MQTT_DEF_CMD_TIMEOUT_MS;
	cs->msg_hd.next = &cs->msg_hd;
	cs->msg_hd.prev = &cs->msg_hd;
	cs->node.next = &cs->node;
	cs->node.prev = &cs->node;
	cs->lock = mqtt_senssion_lock_create();
	if(cs->lock == NULL)
	{
		return MQTT_ERR;
	}
	return MQTT_OK;
}

int mqtt_client_session_init2(mqtt_client_session* cs,unsigned char *send_buf,
					unsigned int sdb_len,unsigned char *read_buf,unsigned int rdb_len)
{
	if(cs == NULL)
	{
		return MQTT_ERR;
	}
	
	memset(cs,0,sizeof(struct mqtt_client_session));
	cs->msg_hd.next = &cs->msg_hd;
	cs->msg_hd.prev = &cs->msg_hd;
	cs->node.next = &cs->node;
	cs->node.prev = &cs->node;
	cs->command_timeout_ms = MQTT_DEF_CMD_TIMEOUT_MS;
	cs->buf_size = sdb_len;
	cs->buf = send_buf;
	cs->readbuf_size = rdb_len;
	cs->readbuf = read_buf;
	cs->lock = mqtt_senssion_lock_create();
	if(cs->lock == NULL)
	{
		return MQTT_ERR;
	}
	
	return MQTT_OK;
}

int mqtt_client_session_deinit(mqtt_client_session* cs)
{
	if(cs == NULL)
	{
		return MQTT_ERR;
	}

	if(cs->lock != NULL)
	{
		mqtt_senssion_lock(cs->lock,MQTT_LOCK_NEVER_TIMEOUT);
		mqtt_senssion_lock_destroy(cs->lock);
		cs->lock = NULL;
	}
		
	int irq_level = rtos_enter_critical();
	if(!list_empty( &cs->node))
	{
		list_del_init( &cs->node );
	}
	rtos_exit_critical(irq_level);
	
	mqtt_net_disconnect(cs);
	
	return MQTT_OK;
}

int mqtt_net_connect(mqtt_client_session* cs,char *host,int port)
{
	if( !cs->net_is_connected )
	{
		cs->netport.net_api->mqtt_net_connect((tmqtt_client_netport *)&cs->netport, (unsigned char*)host,port,0);
		TMQTT_LOG("netport.socket:%d\r\n",cs->netport.socket);
		if(cs->netport.socket >= 0)
		{
			cs->net_is_connected = 1;
		}
		else
		{
			return MQTT_ERR;
		}
	}
	return MQTT_OK;
}

int mqtt_net_disconnect(mqtt_client_session* cs)
{
	if(cs && cs->netport.net_api->disconnect)
	{
		cs->netport.net_api->disconnect(&cs->netport);
		cs->net_is_connected = 0;
	}
	return MQTT_OK;
}
// eof

