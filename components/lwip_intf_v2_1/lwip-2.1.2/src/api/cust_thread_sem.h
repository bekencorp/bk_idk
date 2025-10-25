#include "os/mem.h"
struct thread_sem
{
    void *taskhandle;
    sys_sem_t sem;
    struct thread_sem *next;
};

static struct thread_sem *thread_sem_list = NULL;
sys_mutex_t pt_mutex = NULL;

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#define PT_MUTEX_LOCK() do { \
                        if(!pt_mutex) rtos_init_recursive_mutex(&pt_mutex); \
                        if(pt_mutex)  rtos_lock_recursive_mutex(&pt_mutex); \
                      } while (0)

#define PT_MUTEX_UNLOCK() do { \
                          if(!pt_mutex) rtos_init_recursive_mutex(&pt_mutex); \
                          if(pt_mutex) rtos_unlock_recursive_mutex(&pt_mutex); \
                      } while (0)

extern void *sys_get_task_handle(void);

#ifndef CONFIG_CPU_INDEX
#define CONFIG_CPU_INDEX    0
#endif // CONFIG_CPU_INDEX
err_t LWIP_NETCONN_THREAD_SEM_ALLOC(void *thread)
{
    void *pxTCB = NULL;
    err_t err = ERR_OK;

    pxTCB = (NULL == thread) ? sys_get_task_handle() : thread ;
    if(NULL == pxTCB) {
        bk_printf("LWIP_NETCONN_THREAD_SEM_ALLOC:pxTXB is null, %d\n", CONFIG_CPU_INDEX);
        return ERR_VAL;
    }

    PT_MUTEX_LOCK();
    if(thread_sem_list == NULL) {
        thread_sem_list = (struct thread_sem *)os_malloc(sizeof(struct thread_sem));
        if(NULL == thread_sem_list) {
            bk_printf("LWIP_NETCONN_THREAD_SEM_ALLOC, malloc for thread_sem_list failed, %d\n", CONFIG_CPU_INDEX);
            PT_MUTEX_UNLOCK();
            return ERR_MEM;
        }
        memset((unsigned char *)thread_sem_list, 0x00, sizeof(struct thread_sem));
        thread_sem_list->taskhandle = pxTCB;
        thread_sem_list->next = NULL;
        err = sys_sem_new(&(thread_sem_list->sem), 0);
        if(err != ERR_OK) {
            os_free(thread_sem_list);
            thread_sem_list = NULL;
            bk_printf("LWIP_NETCONN_THREAD_SEM_ALLOC:sys_sem_new failed!!!!!!\n");
            PT_MUTEX_UNLOCK();
            return err;
        }
    }else {
        struct thread_sem *temp = thread_sem_list;
        while(1) {
            if(temp->next == NULL)
                break;

            temp = temp->next;
        }

        temp->next = (struct thread_sem *)os_malloc(sizeof(struct thread_sem));
        if(NULL == temp->next) {
            bk_printf("LWIP_NETCONN_THREAD_SEM_ALLOC, malloc for thread_sem is null, %d\n", CONFIG_CPU_INDEX);
            PT_MUTEX_UNLOCK();
            return ERR_MEM;
        }

        memset((unsigned char *)(temp->next) , 0x00, sizeof(struct thread_sem));
        temp->next->taskhandle = pxTCB;
        temp->next->next = NULL;

        err = sys_sem_new(&(temp->next->sem), 0);
        if(err != ERR_OK) {
            os_free(temp->next);
            temp->next = NULL;
            bk_printf("LWIP_NETCONN_THREAD_SEM_ALLOC:sys_sem_new failed!!!!!!\n");
            PT_MUTEX_UNLOCK();
            return err;
        }
    }
    PT_MUTEX_UNLOCK();

    return err;
}

err_t LWIP_NETCONN_THREAD_SEM_FREE(void *thread)
{
    void *pxTCB = NULL;
    err_t err = ERR_OK;

    pxTCB = (NULL == thread) ? sys_get_task_handle() : thread;
    if(NULL == pxTCB) {
        bk_printf("LWIP_NETCONN_THREAD_SEM_FREE:pxTCB is null\n");
        return ERR_VAL;
    }

    PT_MUTEX_LOCK();
    if(thread_sem_list != NULL) {
        struct thread_sem *temp = thread_sem_list;
        struct thread_sem *bkup = NULL;
        while(1) {
            if(temp->taskhandle == pxTCB) {
                if(NULL == bkup) {
                    thread_sem_list = temp->next;
                }else {
                    bkup->next = temp->next;
                }
                sys_sem_free(&(temp->sem));
                sys_sem_set_invalid(&(temp->sem));
                temp->taskhandle = NULL;
                temp->next = NULL;
                os_free(temp);
                break;
            }

            if(temp->next == NULL) {
                bk_printf("LWIP_NETCONN_THREAD_SEM_FREE:not find thread sem\n");
                break;
            }
            bkup = temp;
            temp = temp->next;
        }
    }

    PT_MUTEX_UNLOCK();
    return err;
}

sys_sem_t* LWIP_NETCONN_THREAD_SEM_GET(void)
{
    void *pxTCB = sys_get_task_handle();
    if(NULL == pxTCB) {
        bk_printf("LWIP_NETCONN_THREAD_SEM_GET:pxTCB is null\n");
        return NULL;
    }
    PT_MUTEX_LOCK();
    if(thread_sem_list == NULL) {
        bk_printf("LWIP_NETCONN_THREAD_SEM_GET:thread_sem_list is null, %d\n", CONFIG_CPU_INDEX);
        PT_MUTEX_UNLOCK();
        return NULL;
    }else {
        struct thread_sem *temp = thread_sem_list;
        while(1) {
            if(temp->taskhandle == pxTCB) {
              PT_MUTEX_UNLOCK();
              return &temp->sem;
            }

            if(temp->next == NULL) {
              PT_MUTEX_UNLOCK();
              return NULL;
            }


            temp = temp->next;
        }
    }
    PT_MUTEX_UNLOCK();
}

err_t
netconn_thread_init(void *ptask)
{
   return LWIP_NETCONN_THREAD_SEM_ALLOC(ptask);
}

err_t
netconn_thread_cleanup(void *ptask)
{
    return LWIP_NETCONN_THREAD_SEM_FREE(ptask);
}

