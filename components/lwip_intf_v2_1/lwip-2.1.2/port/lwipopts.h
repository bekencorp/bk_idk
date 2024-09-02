/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include <common/sys_config.h>
#include <components/log.h>



/**
 * Loopback demo related options.
 */
#define LWIP_NETIF_LOOPBACK             1
#define LWIP_HAVE_LOOPIF                1
#define LWIP_NETIF_LOOPBACK_MULTITHREADING       1
#define LWIP_LOOPBACK_MAX_PBUFS         8

/* Define IP_FORWARD to 1 if you wish to have the ability to forward
   IP packets across network interfaces. If you are going to run lwIP
   on a device with only one network interface, define this to 0. */
#ifdef CONFIG_IP_FORWARD
#define IP_FORWARD                      1
#else
#define IP_FORWARD                      0
#endif

#ifdef CONFIG_IP_NAPT
#define IP_NAPT                         1
#else
#define IP_NAPT                         0
#endif

#define TCPIP_THREAD_NAME               "tcp/ip"
#ifdef CONFIG_KEYVALUE
#define TCPIP_THREAD_STACKSIZE          1024
#else
#define TCPIP_THREAD_STACKSIZE          512
#endif

#if CONFIG_LITEOS_M
#define TCPIP_THREAD_PRIO               7
#else
#define TCPIP_THREAD_PRIO               2
#endif

#define DEFAULT_THREAD_STACKSIZE        200
#if CONFIG_LITEOS_M
#define DEFAULT_THREAD_PRIO             1
#else
#define DEFAULT_THREAD_PRIO             8
#endif

/* Disable lwIP asserts */
#define LWIP_NOASSERT			        1

#if CONFIG_AGORA_IOT_SDK
#define LWIP_DEBUG                      1
#else
#define LWIP_DEBUG                      0
#endif
#define LWIP_DEBUG_TRACE                0
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#define IP_DEBUG                        LWIP_DBG_OFF
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF
#define PBUF_DEBUG                      LWIP_DBG_OFF
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#define ICMP_DEBUG                      LWIP_DBG_OFF
#define IGMP_DEBUG                      LWIP_DBG_OFF
#define INET_DEBUG                      LWIP_DBG_OFF
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#define RAW_DEBUG                       LWIP_DBG_OFF
#define MEM_DEBUG                       LWIP_DBG_OFF
#define SYS_DEBUG                       LWIP_DBG_OFF
#define TCP_DEBUG                       LWIP_DBG_OFF
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#define TCP_FR_DEBUG                    LWIP_DBG_OFF
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF
#define TCP_WND_DEBUG                   LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#define TCP_RST_DEBUG                   LWIP_DBG_OFF
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF
#define UDP_DEBUG                       LWIP_DBG_OFF
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#define PPP_DEBUG                       LWIP_DBG_OFF
#define SLIP_DEBUG                      LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF
#define AUTOIP_DEBUG                    LWIP_DBG_OFF
#define SNMP_MSG_DEBUG                  LWIP_DBG_OFF
#define SNMP_MIB_DEBUG                  LWIP_DBG_OFF
#define DNS_DEBUG                       LWIP_DBG_OFF
#define IP6_DEBUG                       LWIP_DBG_OFF
#define MDNS_DEBUG                      LWIP_DBG_OFF

//#define LWIP_COMPAT_MUTEX      		    1
/**
 * SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#define SYS_LIGHTWEIGHT_PROT            1

#define BK_LWIP                     1

/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/
/**
 * MEM_ALIGNMENT: should be set to the alignment of the CPU
 *    4 byte alignment -> #define MEM_ALIGNMENT 4
 *    2 byte alignment -> #define MEM_ALIGNMENT 2
 */
#define MEM_ALIGNMENT                   4

/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#define MEM_LIBC_MALLOC                  CONFIG_LWIP_MEM_LIBC_MALLOC

/**
 * MEMP_MEM_MALLOC==1: Use mem_malloc/mem_free instead of the lwip pool allocator.
 * Especially useful with MEM_LIBC_MALLOC but handle with care regarding execution
 * speed and usage from interrupts!
 */
#define MEMP_MEM_MALLOC                  CONFIG_LWIP_MEMP_MEM_MALLOC

/*
 * Enable dynamic TX/RX buffer allocation strategy.
 * mem_malloc() will return NULL if the used size is greater than threshold of TX
 * or RX even the remain memory is not empty, it could reserve some
 * necessary memory to maintence the basic function such as receiving
 * TCP ACK on TCP upload scenario or transmitting TCP ACK on TCP
 * download scenario
 */
#define MEM_TRX_DYNAMIC_EN               CONFIG_LWIP_MEM_TRX_DYNAMIC_EN

#define MEMP_STATS                       CONFIG_LWIP_MEMP_STATS
#define MEM_STATS                        CONFIG_LWIP_MEM_STATS


#define MAX_SOCKETS_TCP 8
#define MAX_LISTENING_SOCKETS_TCP 4
#define MAX_SOCKETS_UDP 8
#define TCP_SND_BUF_COUNT 5

/* Value of TCP_SND_BUF_COUNT denotes the number of buffers and is set by
 * CONFIG option available in the SDK
 */
/* Buffer size needed for TCP: Max. number of TCP sockets * Size of pbuf *
 * Max. number of TCP sender buffers per socket
 *
 * Listening sockets for TCP servers do not require the same amount buffer
 * space. Hence do not consider these sockets for memory computation
 */
#define TCP_MEM_SIZE     (MAX_SOCKETS_TCP * \
							PBUF_POOL_BUFSIZE * (TCP_SND_BUF/TCP_MSS))

/* Buffer size needed for UDP: Max. number of UDP sockets * Size of pbuf
 */
#define UDP_MEM_SIZE (MAX_SOCKETS_UDP * PBUF_POOL_BUFSIZE)

/**
 * MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high.
 */
//#if (MEM_TRX_DYNAMIC_EN)
#define MEM_SIZE                        CONFIG_LWIP_MEM_SIZE  //(50*1024)
#define MEM_MAX_TX_SIZE                 CONFIG_LWIP_MEM_MAX_TX_SIZE  //(MEM_SIZE*5)/6
#define MEM_MAX_RX_SIZE                 CONFIG_LWIP_MEM_MAX_RX_SIZE  //(MEM_SIZE*3)/4
//#else
//#if (CONFIG_LWIP_MEM_REDUCE)
//#define MEM_SIZE (32*1024)
//#else
//#define MEM_SIZE (48*1024)
//#endif //CONFIG_LWIP_MEM_REDUCE
//#endif //MEM_TRX_DYNAMIC_EN

/* NAPT options */
#if 0 //IP_NAPT
static int random_mock = -1;
/* Mock the esp-random to return 0 for easier result checking */
uint32_t beken_random(void)
{
  if (random_mock == -1) {
    /* fall back to default "port/unix" rand (if other tests are launched with IP_NAPT ON) */
    return rand();
  }
  return random_mock;
}

#define IP_NAPT_MAX                     16
#define LWIP_RAND() (beken_random())
#include "lwip/arch.h"
u32_t beken_random(void);
#endif /* IP_NAPT */

/* ---------- IP options ---------- */
#define BK_DNS			1
#define LWIP_SIOCOUTQ                  1

#if CONFIG_AGORA_IOT_SDK
#define LWIP_TCPIP_CORE_LOCKING         1
/**
 * LWIP_TCPIP_CORE_LOCKING_INPUT: when LWIP_TCPIP_CORE_LOCKING is enabled,
 * this lets tcpip_input() grab the mutex for input packets as well,
 * instead of allocating a message and passing it to tcpip_thread.
 *
 * ATTENTION: this does not work when tcpip_input() is called from
 * interrupt context!
 */
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1
#endif

/*
   ------------------------------------------------
   ---------- Internal Memory Pool Sizes ----------
   ------------------------------------------------
*/
/**
 * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high.
 */
#define MEMP_NUM_PBUF                   10

/**
 * MEMP_NUM_TCP_PCB: the number of simulatenously active TCP connections.
 * (requires the LWIP_TCP option)
 */
#define MEMP_NUM_TCP_PCB                MAX_SOCKETS_TCP

#define MEMP_NUM_TCP_PCB_LISTEN         MAX_LISTENING_SOCKETS_TCP

/**
 * MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP segments.
 * (requires the LWIP_TCP option)
 */
//#define MEMP_NUM_TCP_SEG                12

/**
 * MEMP_NUM_TCPIP_MSG_INPKT: the number of struct tcpip_msg, which are used
 * for incoming packets.
 * (only needed if you use tcpip.c)
 */

#define MEMP_NUM_TCPIP_MSG_INPKT        32   //default 16

/**
 * MEMP_NUM_SYS_TIMEOUT: the number of simulateously active timeouts.
 * (requires NO_SYS==0)
 */
#define MEMP_NUM_SYS_TIMEOUT            12

/**
 * MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c)
 */
//#if (MEM_TRX_DYNAMIC_EN)
#define MEMP_NUM_NETBUF                 CONFIG_LWIP_MEMP_NUM_NETBUF//32
//#else
//#define MEMP_NUM_NETBUF                 16
//#endif
/**
 * MEMP_NUM_NETCONN: the number of struct netconns.
 * (only needed if you use the sequential API, like api_lib.c)
 *
 * This number corresponds to the maximum number of active sockets at any
 * given point in time. This number must be sum of max. TCP sockets, max. TCP
 * sockets used for listening, and max. number of UDP sockets
 */
#define MEMP_NUM_NETCONN	(MAX_SOCKETS_TCP + \
	MAX_LISTENING_SOCKETS_TCP + MAX_SOCKETS_UDP)



/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
//#if (MEM_TRX_DYNAMIC_EN)
#define PBUF_POOL_SIZE                  CONFIG_LWIP_PBUF_POOL_SIZE//10
//#else
//#define PBUF_POOL_SIZE                  20
//#endif

/*
   ----------------------------------
   ---------- Pbuf options ----------
   ----------------------------------
*/

/**
 * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accomodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header.
 */
#if CONFIG_WIFI6_CODE_STACK
#define PBUF_LINK_ENCAPSULATION_HLEN    CONFIG_MSDU_RESV_HEAD_LENGTH
#if CONFIG_LWIP_RESV_TLEN_ENABLE
#define PBUF_LINK_ENCAPSULATION_TLEN    CONFIG_MSDU_RESV_TAIL_LENGTH
#endif
#define PBUF_POOL_BUFSIZE               (1580 + PBUF_LINK_ENCAPSULATION_HLEN)
#else
#define PBUF_POOL_BUFSIZE               1580
#endif


/*
   ---------------------------------
   ---------- RAW options ----------
   ---------------------------------
*/
/**
 * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
 */
#define LWIP_RAW                        1
#ifdef CONFIG_IPV6
#define LWIP_IPV6                        1
#endif

/* Enable IPv4 Auto IP	*/
#ifdef CONFIG_AUTOIP
#define LWIP_AUTOIP                     1
#define LWIP_DHCP_AUTOIP_COOP           1
#define LWIP_DHCP_AUTOIP_COOP_TRIES		5
#endif


/* Enable MDNS	*/
#ifdef CONFIG_MDNS
#define LWIP_MDNS_RESPONDER             1
#define LWIP_NUM_NETIF_CLIENT_DATA      (LWIP_MDNS_RESPONDER)
#endif

#if CONFIG_BRIDGE
#if !defined LWIP_NUM_NETIF_CLIENT_DATA
#define LWIP_NUM_NETIF_CLIENT_DATA      3
#endif
#endif

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#define LWIP_SOCKET                     1
#define LWIP_NETIF_API			1

/**
 * LWIP_RECV_CB==1: Enable callback when a socket receives data.
 */
#define LWIP_RECV_CB                1
/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#define SO_REUSE                        1
#define SO_REUSE_RXTOALL 				1

/**
 * Enable TCP_KEEPALIVE
 */
#define LWIP_TCP_KEEPALIVE              1

/*
   ----------------------------------------
   ---------- Statistics options ----------
   ----------------------------------------
*/
/**
 * LWIP_STATS==1: Enable statistics collection in lwip_stats.
 */
#define LWIP_STATS                      1

/**
 * LWIP_STATS_DISPLAY==1: Compile in the statistics output functions.
 */
#define LWIP_STATS_DISPLAY              1

/*
   ----------------------------------
   ---------- DHCP options ----------
   ----------------------------------
*/
/**
 * LWIP_DHCP==1: Enable DHCP module.
 */
#define LWIP_DHCP                       1
#define LWIP_NETIF_STATUS_CALLBACK      1

/**
 * DNS related options, revisit later to fine tune.
 */
#define LWIP_DNS                        1
#define DNS_TABLE_SIZE                  2  // number of table entries, default 4
//#define DNS_MAX_NAME_LENGTH           64  // max. name length, default 256
#define DNS_MAX_SERVERS                 2  // number of DNS servers, default 2
#define DNS_DOES_NAME_CHECK             1  // compare received name with given,def 0
#define DNS_MSG_SIZE                    512
#define MDNS_MSG_SIZE                   512
#define MDNS_TABLE_SIZE                 1  // number of mDNS table entries
#define MDNS_MAX_SERVERS                1  // number of mDNS multicast addresses
/* TODO: Number of active UDP PCBs is equal to number of active UDP sockets plus
 * two. Need to find the users of these 2 PCBs
 */
#define MEMP_NUM_UDP_PCB		(MAX_SOCKETS_UDP + 2)
/* NOTE: some times the socket() call for SOCK_DGRAM might fail if you dont
 * have enough MEMP_NUM_UDP_PCB */

/*
   ----------------------------------
   ---------- IGMP options ----------
   ----------------------------------
*/
/**
 * LWIP_IGMP==1: Turn on IGMP module.
 */
#define LWIP_IGMP                       1

/**
 * LWIP_SO_CONTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_CONTIMEO processing.
 */
#define LWIP_SO_CONTIMEO                1

/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#define LWIP_SO_SNDTIMEO                1

/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#define LWIP_SO_RCVTIMEO                1
#define LWIP_SO_SNDTIMEO                1
/**
 * TCP_LISTEN_BACKLOG==1: Handle backlog connections.
 */
#if CONFIG_AGORA_IOT_SDK
//#define TCP_LISTEN_BACKLOG		        1
#else
#define TCP_LISTEN_BACKLOG		        1
#endif
#define LWIP_PROVIDE_ERRNO		        1

#if CONFIG_AGORA_IOT_SDK
//#include <errno.h>
//#define ERRNO				            1

#include "sys/errno.h"
#else
#include <errno.h>
#define ERRNO				            1
#endif
//#define LWIP_SNMP 1


/*
   ------------------------------------------------
   ---------- Network Interfaces options ----------
   ------------------------------------------------
*/
/**
 * LWIP_NETIF_HOSTNAME==1: use DHCP_OPTION_HOSTNAME with netif's hostname
 * field.
 */
#define LWIP_NETIF_HOSTNAME             1


/*
The STM32F107 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.
*/
//#define CHECKSUM_BY_HARDWARE


#ifdef CHECKSUM_BY_HARDWARE
  /* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 0
  /* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                0
  /* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                0
  /* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               0
  /* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              0
  /* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              0
#else
  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP                 1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP                1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP                1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP               1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP              1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP              1
#endif

#if CONFIG_ETH
/* Enable Checksum Per Interface, ETH has HW csum module */
#define LWIP_CHECKSUM_CTRL_PER_NETIF      1
#endif

/**
 * TCP_RESOURCE_FAIL_RETRY_LIMIT: limit for retrying sending of tcp segment
 * on resource failure error returned by driver.
 */
#define TCP_RESOURCE_FAIL_RETRY_LIMIT     50

#define LWIP_TCP_SACK_OUT               CONFIG_LWIP_TCP_SACK_OUT

//#ifdef CONFIG_ENABLE_MXCHIP
/* save memory */
//#if (CONFIG_LWIP_MEM_REDUCE)
#define TCP_MSS                 CONFIG_LWIP_TCP_MSS //(1500 - 40)
/* TCP receive window. */
#define TCP_WND                 CONFIG_LWIP_TCP_WND //(20*TCP_MSS)
/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             CONFIG_LWIP_TCP_SND_BUF //(20*TCP_MSS)

#define TCP_SND_QUEUELEN        CONFIG_LWIP_TCP_SND_QUEUELEN//(40)
//#else
//#define TCP_MSS                 (1500 - 40)
/* TCP receive window. */
//#define TCP_WND                 (10*TCP_MSS)
/* TCP sender buffer space (bytes). */
//#define TCP_SND_BUF             (10*TCP_MSS)

//#define TCP_SND_QUEUELEN        (20)
//#endif

/* ARP before DHCP causes multi-second delay  - turn it off */
#define DHCP_DOES_ARP_CHECK            (0)

#define TCP_MAX_ACCEPT_CONN 5
#define MEMP_NUM_TCP_SEG               CONFIG_LWIP_MEMP_NUM_TCP_SEG//(TCP_SND_QUEUELEN*2)


//#if (CONFIG_LWIP_MEM_REDUCE)
#define DEFAULT_UDP_RECVMBOX_SIZE       CONFIG_LWIP_UDP_RECVMBOX_SIZE  // //each udp socket max buffer 24 packets.
//#else
//#define DEFAULT_UDP_RECVMBOX_SIZE       16 //each udp socket max buffer 16 packets.
//#endif

#define TCP_MSL (TCP_TMR_INTERVAL)

#define LWIP_COMPAT_MUTEX_ALLOWED       (1)

// #define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

#define ETHARP_SUPPORT_STATIC_ENTRIES   1

#define LWIP_RIPPLE20                   1

/* Beken specific LWIP options */
#define BK_DHCP                         1

#define LWIP_TAG "lwip"
#define LWIP_LOGI(...) BK_LOGI(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGW(...) BK_LOGW(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGE(...) BK_LOGE(LWIP_TAG, ##__VA_ARGS__)
#define LWIP_LOGD(...) BK_LOGD(LWIP_TAG, ##__VA_ARGS__)
#if 0
#define LWIP_HOOK_FILENAME              "lwip_hooks.h"
#define LWIP_HOOK_IP4_ROUTE_SRC         ip4_route_src_hook
#endif
#define BK_IP4_ROUTE                    1
#define BK_DHCPS_DNS                    1

#if CONFIG_FREERTOS
#define LWIP_NETCONN_SEM_PER_THREAD 1
#define LWIP_NETCONN_FULLDUPLEX 1
#define BK_LWIP_DEBUG 1
#endif

#endif /* __LWIPOPTS_H__ */

