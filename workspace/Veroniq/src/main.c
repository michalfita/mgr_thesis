#include <string.h>

#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"
// #include "wdt.h"

#include "ctrl_access.h"

//! Scheduler include files.
#include "FreeRTOS.h"
#include "task.h"

//! Project own include files
#include "tasks/init.h"
#include "platform/heap.h"

/*!
 * \brief Reconfigure HMATRIX to better suit our needs.
 * \refer http://www.atmel.com/dyn/resources/prod_documents/doc32058.pdf pages 132 to 144,
 *        Chapter 19.
 */
static void init_hmatrix(void)
{
  // For the internal-flash HMATRIX slave, use last master as default.
  union
  {
     unsigned long                 scfg;
     avr32_hmatrix_scfg_t          SCFG;
  } u_avr32_hmatrix_scfg = {AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_FLASH]};

  union
  {
    unsigned long                  mcfg;
    avr32_hmatrix_mcfg_t           MCFG;
  } u_avr32_hmatrix_mcfg = {AVR32_HMATRIX.mcfg[AVR32_HMATRIX_MASTER_CPU_INSN]};

  /* Set last default master for FLASH */
  u_avr32_hmatrix_scfg.SCFG.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
  AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_FLASH] = u_avr32_hmatrix_scfg.scfg;

  /* Set last default master for PBA to which TC is connected */
  u_avr32_hmatrix_scfg.scfg = AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_PBA];
  u_avr32_hmatrix_scfg.SCFG.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
  AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_PBA] = u_avr32_hmatrix_scfg.scfg;

  /* Set undefined burst length type of instruction master to one */
  u_avr32_hmatrix_mcfg.MCFG.ulbt = AVR32_HMATRIX_ULBT_SINGLE;
  AVR32_HMATRIX.mcfg[AVR32_HMATRIX_MASTER_CPU_INSN] = u_avr32_hmatrix_mcfg.mcfg;
}

int main( void )
{
   // Disable the WDT.
   // wdt_disable();

   //**
   //** 1) Initialize the microcontroller and the shared hardware resources of the board.
   //**

   // switch to external oscillator 0 (done in \c _init_startup() in \c port.c)
   //pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

   // Initialize the delay driver.
   delay_init(configCPU_CLOCK_HZ);

   // Init USB & MACB clock.
   //prv_clk_gen_start();

   // initialize AT45DBX resources: GPIO, SPI and AT45DBX
   //prv_at45dbx_resources_init();
   
#if SD_MMC_MEM == ENABLE
   prv_sd_mmc_resources_init();
#endif

   // Setup the LED's for output.
   //LED_Off( LED0 ); LED_Off( LED1 ); LED_Off( LED2 ); LED_Off( LED3 );
   //LED_Off( LED4 ); LED_Off( LED5 ); LED_Off( LED6 ); LED_Off( LED7 );
   // vParTestInitialise();

   // Init the memory module.
   if( FALSE == ctrl_access_init() )
   {
      // TODO: Add msg on LCD.
      // gpio_clr_gpio_pin( 60 );
      while( 1 );
   }

   /* check if the AT45DBX mem is OK */
   //while( CTRL_GOOD != mem_test_unit_ready( LUN_ID_AT45DBX_MEM ) )
   //{
      // TODO: Add msg on LCD.
      // gpio_clr_gpio_pin( 61 );
   //}

   // Init the time module.
   //v_cptime_Init();

   //**
   //** Init memory allocation multitask protection mechanisms.
   //** (process now handled internally)
   //heap_management_init();
   
   //**
   //** Set HMATRIX for better performance of flash
   //**
   init_hmatrix();
   //**
   //** Start main task here.
   //**
   //vSupervisor_Start( mainSUPERVISOR_TASK_PRIORITY );
   init_start(INIT_TASK_PRIORITY);

   //**
   //** 3) Start FreeRTOS.
   //**
   // Use preemptive scheduler define configUSE_PREEMPTION as 1 in portmacro.h
   vTaskStartScheduler();

   /* Should never reach this point so why not returning the meaning of life. */
   return 42;
}
/*-----------------------------------------------------------*/
