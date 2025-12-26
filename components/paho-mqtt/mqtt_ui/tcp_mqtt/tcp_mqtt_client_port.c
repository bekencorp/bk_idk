#include "tcp_mqtt_client_port.h"
#include "mqtt_client_com_port.h"

#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/api.h"
#include <os/os.h>
#include <os/mem.h>

int tcp_mqtt_net_connect(tmqtt_client_netport *np, unsigned char *host, int port,unsigned int timeout)
{
	int skt = -1;

	if(np != NULL)
	{
		int retVal = -1;
		ip_addr_t ipAddress;
		struct sockaddr_in sAddr;

		if(np->socket > 0)
		{
			close(np->socket);
		}
		np->socket = -1;

		if (0 != netconn_gethostbyname((char*)host, &ipAddress))
		{
			TMQTT_LOG("gethostbyname_failed\r\n");
			goto exit;
		}

		sAddr.sin_family = AF_INET;
		sAddr.sin_port = htons(port);
		os_memcpy((void *)(&sAddr.sin_addr), (void *)&ipAddress, sizeof(ipAddress));

		if ((np->socket = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) < 0)
		{
			TMQTT_LOG("socket_failed\r\n");
			goto exit;
		}
		TMQTT_LOG(" create netport.socket:%d\r\n",np->socket);

		if ((retVal = connect(np->socket, (const struct sockaddr *)&sAddr, sizeof(sAddr))) < 0)
		{
			close(np->socket);

			TMQTT_LOG("connect_failed\r\n");
		    goto exit;
		}

		TMQTT_LOG("socket succeed\r\n");
	exit:
		return retVal;
	}

	return skt;
}

int tcp_mqtt_read(tmqtt_client_netport *np, unsigned char *msg, int mlen,unsigned int timeout)
{
	int len = 0;
	unsigned int start_tick,cur_tick;
	
	if( np )
	{
		int bytes = 0;
		int rc;
		
		len = mlen;
		start_tick = mqtt_get_time();
		
		while (bytes < len)
		{
			fd_set readset;
			struct timeval interval;
			int ret;

			if(timeout == 0)
			{
				timeout = 1;
			}

			interval.tv_sec = timeout/1000;
			interval.tv_usec = (timeout%1000) * 1000;

			FD_ZERO(&readset);
			FD_SET(np->socket, &readset);

			ret = select(np->socket + 1, &readset, NULL, NULL, &interval);
			if(ret < 0)
			{
				bytes = -1;
				TMQTT_LOG("net_read %d:%d, break!socket select ret:%d\r\n", bytes, len,ret);
				break;
			}
			else if(ret == 0)
			{
				TMQTT_LOG("net_read %d:%d, break!socket select ret:%d\r\n", bytes, len,ret);
				break;
			}
			rc = recv(np->socket, &msg[bytes], (size_t)(len - bytes), MSG_DONTWAIT);
				
			if (rc == -1)
			{
				if (errno != ENOTCONN && errno != ECONNRESET)
				{
					bytes = -1;
					break;
				}
			}
			else
			{
				bytes += rc;
			}
			
			if (bytes >= len)
			{
				break;
			}
			
			cur_tick = mqtt_get_time() - start_tick;
			if(cur_tick >= timeout)
			{
				break;
			}
			start_tick = mqtt_get_time();
			timeout = timeout - cur_tick;
		}

		return bytes;
	}
	
	return len;
}


int tcp_mqtt_write(tmqtt_client_netport *np, unsigned char *msg, int mlen,unsigned int timeout)
{
	int sd_len = 0;
	
	if( np && np->socket >= 0)
	{
		{
			struct timeval tv;

			tv.tv_sec = timeout/1000;   ///����ʱ��
			tv.tv_usec = (timeout%1000) * 1000;

			setsockopt(np->socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
		}
		
		sd_len = send( np->socket, msg, mlen, 0 );
		if ( sd_len < 0 )
		{
			TMQTT_LOG("[MQTT]tcp send ret:%d error\r\n",sd_len);
			return MQTT_NET_IS_DISCON;
		}

			
	}
	
	return sd_len;
}

void tcp_mqtt_disconnect(tmqtt_client_netport *np)
{
	if(np && np->socket >= 0)
	{
		close( np->socket );
		TMQTT_LOG("[MQTT]tcp socket %d close\r\n",np->socket);
		np->socket = -1;
	}
}

static const struct tmqtt_netport_api tcp_mqtt_netport = {
	.mqtt_net_connect = tcp_mqtt_net_connect,
	.mqtt_read = tcp_mqtt_read,
	.mqtt_write = tcp_mqtt_write,
	.disconnect = tcp_mqtt_disconnect,
};

int tcp_mqtt_client_api_register(tmqtt_client_netport *np)
{
	if( np )
	{
		np->net_api = (struct tmqtt_netport_api *)&tcp_mqtt_netport;
	}
	return MQTT_OK;
}

int tcp_mqtt_client_init(void)
{
	return MQTT_OK;
}

