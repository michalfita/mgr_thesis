/*! **************************************************************************
 * \file vq_init.c
 *
 * \brief Clock initialization for FreeRTOS on AVR32. File port.c is modified
 *        to call vq_init_cloc().
 *
 * - Compiler:           GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 * - AppNote:
 *
 * \author               Michal Fita <michal.fita@gmail.com>
 *
 *****************************************************************************/

#include <nlao_cpu.h>
#include <nlao_usart.h>

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

void vq_init_clock(void)
{
	int pm_status;

	/* Put clock configuration into structure */
	pm_freq_param_t freq_param = {
			.cpu_f = configCPU_CLOCK_HZ,
			.pba_f = configPBA_CLOCK_HZ,
			.osc0_f = FOSC0,
			.osc0_startup = OSC0_STARTUP
	};

	/* Configure CPU by PLL clocking */
	pm_status = pm_configure_clocks(&freq_param);
	if (pm_status != PM_FREQ_STATUS_OK)
	{
		/* TODO: create initialization error hang with LED flash */
	}

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
}
