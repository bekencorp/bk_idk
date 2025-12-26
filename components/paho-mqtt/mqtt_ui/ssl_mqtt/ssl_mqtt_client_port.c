#include "ssl_mqtt_client_port.h"
#include "mqtt_client_com_port.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#if 1
#include "sl_tls.h"
#include <string.h>





static int ssl_mqtt_net_connect(tmqtt_client_netport *np, unsigned char *host, int port,unsigned int timeout)
{
	char tls_port[10];
	MbedTLSSession *tls_session = NULL;
	
	snprintf(tls_port,sizeof(tls_port),"%d",port);
	
	tls_session = ssl_create((char *)host,tls_port);
	np->session = tls_session;
	if(tls_session != NULL)
	{
		bk_printf("ssl connect succeed\r\n");
		np->socket = tls_session->server_fd.fd;
		return  np->socket;
	}
	
	return -1;
}


static int ssl_mqtt_read(tmqtt_client_netport *np, unsigned char *msg, int mlen,unsigned int timeout)
{
	int len = 0,readed = 0;
	MbedTLSSession *session = np->session;

	do
	{
		len = ssl_read_data(np->session,msg,mlen,timeout);
		if(len > 0)
		{
			memcpy(&msg[readed],session->buffer,len);
			readed += len;
			len = readed;
		}
		else if(len == 0)
		{
			len = readed;
			break;
		}
		else
		{
			break;
		}
	}while(readed < mlen);
	
	return len;
}


static int ssl_mqtt_write(tmqtt_client_netport *np, unsigned char *msg, int mlen,unsigned int timeout)
{
	int sd_len = 0;

	{
		struct timeval tv;

		tv.tv_sec = timeout/1000;   ///����ʱ��
		tv.tv_usec = (timeout%1000) * 1000;

		setsockopt(np->socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
	}
	
	sd_len = ssl_txdat_sender(np->session,mlen,(char *)msg);
	
	return sd_len;
}

void ssl_mqtt_disconnect(tmqtt_client_netport *np)
{
	if(np && (np->session != NULL) && np->socket >= 0)
	{
		TMQTT_LOG("[MQTT]ssl session %p,%d close\r\n",np->session,np->socket);
		np->socket = -1;
		ssl_close( np->session );
		np->session = NULL;
	}
}

static const struct tmqtt_netport_api ssl_mqtt_netport = {
	.mqtt_net_connect = ssl_mqtt_net_connect,
	.mqtt_read = ssl_mqtt_read,
	.mqtt_write = ssl_mqtt_write,
	.disconnect = ssl_mqtt_disconnect,
};

int ssl_mqtt_client_api_register(tmqtt_client_netport *np)
{
	if( np )
	{
		np->net_api = (struct tmqtt_netport_api *)&ssl_mqtt_netport;
	}
	return MQTT_OK;
}

int ssl_mqtt_client_init(void)
{
	
	return MQTT_OK;
}
#endif
