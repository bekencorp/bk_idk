#ifndef _MQTT_CLIENT_COM_PORT_H__
#define _MQTT_CLIENT_COM_PORT_H__






extern unsigned int mqtt_get_time(void);



extern void* mqtt_senssion_lock_create(void);
extern int mqtt_senssion_lock(void* lock,unsigned int timeout);
extern int mqtt_senssion_unlock(void * lock);
extern void* mqtt_senssion_lock_destroy(void* lock);

extern void mqtt_core_handler_thread_init(void);

#endif
