/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *    Ian Craggs - convert to FreeRTOS
 *******************************************************************************/
#include "MQTTFreeRTOS.h"
#include "lwip/api.h"
#include <os/mem.h>
#include <os/os.h>

int ThreadStart(Thread* thread, void (*fn)(void*), void* arg)
{
	int rc = 0;
	uint16_t usTaskStackSize = (configMINIMAL_STACK_SIZE * 5);
	UBaseType_t uxTaskPriority = uxTaskPriorityGet(NULL); /* set the priority as the same as the calling task*/

	rc = xTaskCreate(fn,	                /* The function that implements the task. */
						"MQTTTask",			/* Just a text name for the task to aid debugging. */
						usTaskStackSize,	/* The stack size is defined in FreeRTOSIPConfig.h. */
						arg,				/* The task parameter, not used in this case. */
						uxTaskPriority,		/* The priority assigned to the task is defined in FreeRTOSConfig.h. */
						&thread->task);		/* The task handle is not used. */

	return rc;
}


void MutexInit(Mutex* mutex)
{
	mutex->sem = xSemaphoreCreateMutex();
}

int MutexLock(Mutex* mutex)
{
	return xSemaphoreTake(mutex->sem, portMAX_DELAY);
}

int MutexUnlock(Mutex* mutex)
{
	return xSemaphoreGive(mutex->sem);
}


void TimerCountdownMS(Timer* timer, unsigned int timeout_ms)
{
	timer->xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	vTaskSetTimeOutState(&timer->xTimeOut); /* Record the time at which this function was entered. */
}


void TimerCountdown(Timer* timer, unsigned int timeout) 
{
	TimerCountdownMS(timer, timeout * 1000);
}


int TimerLeftMS(Timer* timer) 
{
	xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait); /* updates xTicksToWait to the number left */
	return (timer->xTicksToWait < 0) ? 0 : (timer->xTicksToWait * portTICK_PERIOD_MS);
}


char TimerIsExpired(Timer* timer)
{
	return xTaskCheckForTimeOut(&timer->xTimeOut, &timer->xTicksToWait) == pdTRUE;
}


void TimerInit(Timer* timer)
{
	timer->xTicksToWait = 0;
	os_memset(&timer->xTimeOut, '\0', sizeof(timer->xTimeOut));
}


int FreeRTOS_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	TimeOut_t xTimeOut;
	struct timeval time_to_wait = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
	int recvLen = 0;

	vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
	do
	{
		int rc = 0;

		FreeRTOS_setsockopt(n->my_socket, SOL_SOCKET, FREERTOS_SO_RCVTIMEO, &time_to_wait, sizeof(time_to_wait));
		rc = FreeRTOS_recv(n->my_socket, buffer + recvLen, len - recvLen, 0);
		if (rc > 0)
			recvLen += rc;
		else if (rc < 0)
		{
			recvLen = rc;
			break;
		}
	} while (recvLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

	return recvLen;
}


int FreeRTOS_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	TickType_t xTicksToWait = timeout_ms / portTICK_PERIOD_MS; /* convert milliseconds to ticks */
	TimeOut_t xTimeOut;
	struct timeval time_to_wait = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
	int sentLen = 0;

	vTaskSetTimeOutState(&xTimeOut); /* Record the time at which this function was entered. */
	do
	{
		int rc = 0;

		FreeRTOS_setsockopt(n->my_socket, SOL_SOCKET, FREERTOS_SO_RCVTIMEO, &time_to_wait, sizeof(time_to_wait));
		rc = FreeRTOS_send(n->my_socket, buffer + sentLen, len - sentLen, 0);
		if (rc > 0)
			sentLen += rc;
		else if (rc < 0)
		{
			sentLen = rc;
			break;
		}
	} while (sentLen < len && xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdFALSE);

	return sentLen;
}


void FreeRTOS_disconnect(Network* n)
{
	FreeRTOS_closesocket(n->my_socket);
}


void NetworkInit(Network* n)
{
	n->my_socket = 0;
	n->mqttread = FreeRTOS_read;
	n->mqttwrite = FreeRTOS_write;
	n->disconnect = FreeRTOS_disconnect;
}


int NetworkConnect(Network* n, char* name, int port)
{
	int retVal = -1;
	uint32_t ipAddress;
	struct freertos_sockaddr sAddr;

	if (0 != FreeRTOS_gethostbyname(name, (ip_addr_t*)&ipAddress))
	{
		os_printf("gethostbyname_failed\r\n");
		goto exit;
	}

	sAddr.sin_family = FREERTOS_AF_INET;
	sAddr.sin_port = FreeRTOS_htons(port);
	os_memcpy((void *)(&sAddr.sin_addr), (void *)&ipAddress, sizeof(ipAddress));

	if ((n->my_socket = FreeRTOS_socket(FREERTOS_AF_INET, 
											FREERTOS_SOCK_STREAM, 
											FREERTOS_IPPROTO_TCP)) < 0)
	{
		os_printf("socket_failed\r\n");
		goto exit;
	}

	if ((retVal = FreeRTOS_connect(n->my_socket, (const struct sockaddr *)&sAddr, sizeof(sAddr))) < 0)
	{
		FreeRTOS_closesocket(n->my_socket);
		
		os_printf("connect_failed\r\n");
	    goto exit;
	}

exit:
	return retVal;
}
// eof

