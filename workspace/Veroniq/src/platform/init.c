#include <sys/cpu.h>
#include <sys/usart.h>

#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"
#include "sdramc.h"
#include "intc.h"
#include "flashc.h"

// #include "wdt.h"

#include "ctrl_access.h"

#include "FreeRTOS.h"

/*
 * Low-level initialization routine called during startup, before the main
 * function.
 * This version comes in replacement to the default one provided by Newlib.
 * Newlib's _init_startup only calls init_exceptions, but Newlib's exception
 * vectors are not compatible with the SCALL management in the current FreeRTOS
 * port. More low-level initializations are besides added here.
 */
void _init_startup(void)
{
	/* Import the Exception Vector Base Address. */
	extern void _evba;

	#if configHEAP_INIT
		extern void __heap_start__;
		extern void __heap_end__;
		portBASE_TYPE *pxMem;
	#endif

	/* Load the Exception Vector Base Address in the corresponding system register. */
	Set_system_register( AVR32_EVBA, ( int ) &_evba );

	/* Enable exceptions. */
	ENABLE_ALL_EXCEPTIONS();

	/* Initialize interrupt handling. */
	INTC_init_interrupts();

	pm_freq_param_t freq_param = {
			.cpu_f = configCPU_CLOCK_HZ,
			.pba_f = configPBA_CLOCK_HZ,
			.osc0_f = FOSC0,
			.osc0_startup = OSC0_STARTUP
	};
	
	/* Configure CPU by PLL clocking */
	pm_configure_clocks(&freq_param);
	
	/* Switch to external oscillator 0. */
	//pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
#if 0
	pm_pll_setup(&AVR32_PM, 0,  // pll.
	               15,  // mul.
	               1,   // div.
	               0,   // osc.
	               16); // lockcount.
	pm_pll_set_option(&AVR32_PM, 0, // pll.
	                    0,  // pll_freq.
	                    1,  // pll_div2.
	                    0); // pll_wbwdisable.
	pm_pll_enable(&AVR32_PM, 0);
	
	/* Wait until PLL lock in phase */
	pm_wait_for_pll0_locked(&AVR32_PM);
	
	/* Select all the power manager clocks */
	pm_cksel(&AVR32_PM,
	           1,   // pbadiv.
	           0,   // pbasel.
	           1,   // pbbdiv.
	           0,   // pbbsel.
	           1,   // hsbdiv.
	           0);  // hsbsel.
	
	/* Set flash memory wait state to 1*/
	flashc_set_wait_state(1);
	
	/* Switch to clock driven from PLL */
	pm_switch_to_clock(&AVR32_PM, AVR32_PM_MCCTRL_MCSEL_PLL0); 

#endif
	
	/* Initialize SDRAM Controller and external SDRAM chip. */ 
	sdramc_init( configCPU_CLOCK_HZ );
	
	#if configHEAP_INIT

		/* Initialize the heap used by malloc. */
		for( pxMem = &__heap_start__; pxMem < ( portBASE_TYPE * )&__heap_end__; )
		{
			*pxMem++ = 0xA5A5A5A5;
		}

	#endif

	/* Give the used CPU clock frequency to Newlib, so it can work properly. */
	set_cpu_hz( configCPU_CLOCK_HZ );	
		
	/* Code section present if and only if the debug trace is activated. */
	#if configDBG
	{
		static const gpio_map_t DBG_USART_GPIO_MAP =
		{
			{ configDBG_USART_RX_PIN, configDBG_USART_RX_FUNCTION },
			{ configDBG_USART_TX_PIN, configDBG_USART_TX_FUNCTION }
		};

		/* Initialize the USART used for the debug trace with the configured parameters. */
		set_usart_base( ( void * ) configDBG_USART );
		gpio_enable_module( DBG_USART_GPIO_MAP,
		                    sizeof( DBG_USART_GPIO_MAP ) / sizeof( DBG_USART_GPIO_MAP[0] ) );
		usart_init( configDBG_USART_BAUDRATE );
	}
	#endif
}
