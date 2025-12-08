/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include "soc/soc.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "portable.h"
#include "lock.h"

#define IRAM_ATTR __IRAM_SEC

/* Notes on our newlib lock implementation:
 *
 * - Use FreeRTOS mutex semaphores as locks.
 * - lock_t is int, but we store an SemaphoreHandle_t there.
 * - Locks are no-ops until the FreeRTOS scheduler is running.
 * - Due to this, locks need to be lazily initialised the first time
 *   they are acquired. Initialisation/deinitialisation of locks is
 *   protected by lock_init_spinlock.
 * - Race conditions around lazy initialisation (via lock_acquire) are
 *   protected against.
 * - Anyone calling lock_close is reponsible for ensuring noone else
 *   is holding the lock at this time.
 * - Race conditions between lock_close & lock_init (for the same lock)
 *   are the responsibility of the caller.
	static portMUX_TYPE lock_init_spinlock = portMUX_INITIALIZER_UNLOCKED;
 */

bool xPortCanYield(void)
{
	return (!xPortIsInsideInterrupt());
}

/* Initialize the given lock by allocating a new mutex semaphore
   as the _lock_t value.

   Called by _lock_init*, also called by _lock_acquire* to lazily initialize locks that might have
   been initialised (to zero only) before the RTOS scheduler started.
*/
static void IRAM_ATTR lock_init_generic(_lock_t *lock, uint8_t mutex_type)
{
    portENTER_CRITICAL();
    if (*lock) {
         /* Lock already initialised (either we didn't check earlier,
          or it got initialised while we were waiting for the
          spinlock.) */
    }
    else
    {
        /* Create a new semaphore

           this is a bit of an API violation, as we're calling the
           private function xQueueCreateMutex(x) directly instead of
           the xSemaphoreCreateMutex / xSemaphoreCreateRecursiveMutex
           wrapper functions...

           The better alternative would be to pass pointers to one of
           the two xSemaphoreCreate___Mutex functions, but as FreeRTOS
           implements these as macros instead of inline functions
           (*party like it's 1998!*) it's not possible to do this
           without writing wrappers. Doing it this way seems much less
           spaghetti-like.
        */
        SemaphoreHandle_t new_sem = xQueueCreateMutex(mutex_type);
        if (!new_sem) {
            abort(); /* No more semaphores available or OOM */
        }
        *lock = (_lock_t)new_sem;
    }
    portEXIT_CRITICAL();
}

void IRAM_ATTR _lock_init(_lock_t *lock)
{
    *lock = 0; // In case lock's memory is uninitialized
    lock_init_generic(lock, queueQUEUE_TYPE_MUTEX);
}

void IRAM_ATTR _lock_init_recursive(_lock_t *lock)
{
    *lock = 0; // In case lock's memory is uninitialized
    lock_init_generic(lock, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

/* Free the mutex semaphore pointed to by *lock, and zero it out.

   Note that FreeRTOS doesn't account for deleting mutexes while they
   are held, and neither do we... so take care not to delete newlib
   locks while they may be held by other tasks!

   Also, deleting a lock in this way will cause it to be lazily
   re-initialised if it is used again. Caller has to avoid doing
   this!
*/
void IRAM_ATTR _lock_close(_lock_t *lock)
{
    portENTER_CRITICAL();
    if (*lock) {
        SemaphoreHandle_t h = (SemaphoreHandle_t)(*lock);
#if (INCLUDE_xSemaphoreGetMutexHolder == 1)
		/* mutex should not be held */
        if(xSemaphoreGetMutexHolder(h) != NULL){
			while(110);
		}
#endif
        vSemaphoreDelete(h);
        *lock = 0;
    }
    portEXIT_CRITICAL();
}

/* Acquire the mutex semaphore for lock. wait up to delay ticks.
   mutex_type is queueQUEUE_TYPE_RECURSIVE_MUTEX or queueQUEUE_TYPE_MUTEX
*/
static int IRAM_ATTR lock_acquire_generic(_lock_t *lock, uint32_t delay, uint8_t mutex_type)
{
    SemaphoreHandle_t h = (SemaphoreHandle_t)(*lock);
    if (!h) {
        if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
            return 0; /* locking is a no-op before scheduler is up, so this "succeeds" */
        }
        /* lazy initialise lock - might have had a static initializer (that we don't use) */
        lock_init_generic(lock, mutex_type);
        h = (SemaphoreHandle_t)(*lock);
        if(h == NULL){
			while(110);
		}
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return 0; /* locking is a no-op before scheduler is up, so this "succeeds" */
    }
    BaseType_t success = pdFAIL;
    if (!xPortCanYield()) {
        /* In ISR Context */
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
            abort(); /* recursive mutexes make no sense in ISR context */
        }
        BaseType_t higher_task_woken = false;
        success = xSemaphoreTakeFromISR(h, &higher_task_woken);
        if (!success && delay > 0) {
            abort(); /* Tried to block on mutex from ISR, couldn't... rewrite your program to avoid libc interactions in ISRs! */
        }
        if (higher_task_woken) {
            portYIELD_FROM_ISR(higher_task_woken);
        }
    }
    else {
        /* In task context */
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
#if ( configUSE_RECURSIVE_MUTEXES == 1 )
            success = xSemaphoreTakeRecursive(h, delay);
#endif
        } else {
            success = xSemaphoreTake(h, delay);
        }
    }

    return (success == pdTRUE) ? 0 : -1;
}

void IRAM_ATTR _lock_acquire(_lock_t *lock)
{
    lock_acquire_generic(lock, portMAX_DELAY, queueQUEUE_TYPE_MUTEX);
}

void IRAM_ATTR _lock_acquire_recursive(_lock_t *lock)
{
    lock_acquire_generic(lock, portMAX_DELAY, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

int IRAM_ATTR _lock_try_acquire(_lock_t *lock)
{
    return lock_acquire_generic(lock, 0, queueQUEUE_TYPE_MUTEX);
}

int IRAM_ATTR _lock_try_acquire_recursive(_lock_t *lock)
{
    return lock_acquire_generic(lock, 0, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}

/* Release the mutex semaphore for lock.
   mutex_type is queueQUEUE_TYPE_RECURSIVE_MUTEX or queueQUEUE_TYPE_MUTEX
*/
static void IRAM_ATTR lock_release_generic(_lock_t *lock, uint8_t mutex_type)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return; /* locking is a no-op before scheduler is up */
    }
    SemaphoreHandle_t h = (SemaphoreHandle_t)(*lock);
    BK_ASSERT(h);

    if (!xPortCanYield()) {
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
            abort(); /* indicates logic bug, it shouldn't be possible to lock recursively in ISR */
        }
        BaseType_t higher_task_woken = false;
        xSemaphoreGiveFromISR(h, &higher_task_woken);
        if (higher_task_woken) {
            portYIELD_FROM_ISR(higher_task_woken);
        }
    } else {
        if (mutex_type == queueQUEUE_TYPE_RECURSIVE_MUTEX) {
#if ( configUSE_RECURSIVE_MUTEXES == 1 )
            xSemaphoreGiveRecursive(h);
#endif
        } else {
            xSemaphoreGive(h);
        }
    }
}

void IRAM_ATTR _lock_release(_lock_t *lock)
{
    lock_release_generic(lock, queueQUEUE_TYPE_MUTEX);
}

void IRAM_ATTR _lock_release_recursive(_lock_t *lock)
{
    lock_release_generic(lock, queueQUEUE_TYPE_RECURSIVE_MUTEX);
}
// eof

