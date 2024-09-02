#include <string.h>
#include <stdlib.h>

#include <common/bk_kernel_err.h>
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "timers.h"
#include <os/os.h>
#include <common/bk_generic.h>
#include "bk_fake_clock.h"
#include <os/mem.h>
#include "bk_uart.h"
#include "bk_arch.h"
#include <components/system.h>
#include <driver/gpio.h>
#include "rtos_impl.h"

/******************************************************
 *                    Constants
 ******************************************************/
#ifndef TIMER_THREAD_STACK_SIZE
#define TIMER_THREAD_STACK_SIZE      1024 + 4*1024
#endif
#define TIMER_QUEUE_LENGTH           5

#define RTOS_ASSERT_TASK_CONTEXT() do {                    \
	BK_ASSERT(0 == platform_is_in_interrupt_context());    \
} while(0)

#define RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER() do {    \
	if(s_is_started_scheduler){                       \
		BK_ASSERT(0 == platform_local_irq_disabled());\
	}                                                 \
} while(0)

#define RTOS_ASSERT_INT_ENABLED() do {    \
	BK_ASSERT(0 == platform_local_irq_disabled());    \
} while(0)

#define _xTaskCreate( pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask ) xTaskCreate( pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask )
#define _xTaskCreateAffinitySet( pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, uxCoreAffinityMask, pxCreatedTask ) xTaskCreateAffinitySet( pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, uxCoreAffinityMask, pxCreatedTask )
#define _xTimerCreate( pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID, pxCallbackFunction ) xTimerCreate( pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID, pxCallbackFunction )

BaseType_t xTaskCreateInPsram( TaskFunction_t pxTaskCode,
                        const char * const pcName, /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
                        const configSTACK_DEPTH_TYPE usStackDepth,
                        void * const pvParameters,
                        UBaseType_t uxPriority,
                        TaskHandle_t * const pxCreatedTask );

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef tmrTIMER_CALLBACK native_timer_handler_t;
typedef pdTASK_CODE       native_thread_t;

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct
{
    event_handler_t function;
    void*           arg;
} timer_queue_message_t;

typedef struct
{
    event_handler_t function;
    void*           arg;
} beken_event_message_t;


/******************************************************
 *               Function Declarations
 ******************************************************/
extern void rtos_stack_overflow(char *taskname);
extern uint32_t platform_is_in_interrupt_context( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/
static beken_time_t beken_time_offset = 0;
uint32_t  max_syscall_int_prio;
uint32_t rtos_max_priorities = RTOS_HIGHEST_PRIORITY - RTOS_LOWEST_PRIORITY + 1;
static bool s_is_started_scheduler = false;

/******************************************************
 *               Function Definitions
 ******************************************************/
bk_err_t rtos_create_thread_with_affinity( beken_thread_t *thread, uint32_t affinity, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    /* Limit priority to default lib priority */
    if ( priority > RTOS_HIGHEST_PRIORITY )
    {
        priority = RTOS_HIGHEST_PRIORITY;
    }

    if( pdPASS == _xTaskCreateAffinitySet( (native_thread_t)function, name,
            (unsigned short) (stack_size/sizeof( portSTACK_TYPE )),
            (void *)arg, BK_PRIORITY_TO_NATIVE_PRIORITY(priority),
            affinity, //tskNO_AFFINITY,
            (TaskHandle_t *)thread ) )
    {
        return kNoErr;
    }
    else
    {
        return kGeneralErr;
    }
}

// create task and set affinity to cpu0
bk_err_t rtos_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    return rtos_create_thread_with_affinity(thread, 1 << 0, priority, name, function, stack_size, arg);
}

bk_err_t rtos_smp_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    return rtos_create_thread_with_affinity(thread, tskNO_AFFINITY, priority, name, function, stack_size, arg);
}

bk_err_t rtos_core0_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    return rtos_create_thread_with_affinity(thread, 1 << 0, priority, name, function, stack_size, arg);
}

#if (CONFIG_CPU_CNT > 1)
bk_err_t rtos_core1_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    return rtos_create_thread_with_affinity(thread, 1 << 1, priority, name, function, stack_size, arg);
}
#endif

#if (CONFIG_CPU_CNT > 2)
bk_err_t rtos_core2_create_thread( beken_thread_t* thread, uint8_t priority, const char* name,
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    return rtos_create_thread_with_affinity(thread, 1 << 2, priority, name, function, stack_size, arg);
}
#endif

bk_err_t rtos_create_psram_thread( beken_thread_t* thread, uint8_t priority, const char* name, 
                        beken_thread_function_t function, uint32_t stack_size, beken_thread_arg_t arg )
{
    os_printf("Task Create In Psram is not supported.\r\n.");
    return kGeneralErr;
}

bk_err_t rtos_delete_thread( beken_thread_t* thread )
{
	extern void pthread_internal_local_storage_destructor_callback(TaskHandle_t handle);
    if ( thread == NULL )
    {
		pthread_internal_local_storage_destructor_callback(NULL);
        vTaskDelete( NULL );
    }
    else if ( xTaskIsTaskFinished( *thread ) != pdTRUE )
    {
		pthread_internal_local_storage_destructor_callback(*thread);
        vTaskDelete( *thread );
    }

    return kNoErr;
}

bk_err_t rtos_thread_join(beken_thread_t* thread)
{
    while ( xTaskIsTaskFinished( (xTaskHandle)*thread ) != pdTRUE )
    {
        rtos_delay_milliseconds(10);
    }

    return kNoErr;
}


bool rtos_is_current_thread( beken_thread_t* thread )
{
    if ( xTaskGetCurrentTaskHandle() == *thread )
    {
        return true;
    }
    else
    {
        return false;
    }
}

beken_thread_t* rtos_get_current_thread( void )
{
    return (beken_thread_t *)xTaskGetCurrentTaskHandle();
}

bk_err_t rtos_thread_set_priority(beken_thread_t *thread, int priority)
{
    if (thread)
        vTaskPrioritySet(*thread, BK_PRIORITY_TO_NATIVE_PRIORITY(priority));
    return kNoErr;
}

bk_err_t rtos_check_stack( void )
{
    //  TODO: Add stack checking here.
    return kNoErr;
}

bk_err_t rtos_thread_force_awake( beken_thread_t* thread )
{
    xTaskAbortDelay(*thread);

    return kNoErr;
}

void rtos_thread_sleep(uint32_t seconds)
{
	RTOS_ASSERT_INT_ENABLED();
	
    rtos_delay_milliseconds(seconds * 1000);
}

bk_err_t beken_time_get_time(beken_time_t* time_ptr)
{
    *time_ptr = (beken_time_t) ( xTaskGetTickCount() * bk_get_ms_per_tick() ) + beken_time_offset;
    return kNoErr;
}

bk_err_t beken_time_set_time(beken_time_t* time_ptr)
{
    beken_time_offset = *time_ptr - (beken_time_t) ( xTaskGetTickCount() * bk_get_ms_per_tick() );
    return kNoErr;
}

bk_err_t rtos_init_semaphore( beken_semaphore_t* semaphore, int maxCount )
{
    *semaphore = xSemaphoreCreateCounting( (unsigned portBASE_TYPE) maxCount, (unsigned portBASE_TYPE) 0 );

    return ( *semaphore != NULL ) ? kNoErr : kGeneralErr;
}

bk_err_t rtos_init_semaphore_ex( beken_semaphore_t* semaphore, int maxCount, int init_count)
{
    *semaphore = xSemaphoreCreateCounting( (unsigned portBASE_TYPE) maxCount, (unsigned portBASE_TYPE) init_count );

    return ( *semaphore != NULL ) ? kNoErr : kGeneralErr;
}


bk_err_t rtos_get_semaphore( beken_semaphore_t* semaphore, uint32_t timeout_ms )
{
    uint32_t time;

    if(timeout_ms == BEKEN_WAIT_FOREVER)
        time = portMAX_DELAY;
    else
        time = timeout_ms / bk_get_ms_per_tick();

    if(time != 0)
    {
        RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();
        RTOS_ASSERT_TASK_CONTEXT();
    }
        
    if ( pdTRUE == xSemaphoreTake( *semaphore, (portTickType) (time) ) )
    {
        return kNoErr;
    }
    else
    {
        return kTimeoutErr;
    }
}
int rtos_get_semaphore_count( beken_semaphore_t* semaphore )
{
    return uxSemaphoreGetCount( *semaphore );
}

int rtos_set_semaphore( beken_semaphore_t* semaphore )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS )
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xSemaphoreGiveFromISR( *semaphore, &xHigherPriorityTaskWoken );

        /* If xSemaphoreGiveFromISR() unblocked a task, and the unblocked task has
         * a higher priority than the currently executing task, then
         * xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
         * return directly to the higher priority unblocked task.
         */
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
        result = xSemaphoreGive( *semaphore );
    }

    return ( result == pdPASS )? kNoErr : kGeneralErr;
}

bk_err_t rtos_deinit_semaphore( beken_semaphore_t* semaphore )
{
    if (semaphore != NULL)
    {
        vQueueDelete( *semaphore );
        *semaphore = NULL;
    }
    return kNoErr;
}

static volatile spinlock_t rtos_spin_lock = SPIN_LOCK_INIT;
uint32_t rtos_enter_critical( void )
{
	uint32_t flags = rtos_disable_int();
	spin_lock(&rtos_spin_lock);
	return flags;
}

void rtos_exit_critical( uint32_t state )
{
	spin_unlock(&rtos_spin_lock);
	rtos_enable_int(state);
}

// disable current task be preempt
void rtos_disable_preempt()
{
#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
    vTaskPreemptionDisable( NULL );
#endif
}

// enable current task be preempt
void rtos_enable_preempt()
{
#if ( configUSE_TASK_PREEMPTION_DISABLE == 1 )
    vTaskPreemptionEnable( NULL );
#endif
}

bk_err_t rtos_init_mutex( beken_mutex_t* mutex )
{
    /* Mutex uses priority inheritance */
    *mutex = xSemaphoreCreateMutex();
    if ( *mutex == NULL )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_trylock_mutex(beken_mutex_t *mutex)
{
    if ( xSemaphoreTake( *mutex, 0 ) != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_lock_mutex( beken_mutex_t* mutex )
{
	RTOS_ASSERT_TASK_CONTEXT();
	RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();

    if ( xSemaphoreTake( *mutex, BEKEN_WAIT_FOREVER ) != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_lock_mutex_timeout( beken_mutex_t* mutex, uint32_t timeout_ms)
{
	RTOS_ASSERT_TASK_CONTEXT();
	RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();

    if ( xSemaphoreTake( *mutex, timeout_ms) != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_unlock_mutex( beken_mutex_t* mutex )
{
    if ( xSemaphoreGive( *mutex ) != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_deinit_mutex( beken_mutex_t* mutex )
{
    vSemaphoreDelete( *mutex );
    *mutex = NULL;
    return kNoErr;
}

#if CONFIG_FREERTOS_USE_RECURSIVE_MUTEXES

bk_err_t rtos_init_recursive_mutex( beken_mutex_t* mutex )
{
    *mutex = xSemaphoreCreateRecursiveMutex();
    if ( *mutex == NULL )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_lock_recursive_mutex( beken_mutex_t* mutex )
{
	RTOS_ASSERT_TASK_CONTEXT();
	RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();

    if ( xSemaphoreTakeRecursive( *mutex, BEKEN_WAIT_FOREVER ) != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_unlock_recursive_mutex( beken_mutex_t* mutex )
{
    if ( xSemaphoreGiveRecursive(*mutex ) != pdPASS)
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_deinit_recursive_mutex( beken_mutex_t* mutex )
{
    return rtos_deinit_mutex(mutex);
}

#endif

bk_err_t rtos_init_queue( beken_queue_t* queue, const char* name, uint32_t message_size, uint32_t number_of_messages )
{
    __maybe_unused_var(name);

    if ( ( *queue = xQueueCreate( number_of_messages, message_size ) ) == NULL )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_push_to_queue( beken_queue_t* queue, void* message, uint32_t timeout_ms )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS )
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xQueueSendToBackFromISR( *queue, message, &xHigherPriorityTaskWoken );

        /* If xQueueSendToBackFromISR() unblocked a task, and the unblocked task has
         * a higher priority than the currently executing task, then
         * xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
         * return directly to the higher priority unblocked task.
         */
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
        uint32_t time;

        if(timeout_ms == BEKEN_WAIT_FOREVER)
            time = portMAX_DELAY;
        else
            time = timeout_ms / bk_get_ms_per_tick();

		if(time != 0)
			RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();
        
        result = xQueueSendToBack( *queue, message, (portTickType) ( time ) );
    }

    return ( result == pdPASS )? kNoErr : kGeneralErr;
}


bk_err_t rtos_push_to_queue_front( beken_queue_t* queue, void* message, uint32_t timeout_ms )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() != 0 )
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xQueueSendToFrontFromISR( *queue, message, &xHigherPriorityTaskWoken );

        /* If xQueueSendToBackFromISR() unblocked a task, and the unblocked task has
         * a higher priority than the currently executing task, then
         * xHigherPriorityTaskWoken will have been set to pdTRUE and this ISR should
         * return directly to the higher priority unblocked task.
         */
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
        uint32_t time;

        if(timeout_ms == BEKEN_WAIT_FOREVER)
            time = portMAX_DELAY;
        else
            time = timeout_ms / bk_get_ms_per_tick();

		if(time != 0)
			RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();
        
        result = xQueueSendToFront( *queue, message, (portTickType) ( time ) );
    }

    return ( result == pdPASS )? kNoErr : kGeneralErr;
}


bk_err_t rtos_pop_from_queue( beken_queue_t* queue, void* message, uint32_t timeout_ms )
{
    uint32_t time;

    if(timeout_ms == BEKEN_WAIT_FOREVER)
        time = portMAX_DELAY;
	else
        time = timeout_ms / bk_get_ms_per_tick();

	if(time != 0)
		RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();

    if ( xQueueReceive( *queue, message, ( time ) ) != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}


bk_err_t rtos_deinit_queue( beken_queue_t* queue )
{
    vQueueDelete( *queue );
    *queue = NULL;
    return kNoErr;
}

bool rtos_is_queue_empty( beken_queue_t* queue )
{
    signed portBASE_TYPE result;

#ifdef CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED
    if (platform_is_in_interrupt_context() == RTOS_SUCCESS)
    {
    //    uint32_t flags = taskENTER_CRITICAL_FROM_ISR();
        result = xQueueIsQueueEmptyFromISR( *queue );
    //    taskEXIT_CRITICAL_FROM_ISR(flags);
    }
    else
    {
    //    taskENTER_CRITICAL();
        result = xQueueIsQueueEmptyFromISR(*queue);
    //    taskEXIT_CRITICAL();
    }
#else
    result = xQueueIsQueueEmptyFromISR( *queue );
#endif

    return ( result != 0 ) ? true : false;
}

bool rtos_is_queue_full( beken_queue_t* queue )
{
    signed portBASE_TYPE result;

#ifdef CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED
    if (platform_is_in_interrupt_context() == RTOS_SUCCESS)
    {
    //    uint32_t flags = taskENTER_CRITICAL_FROM_ISR();
        result = xQueueIsQueueFullFromISR( *queue );
    //    taskEXIT_CRITICAL_FROM_ISR(flags);
    }
    else
    {
    //    taskENTER_CRITICAL();
        result = xQueueIsQueueFullFromISR(*queue);
    //    taskEXIT_CRITICAL();
    }
#else
    result = xQueueIsQueueFullFromISR( *queue );
#endif

    return ( result != 0 ) ? true : false;
}

bool rtos_reset_queue( beken_queue_t* queue )
{
    return true;
}

static void timer_callback2( xTimerHandle handle )
{
    beken2_timer_t *timer = (beken2_timer_t*) pvTimerGetTimerID( handle );

    if(BEKEN_MAGIC_WORD != timer->beken_magic)
    {
        return;
    }
    if ( timer->function )
    {
        timer->function( timer->left_arg, timer->right_arg );
    }
}

static void timer_callback1( xTimerHandle handle )
{
    beken_timer_t *timer = (beken_timer_t*) pvTimerGetTimerID( handle );

    if ( timer->function )
    {
        timer->function( timer->arg);
    }
}

bk_err_t rtos_start_oneshot_timer( beken2_timer_t* timer )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerStartFromISR(timer->handle, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
        result = xTimerStart( timer->handle, BEKEN_WAIT_FOREVER );
    }

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_deinit_oneshot_timer( beken2_timer_t* timer )
{
    bk_err_t ret = kNoErr;
    GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

    if (!rtos_is_oneshot_timer_init(timer)) {
        //need not deinit
		GLOBAL_INT_RESTORE();
        return kNoErr;
    }

	void * handle = timer->handle;
	timer->handle = 0;
	
	GLOBAL_INT_RESTORE();

    if ( xTimerDelete(handle, BEKEN_WAIT_FOREVER ) != pdPASS )
    {
		timer->handle = handle;
        ret = kGeneralErr;
    }
	else
	{
		timer->handle = 0;
		timer->function = 0;
		timer->left_arg = 0;
		timer->right_arg = 0;
		timer->beken_magic = 0;
	}

    return ret;
}

bk_err_t rtos_stop_oneshot_timer( beken2_timer_t* timer )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerStopFromISR(timer->handle, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else
        result = xTimerStop( timer->handle, BEKEN_WAIT_FOREVER );

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}


bool rtos_is_oneshot_timer_init( beken2_timer_t* timer )
{
    return timer->handle ? true : false;
}

bool rtos_is_oneshot_timer_running( beken2_timer_t* timer )
{
    return ( xTimerIsTimerActive( timer->handle ) != 0 ) ? true : false;
}

bk_err_t rtos_oneshot_reload_timer( beken2_timer_t* timer )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS )
    {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerResetFromISR(timer->handle, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    }
    else
        result = xTimerReset( timer->handle, BEKEN_WAIT_FOREVER );

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_oneshot_reload_timer_ex(beken2_timer_t *timer,
                                        uint32_t time_ms,
                                        timer_2handler_t function,
                                        void *larg,
                                        void *rarg)
{
    bk_err_t ret;

    if (rtos_is_oneshot_timer_running(timer)) {
        ret = rtos_stop_oneshot_timer(timer);
        if (ret != kNoErr) {
            return ret;
        }
    }

    ret = rtos_change_period((beken_timer_t*)timer, time_ms);
    if (ret != kNoErr) {
        return ret;
    }

    timer->function = function;
    timer->left_arg = larg;
    timer->right_arg = rarg;
    ret = rtos_oneshot_reload_timer(timer);
    return ret;
}

bk_err_t rtos_init_oneshot_timer( beken2_timer_t *timer,
                                    uint32_t time_ms,
                                    timer_2handler_t function,
                                    void* larg,
                                    void* rarg )
{
    bk_err_t ret = kNoErr;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    timer->function = function;
    timer->left_arg = larg;
    timer->right_arg = rarg;
    timer->beken_magic = BEKEN_MAGIC_WORD;
    timer->handle = _xTimerCreate("",
                                (portTickType)( time_ms / bk_get_ms_per_tick() ),
                                pdFALSE,
                                timer,
                                (native_timer_handler_t) timer_callback2 );
    if ( timer->handle == NULL )
    {
        ret = kGeneralErr;
    }

    GLOBAL_INT_RESTORE();

    return ret;
}

bk_err_t rtos_init_timer( beken_timer_t *timer,
                                    uint32_t time_ms,
                                    timer_handler_t function,
                                    void* arg )
{
    bk_err_t ret = kNoErr;

    GLOBAL_INT_DECLARATION();

    GLOBAL_INT_DISABLE();
    timer->function = function;
    timer->arg      = arg;

    timer->handle = _xTimerCreate("",
                                (portTickType)( time_ms / bk_get_ms_per_tick() ),
                                pdTRUE,
                                timer,
                                (native_timer_handler_t) timer_callback1 );
    if ( timer->handle == NULL )
    {
        ret = kGeneralErr;
    }

    GLOBAL_INT_RESTORE();

    return ret;;
}

bk_err_t rtos_start_timer( beken_timer_t* timer )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerStartFromISR(timer->handle, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else
        result = xTimerStart( timer->handle, BEKEN_WAIT_FOREVER );

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_stop_timer( beken_timer_t* timer )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerStopFromISR(timer->handle, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else
        result = xTimerStop( timer->handle, BEKEN_WAIT_FOREVER );

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_reload_timer( beken_timer_t* timer )
{
    signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerResetFromISR(timer->handle, &xHigherPriorityTaskWoken );
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else
        result = xTimerReset( timer->handle, BEKEN_WAIT_FOREVER );

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;
}

bk_err_t rtos_change_period( beken_timer_t* timer, uint32_t time_ms)
{
   signed portBASE_TYPE result;

    if ( platform_is_in_interrupt_context() == RTOS_SUCCESS ) {
        signed portBASE_TYPE xHigherPriorityTaskWoken = 0;
        result = xTimerChangePeriodFromISR(timer->handle,
                                     (portTickType)( time_ms / bk_get_ms_per_tick() ),
                                     &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
    } else
        result = xTimerChangePeriod( timer->handle,
                                     (portTickType)( time_ms / bk_get_ms_per_tick() ),
                                     0 );

    if ( result != pdPASS )
    {
        return kGeneralErr;
    }

    return kNoErr;

}

bk_err_t rtos_deinit_timer( beken_timer_t* timer )
{
	bk_err_t ret = kNoErr;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	void * handle = timer->handle;
	timer->handle = 0;
	GLOBAL_INT_RESTORE();

	if(handle == 0)
	{
		return kNoErr;
	}

    if ( xTimerDelete( handle, BEKEN_WAIT_FOREVER ) != pdPASS )
    {
		timer->handle = handle;
        ret = kGeneralErr;
    }
	else
	{
		timer->handle = 0;
	}

    return ret;
}

uint32_t rtos_get_timer_expiry_time( beken_timer_t* timer )
{
    return xTimerGetExpiryTime( timer->handle);
}

bool rtos_is_timer_init( beken_timer_t* timer )
{
    return timer->handle ? true : false;
}

bool rtos_is_timer_running( beken_timer_t* timer )
{
    return ( xTimerIsTimerActive( timer->handle ) != 0 ) ? true : false;
}

bk_err_t rtos_init_event_flags( beken_event_flags_t* event_flags )
{
    __maybe_unused_var( event_flags );

    return kUnsupportedErr;
}

bk_err_t rtos_wait_for_event_flags( beken_event_flags_t* event_flags, uint32_t flags_to_wait_for, uint32_t* flags_set, beken_bool_t clear_set_flags, beken_event_flags_wait_option_t wait_option, uint32_t timeout_ms )
{
    __maybe_unused_var( event_flags );
    __maybe_unused_var( flags_to_wait_for );
    __maybe_unused_var( flags_set );
    __maybe_unused_var( clear_set_flags );
    __maybe_unused_var( wait_option );
    __maybe_unused_var( timeout_ms );

    return kUnsupportedErr;
}

bk_err_t rtos_set_event_flags( beken_event_flags_t* event_flags, uint32_t flags_to_set )
{
    __maybe_unused_var( event_flags );
    __maybe_unused_var( flags_to_set );

    return kUnsupportedErr;
}

bk_err_t rtos_deinit_event_flags( beken_event_flags_t* event_flags )
{
    __maybe_unused_var( event_flags );

    return kUnsupportedErr;
}

void rtos_suspend_thread(beken_thread_t* thread)
{
    if (thread == NULL)
    {
		RTOS_ASSERT_INT_ENABLED_WITH_SCHEDULER();
		
        vTaskSuspend(NULL);
    }
    else
        vTaskSuspend(*thread);
}

void rtos_resume_thread(beken_thread_t* thread)
{
    if (thread == NULL)
        vTaskResume(NULL);
    else
        vTaskResume(*thread);
}

uint32_t beken_ms_per_tick(void)
{
    return bk_get_ms_per_tick();
}

/**
 * Gets time in milliseconds since RTOS start
 *
 * @Note: since this is only 32 bits, it will roll over every 49 days, 17 hours.
 *
 * @returns Time in milliseconds since RTOS started.
 */
beken_time_t rtos_get_time( void )
{
    return (beken_time_t) ( xTaskGetTickCount() * bk_get_ms_per_tick() );
}

/**
 * Delay for a number of milliseconds
 *
 * Processing of this function depends on the minimum sleep
 * time resolution of the RTOS.
 * The current thread sleeps for the longest period possible which
 * is less than the delay required, then makes up the difference
 * with a tight loop
 *
 * @return bk_err_t : kNoErr if delay was successful
 *
 */
bk_err_t rtos_delay_milliseconds( uint32_t num_ms )
{
    uint32_t ticks;

    ticks = num_ms / bk_get_ms_per_tick();
	if (ticks == 0)
        ticks = 1;

	RTOS_ASSERT_INT_ENABLED();

    vTaskDelay( (portTickType) ticks );

    return kNoErr;
}

/*-----------------------------------------------------------*/
#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    __maybe_unused_var( pxTask );
    __maybe_unused_var( pcTaskName );

    rtos_stack_overflow((char*)pcTaskName);
}
#endif

/*-----------------------------------------------------------*/
void vApplicationMallocFailedHook( void )
{
}

/*-----------------------------------------------------------*/
void *beken_malloc( size_t xWantedSize )
{
    return (void *)os_malloc(xWantedSize);
}

/*-----------------------------------------------------------*/
void beken_free( void *pv )
{
    os_free(pv);
}

/*-----------------------------------------------------------*/
void *beken_realloc( void *pv, size_t xWantedSize )
{
    return os_realloc(pv, xWantedSize);
}

void rtos_start_scheduler(void)
{
    s_is_started_scheduler = true;
    vTaskStartScheduler();
}

bool rtos_is_scheduler_started(void)
{
    return s_is_started_scheduler;
}

char* rtos_get_name(void)
{
#define RTOS_NAME                     "FreeRTOS"
    return RTOS_NAME;
}

char* rtos_get_version(void)
{
#define RTOS_VERSION                  "v10.4"
    return RTOS_VERSION;
}

size_t rtos_get_total_heap_size(void)
{
    return prvHeapGetTotalSize();
}

size_t rtos_get_free_heap_size(void)
{
    return xPortGetFreeHeapSize();
}

size_t rtos_get_minimum_free_heap_size(void)
{
    return xPortGetMinimumEverFreeHeapSize();
}

size_t rtos_get_psram_total_heap_size(void)
{
    return xPortGetPsramTotalHeapSize();
}

size_t rtos_get_psram_free_heap_size(void)
{
    return xPortGetPsramFreeHeapSize();
}

size_t rtos_get_psram_minimum_free_heap_size(void)
{
    return xPortGetPsramMinimumFreeHeapSize();
}

uint32_t rtos_disable_int(void)
{
    return port_disable_interrupts_flag();
}

void rtos_enable_int(uint32_t int_level)
{
    port_enable_interrupts_flag(int_level);
}

uint32_t rtos_before_sleep(void)
{
    return rtos_disable_int();
}

void rtos_after_sleep(uint32_t int_level)
{
    rtos_enable_int(int_level);
}

void rtos_stop_int(void)
{
    portDISABLE_INTERRUPTS();
}

void rtos_start_int(void)
{
    portENABLE_INTERRUPTS();
}

bool rtos_is_in_interrupt_context(void)
{
    return platform_is_in_interrupt_context();
}

void rtos_wait_for_interrupt(void)
{
#if (!CONFIG_ARCH_RISCV)
    arch_wait_for_interrupt();
#endif
}

#if (configSUPPORT_STATIC_ALLOCATION == 1)

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
#if configNUM_CORES < 2
    static __attribute__((section(".dtcm_sec_data "))) StaticTask_t xIdleTaskTCB;
    static __attribute__((section(".dtcm_sec_data "))) StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];
#else
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];
#endif
    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/
#endif // (configSUPPORT_STATIC_ALLOCATION == 1)

#if (configSUPPORT_STATIC_ALLOCATION == 1) && (configUSE_TIMERS == 1)

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
#if configNUM_CORES < 2
    static __attribute__((section(".dtcm_sec_data "))) StaticTask_t xTimerTaskTCB;
    static __attribute__((section(".dtcm_sec_data "))) StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];
#else
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];
#endif
    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

#endif // (configSUPPORT_STATIC_ALLOCATION == 1) && (configUSE_TIMERS == 1)

