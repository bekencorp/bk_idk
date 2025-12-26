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
 *******************************************************************************/
#if !defined(_MQTT_FREE_RTOS_H_)
#define _MQTT_FREE_RTOS_H_

#include "lwip/sockets.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define FreeRTOS_setsockopt  			setsockopt
#define FreeRTOS_getsockopt				getsockopt
#define FreeRTOS_recv 		 			recv
#define FreeRTOS_closesocket 			close
#define FreeRTOS_socket					socket
#define FreeRTOS_connect				connect
#define	FreeRTOS_gethostbyname 			netconn_gethostbyname
#define FreeRTOS_htons					htons
#define FreeRTOS_send					send

#define FREERTOS_SO_RCVTIMEO 			SO_RCVTIMEO
#define FRERRTOS_SO_SNDTIMEO			SO_SNDTIMEO
#define FRERRTOS_SO_ERROR				SO_ERROR
#define FREERTOS_AF_INET				AF_INET
#define FREERTOS_SOCK_STREAM			SOCK_STREAM
#define FREERTOS_IPPROTO_TCP			IPPROTO_TCP
#define FREERTOS_SOL_SOCKET				SOL_SOCKET

#define freertos_sockaddr 				sockaddr_in

typedef int xSocket_t;


typedef struct Timer 
{
	TickType_t xTicksToWait;
	TimeOut_t xTimeOut;
} Timer;

typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

typedef struct Mutex
{
	SemaphoreHandle_t sem;
} Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);

typedef struct Thread
{
	TaskHandle_t task;
} Thread;

int ThreadStart(Thread*, void (*fn)(void*), void* arg);
int FreeRTOS_read(Network*, unsigned char*, int, int);
int FreeRTOS_write(Network*, unsigned char*, int, int);
void FreeRTOS_disconnect(Network*);
void NetworkInit(Network*);
int NetworkConnect(Network*, char*, int);
/*int NetworkConnectTLS(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);*/

#endif
