/* This source file is part of the ATMEL AVR32-SoftwareFramework-1.3.0-AT32UC3A Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief FreeRTOS port source for AVR32 UC3.
 *
 * - Compiler:           GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 * \author               Michal Fita <michal.fita@gmail.com>
 *
 *****************************************************************************/

/*
	FreeRTOS.org V4.7.2 - Copyright (C) 2003-2008 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS.org is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS.org; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS.org, without being obliged to provide
	the source code for any proprietary components.  See the licensing section
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************

	Please ensure to read the configuration and relevant port sections of the 
	online documentation.

	+++ http://www.FreeRTOS.org +++
	Documentation, latest information, license and contact details.  

	+++ http://www.SafeRTOS.com +++
	A version that is certified for use in safety critical systems.

	+++ http://www.OpenRTOS.com +++
	Commercial support, development, porting, licensing and training services.

	***************************************************************************
*/


/* Standard includes. */
//#include <sys/cpu.h>
//#include <sys/usart.h>
#include <malloc.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* AVR32 UC3 includes. */
#include <avr32/io.h>
#include "gpio.h"
#if( configTICK_USE_TC==1 )
	#include "tc.h"
#endif


/* Constants required to setup the task context. */
#define portINITIAL_SR            ( ( portSTACK_TYPE ) 0x00400000 ) /* AVR32 : [M2:M0]=001 I1M=0 I0M=0, GM=0 */
#define portINSTRUCTION_SIZE      ( ( portSTACK_TYPE ) 0 )

/* Each task maintains its own critical nesting variable. */
#define portNO_CRITICAL_NESTING   ( ( unsigned portLONG ) 0 )
volatile unsigned portLONG ulCriticalNesting = 9999UL;

#if( configTICK_USE_TC==0 )
	static void prvClearCcInt( void );
#else
	static void prvClearTcInt( void );
#endif

/* Setup the timer to generate the tick interrupts. */
static void prvSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/

/**
 * \note Function _init_startup() is moved to the file \c init.c
 * in \c platform directory.
 */

/*-----------------------------------------------------------*/

/**
 * \note Functions __malloc_lock() and __malloc_unlock are moved to file
 * \c heap.c in \c platform directory.
 */

/*-----------------------------------------------------------*/

/* Added as there is no such function in FreeRTOS. */
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
/*-----------------------------------------------------------*/

/* The cooperative scheduler requires a normal IRQ service routine to
simply increment the system tick. */
/* The preemptive scheduler is defined as "naked" as the full context is saved
on entry as part of the context switch. */
__attribute__((__naked__)) static void vTick( void )
{
	/* Save the context of the interrupted task. */
	portSAVE_CONTEXT_OS_INT();

	#if( configTICK_USE_TC==1 )
		/* Clear the interrupt flag. */
		prvClearTcInt();
	#else
		/* Clear the interrupt flag. */
		prvClearCcInt();
	#endif

	/* Because FreeRTOS is not supposed to run with nested interrupts, put all OS
	calls in a critical section . */
	portENTER_CRITICAL();
		vTaskIncrementTick();
	portEXIT_CRITICAL();

	/* Restore the context of the "elected task". */
	portRESTORE_CONTEXT_OS_INT();
}
/*-----------------------------------------------------------*/

__attribute__((__naked__)) void SCALLYield( void )
{
	/* Save the context of the interrupted task. */
	portSAVE_CONTEXT_SCALL();
	vTaskSwitchContext();
	portRESTORE_CONTEXT_SCALL();
}
/*-----------------------------------------------------------*/

/* The code generated by the GCC compiler uses the stack in different ways at
different optimisation levels.  The interrupt flags can therefore not always
be saved to the stack.  Instead the critical section nesting level is stored
in a variable, which is then saved as part of the stack context. */
__attribute__((__noinline__)) void vPortEnterCritical( void )
{
	/* Disable interrupts */
	portDISABLE_INTERRUPTS();

	/* Now interrupts are disabled ulCriticalNesting can be accessed
	 directly.  Increment ulCriticalNesting to keep a count of how many times
	 portENTER_CRITICAL() has been called. */
	ulCriticalNesting++;
}
/*-----------------------------------------------------------*/

__attribute__((__noinline__)) void vPortExitCritical( void )
{
	if(ulCriticalNesting > portNO_CRITICAL_NESTING)
	{
		ulCriticalNesting--;
		if( ulCriticalNesting == portNO_CRITICAL_NESTING )
		{
			/* Enable all interrupt/exception. */
			portENABLE_INTERRUPTS();
		}
	}
}
/*-----------------------------------------------------------*/

/*
 * Initialise the stack of a task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 *
 * See header file for description.
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro. */

	/* When the task starts, it will expect to find the function parameter in R12. */
	pxTopOfStack--;
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x08080808;					/* R8 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x09090909;					/* R9 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x0A0A0A0A;					/* R10 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x0B0B0B0B;					/* R11 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) pvParameters;					/* R12 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0xDEADBEEF;					/* R14/LR */
	*pxTopOfStack-- = ( portSTACK_TYPE ) pxCode + portINSTRUCTION_SIZE; /* R15/PC */
	*pxTopOfStack-- = ( portSTACK_TYPE ) portINITIAL_SR;				/* SR */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0xFF0000FF;					/* R0 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x01010101;					/* R1 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x02020202;					/* R2 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x03030303;					/* R3 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x04040404;					/* R4 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x05050505;					/* R5 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x06060606;					/* R6 */
	*pxTopOfStack-- = ( portSTACK_TYPE ) 0x07070707;					/* R7 */
	*pxTopOfStack = ( portSTACK_TYPE ) portNO_CRITICAL_NESTING;			/* ulCriticalNesting */

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

portBASE_TYPE xPortStartScheduler( void )
{
	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	prvSetupTimerInterrupt();

	/* Start the first task. */
	portRESTORE_CONTEXT();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the AVR32 port will require this function as there
	is nothing to return to.  */
}
/*-----------------------------------------------------------*/

/* Schedule the COUNT&COMPARE match interrupt in (configCPU_CLOCK_HZ/configTICK_RATE_HZ)
clock cycles from now. */
#if( configTICK_USE_TC==0 )
	static void prvScheduleFirstTick(void)
	{
		Set_system_register(AVR32_COMPARE, configCPU_CLOCK_HZ/configTICK_RATE_HZ);
		Set_system_register(AVR32_COUNT, 0);
	}

	__attribute__((__noinline__)) static void prvClearCcInt(void)
	{
		Set_system_register(AVR32_COMPARE, Get_system_register(AVR32_COMPARE));
	}
#else
	__attribute__((__noinline__)) static void prvClearTcInt(void)
	{
		AVR32_TC.channel[configTICK_TC_CHANNEL].sr;
	}
#endif
/*-----------------------------------------------------------*/

/* Setup the timer to generate the tick interrupts. */
static void prvSetupTimerInterrupt(void)
{
#if( configTICK_USE_TC==1 )

	volatile avr32_tc_t *tc = &AVR32_TC;

	// Options for waveform genration.
	tc_waveform_opt_t waveform_opt =
	{
	.channel  = configTICK_TC_CHANNEL,             /* Channel selection. */

	.bswtrg   = TC_EVT_EFFECT_NOOP,                /* Software trigger effect on TIOB. */
	.beevt    = TC_EVT_EFFECT_NOOP,                /* External event effect on TIOB. */
	.bcpc     = TC_EVT_EFFECT_NOOP,                /* RC compare effect on TIOB. */
	.bcpb     = TC_EVT_EFFECT_NOOP,                /* RB compare effect on TIOB. */

	.aswtrg   = TC_EVT_EFFECT_NOOP,                /* Software trigger effect on TIOA. */
	.aeevt    = TC_EVT_EFFECT_NOOP,                /* External event effect on TIOA. */
	.acpc     = TC_EVT_EFFECT_NOOP,                /* RC compare effect on TIOA: toggle. */
	.acpa     = TC_EVT_EFFECT_NOOP,                /* RA compare effect on TIOA: toggle (other possibilities are none, set and clear). */

	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,/* Waveform selection: Up mode without automatic trigger on RC compare. */
	.enetrg   = FALSE,                             /* External event trigger enable. */
	.eevt     = 0,                                 /* External event selection. */
	.eevtedg  = TC_SEL_NO_EDGE,                    /* External event edge selection. */
	.cpcdis   = FALSE,                             /* Counter disable when RC compare. */
	.cpcstop  = FALSE,                             /* Counter clock stopped with RC compare. */

	.burst    = FALSE,                             /* Burst signal selection. */
	.clki     = FALSE,                             /* Clock inversion. */
	.tcclks   = TC_CLOCK_SOURCE_TC3                /* Internal source clock 3. */
	};

	tc_interrupt_t tc_interrupt =
	{
		.etrgs=0,
		.ldrbs=0,
		.ldras=0,
		.cpcs =1,
		.cpbs =0,
		.cpas =0,
		.lovrs=0,
		.covfs=0,
	};

#endif

	/* Disable all interrupt/exception. */
	portDISABLE_INTERRUPTS();

	/* Register the compare interrupt handler to the interrupt controller and
	enable the compare interrupt. */

	#if( configTICK_USE_TC==1 )
	{
		INTC_register_interrupt(&vTick, configTICK_TC_IRQ, AVR32_INTC_INT0);

		/* Initialize the timer/counter. */
		tc_init_waveform(tc, &waveform_opt);

		/* Set the compare triggers.
		Remember TC counter is 16-bits, so counting second is not possible!
		That's why we configure it to count ms. Set Rc to the right value with
      rounding.*/
		tc_write_rc( tc, configTICK_TC_CHANNEL, ( configPBA_CLOCK_HZ + 4 * configTICK_RATE_HZ ) /
		                                        ( 8 * configTICK_RATE_HZ ) );

		tc_configure_interrupts( tc, configTICK_TC_CHANNEL, &tc_interrupt );

		/* Start the timer/counter. */
		tc_start(tc, configTICK_TC_CHANNEL);
	}
	#else
	{
		INTC_register_interrupt(&vTick, AVR32_CORE_COMPARE_IRQ, AVR32_INTC_INT0);
		prvScheduleFirstTick();
	}
	#endif
}
