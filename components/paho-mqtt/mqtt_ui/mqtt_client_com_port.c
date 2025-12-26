#include "mqtt_client_com_port.h"
#include "mqtt_client_core.h"


unsigned int mqtt_get_time(void)
{
	return rtos_get_time();
}

void* mqtt_senssion_lock_create(void)
{
	beken_semaphore_t semaphore = NULL;
	
	rtos_init_semaphore( &semaphore, 1 );
	if( semaphore != NULL )
	{
		rtos_set_semaphore( &semaphore );
	}
	return semaphore;
}

int mqtt_senssion_lock(void* lock,unsigned int timeout)
{
	beken_semaphore_t semaphore = lock;
	
	if( semaphore != NULL )
	{
		if(rtos_get_semaphore( &semaphore, timeout ) == kNoErr)
		{
			return MQTT_OK;
		}
	}
	return MQTT_ERR;
}

int mqtt_senssion_unlock(void * lock)
{
	beken_semaphore_t semaphore = lock;
	
	if(semaphore != NULL)
	{
		rtos_set_semaphore(&semaphore);
		return MQTT_OK;
	}
	
	return MQTT_ERR;
}

void* mqtt_senssion_lock_destroy(void* lock)
{
	beken_semaphore_t semaphore = lock;
	
	if(semaphore != NULL)
	{
		rtos_deinit_semaphore( &semaphore );
		semaphore = NULL;
	}
	
	return semaphore;
}

void mqtt_core_handler_thd(void *arg)
{
	bk_printf("%d:%s\r\n",__LINE__,__FUNCTION__);
	
	while(1)
	{
		mqtt_core_handler();
		rtos_delay_milliseconds( 2 );
	}
	
	rtos_delete_thread( NULL );
}

void mqtt_core_handler_thread_init(void)
{
	rtos_create_thread(NULL,
	                 BEKEN_DEFAULT_WORKER_PRIORITY,
	                 "mqtt-core",
	                 (beken_thread_function_t)mqtt_core_handler_thd,
	                 6 * 1024,
			         NULL);
}
// eof

