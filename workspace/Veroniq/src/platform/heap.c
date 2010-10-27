
/**
 * This file contains implementation fo the layer for cooperation of FreeRTOS
 * and newlib, which allows safely use of malloc() directly as we have some 
 * code which uses it.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "heap.h"

/*
 * Original comment to two below was (no longer fully true from 16-05-2009):
 * 
 * malloc, realloc and free are meant to be called through respectively
 * pvPortMalloc, pvPortRealloc and vPortFree.
 * The latter functions call the former ones from within sections where tasks
 * are suspended, so the latter functions are task-safe. __malloc_lock and
 * __malloc_unlock use the same mechanism to also keep the former functions
 * task-safe as they may be called directly from Newlib's functions.
 * However, all these functions are interrupt-unsafe and SHALL THEREFORE NOT BE
 * CALLED FROM WITHIN AN INTERRUPT, because __malloc_lock and __malloc_unlock do
 * not call portENTER_CRITICAL and portEXIT_CRITICAL in order not to disable
 * interrupts during memory allocation management as this may be a very time-
 * consuming process.
 */

/**
 * The recursive mutex for heap allocation.
 * 
 * \see xSemaphoreCreateRecursiveMutex
 */

xSemaphoreHandle g_xHeapRecursiveMutex;

/**
 * Forward declarations
 */
static void l_vEmptyFunction();
static void l_vRaiseProtection();
static void l_vLowerProtection();

/**
 * Function pointers to be used in protection mechanisms.
 */
static void (*l_pxRaiseProtection)() = l_vEmptyFunction;
static void (*l_pxLowerProtection)() = l_vEmptyFunction;

/**
 * The heap_management_init() function.
 *
 * It creates g_xHeapRecursiveMutex and enables mechanics protecting memory
 * allocations in __malloc_lock() and __malloc_unlock()
 *
 */
void heap_management_init()
{
	//**
    //** Create mutex for heap management locks.
    //**
    g_xHeapRecursiveMutex = xSemaphoreCreateRecursiveMutex();
    if (NULL != g_xHeapRecursiveMutex)
    {
    	l_pxRaiseProtection = l_vRaiseProtection;
    	l_pxLowerProtection = l_vLowerProtection;
    }
}

/**
 * The local l_vEmptyFunction() function.
 *
 * The empty function to point to the protection function pointers before
 * initialization of mutexes.
 */
static void l_vEmptyFunction()
{
	return;
}

static void l_vRaiseProtection()
{
	// Take mutex or wait assumed infinity.
	xSemaphoreTakeRecursive(g_xHeapRecursiveMutex, 99999);
}

static void l_vLowerProtection()
{
	// Give the mutex.
	xSemaphoreGiveRecursive(g_xHeapRecursiveMutex);
}

/**
 * The __malloc_lock() function.
 * 
 * It implements locking mechanism for memory allocation.
 * 
 * Refer to: http://www.embedded.com/story/OEG20020103S0073
*/
void __malloc_lock ( struct _reent *_r )
{
	l_pxRaiseProtection();
}

/**
 * The __malloc_unlock() function.
 * 
 * It implements unlocking mechanism for memory allocation.
 * 
 * Refer to: http://www.embedded.com/story/OEG20020103S0073
*/
void __malloc_unlock ( struct _reent *_r )
{
	l_pxLowerProtection();
}

/*
 * Implementation of pvPortMalloc() and vPortFree() that relies on the
 * compilers own malloc() and free() implementations.
 *
 * This file can only be used if the linker is configured to to generate
 * a heap memory area.
 *
 * See heap_2.c and heap_1.c for alternative implementations, and the memory
 * management pages of http://www.FreeRTOS.org for more information.
 */

/**
 * The pvPortMalloc() function.
 *
 * It implements the call required by FreeRTOS, has been moved from original
 * port in heap3.c and modified accordingly.
 *
 * The idea is to remove protection mechanisms here.
 *
 * @param size The size of the memory to allocate.
*/

#if 0
void *pvPortMalloc( size_t xWantedSize )
{
	void *pvReturn;

	vTaskSuspendAll();
	{
		pvReturn = malloc( xWantedSize );
	}
	xTaskResumeAll();

	return pvReturn;
}
#endif
/*-----------------------------------------------------------*/

#if 0
void vPortFree( void *pv )
{
	if( pv )
	{
		vTaskSuspendAll();
		{
			free( pv );
		}
		xTaskResumeAll();
	}
}
#endif

/**
 * The pvPortRealloc() function.
 *
 * It implements the call required by FreeRTOS, has been moved from original
 * port in port.c (put there by error) and modified accordingly.
 *
 * The idea is to remove protection mechanisms here.
 *
 * @param size The size of the memory to allocate.
*/
#if 0
void *pvPortRealloc( void *pv, size_t xWantedSize )
{
	void *pvReturn;

	vTaskSuspendAll();
	{
		pvReturn = realloc( pv, xWantedSize );
	}
	xTaskResumeAll();

	return pvReturn;
}
#endif
