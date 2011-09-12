/*
# This file is Copyright 2006, 2007, 2009 Dean Hall.
#
# This file is part of the Python-on-a-Chip program.
# Python-on-a-Chip is free software: you can redistribute it and/or modify
# it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE Version 2.1.
#
# Python-on-a-Chip is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# A copy of the GNU LESSER GENERAL PUBLIC LICENSE Version 2.1
# is seen in the file COPYING up one directory from this.
*/


#undef __FILE_ID__
#define __FILE_ID__ 0x70


/** PyMite platform-specific routines for AVR32 target */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr32/io.h>

#include "pymite.h"

#include "FreeRTOS.h"
#include "serial.h"
#include "task.h"


#define UART_BAUD 115200UL

/* Handler for serial port used for command entry */
extern xComPortHandle serialPortHdlr;

/**
 * When defined, the AVR target configures Timer/Counter0 to generate an
 * overflow interrupt to call pm_vmPeriodic().
 * If you configure T/C0 yourself, disable this define and be sure to
 * periodically call pm_vmPeriodic(usec)!
 * Has no meaning on non-AVR.
 */
#define AVR_DEFAULT_TIMER_SOURCE


#ifdef AVR_DEFAULT_TIMER_SOURCE

/* Hint: 1,000,000 �s/s * 256 T/C0 clock cycles per tick * 8 CPU clocks per
 * T/C0 clock cycle / x,000,000 CPU clock cycles per second -> �s per tick
 */
#define PLAT_TIME_PER_TICK_USEC (1000000ULL*256ULL*8ULL/F_CPU)

#endif /* AVR_DEFAULT_TIMER_SOURCE */


/* Configure stdin, stdout, stderr */
//static int uart_putc(char c, FILE *stream);
//static int uart_getc(FILE *stream);
//FILE avr_uart = FDEV_SETUP_STREAM(uart_putc, uart_getc, _FDEV_SETUP_RW);


/*
 * AVR target shall use stdio for I/O routines.
 * The UART or USART must be configured for the interactive interface to work.
 */
PmReturn_t
plat_init(void)
{
    /* PORT BEGIN: Set these UART/USART SFRs properly for your AVR */

    /* PORT END */

    return PM_RET_OK;
}


PmReturn_t
plat_deinit(void)
{
    /* Disable UART */

#if (TARGET_MCU == uc3a0512) || (TARGET_MCU == uc3a0256) || (TARGET_MCU == uc3a0128)

#endif /* AVR_DEFAULT_TIMER_SOURCE */

    return PM_RET_OK;
}


#ifdef AVR_DEFAULT_TIMER_SOURCE
//ISR(TIMER0_OVF_vect)
//{
    /* TODO Find a clever way to handle bad return code, maybe use
     * PM_REPORT_IF_ERROR(retval) when that works on AVR inside an
     * interrupt.
     */
//    pm_vmPeriodic(PLAT_TIME_PER_TICK_USEC);//
//}
#endif


/*
 * Gets a byte from the address in the designated memory space
 * Post-increments *paddr.
 */
uint8_t
plat_memGetByte(PmMemSpace_t memspace, uint8_t const **paddr)
{
    uint8_t b = 0;

    switch (memspace)
    {
        case MEMSPACE_PROG:
        case MEMSPACE_EEPROM:
        case MEMSPACE_RAM:
            b = **paddr;
            *paddr += 1;
            return b;

        case MEMSPACE_SEEPROM:
        case MEMSPACE_OTHER0:
        case MEMSPACE_OTHER1:
        case MEMSPACE_OTHER2:
        case MEMSPACE_OTHER3:
        default:
            return 0;
    }
}


//static int
//uart_getc(FILE *stream)
//{
//    char c;
//
//    /* Wait for reception of a byte */
//
//
//    return c;
//}


//static int
//uart_putc(char c, FILE *stream)
//{
//
//    return 0;
//}


/*
 * UART receive char routine MUST return exactly and only the received char;
 * it should not translate \n to \r\n.
 * This is because the interactive interface uses binary transfers.
 */
PmReturn_t
plat_getByte(uint8_t *b)
{
    PmReturn_t retval = PM_RET_OK;

    /* PORT BEGIN: Set these UART/USART SFRs properly for your AVR */

    /* PORT END */

    return retval;
}


/*
 * UART send char routine MUST send exactly and only the given char;
 * it should not translate \n to \r\n.
 * This is because the interactive interface uses binary transfers.
 */
PmReturn_t
plat_putByte(uint8_t b)
{
    /* PORT BEGIN: Set these UART/USART SFRs properly for your AVR */
    /* Using ASF drivers */
    if (pdFAIL == xUsartPutChar(serialPortHdlr, b, 400))
    {
        return PM_RET_ERR;
    }
    /* PORT END */

    return PM_RET_OK;
}


/*
 * This operation is made atomic by temporarily disabling
 * the interrupts. The old state is restored afterwards.
 */
PmReturn_t
plat_getMsTicks(uint32_t *r_ticks)
{
    /* Critical section start */
    *r_ticks = xTaskGetTickCount();
    /* Critical section end */
    return PM_RET_OK;
}


void
plat_reportError(PmReturn_t result)
{
    static portCHAR output[200] = {'\0'};

    /* Print error */
    snprintf(output, 200, "Error:     0x%02X\n", result);
    usUsartPutString(serialPortHdlr, output, strlen(output));
    snprintf(output, 200, "  Release: 0x%02X\n", gVmGlobal.errVmRelease);
    usUsartPutString(serialPortHdlr, output, strlen(output));
    snprintf(output, 200, "  FileId:  0x%02X\n", gVmGlobal.errFileId);
    usUsartPutString(serialPortHdlr, output, strlen(output));
    snprintf(output, 200, "  LineNum: %d\n", gVmGlobal.errLineNum);
    usUsartPutString(serialPortHdlr, output, strlen(output));

    /* Print traceback */
    {
        pPmObj_t pframe;
        pPmObj_t pstr;
        PmReturn_t retval;

        const char* traceback_str = "Traceback (top first):";
        usUsartPutString(serialPortHdlr, traceback_str, strlen(traceback_str));

        /* Get the top frame */
        pframe = (pPmObj_t)gVmGlobal.pthread->pframe;

        /* If it's the native frame, print the native function name */
        if (pframe == (pPmObj_t)&(gVmGlobal.nativeframe))
        {

            /* The last name in the names tuple of the code obj is the name */
            retval = tuple_getItem((pPmObj_t)gVmGlobal.nativeframe.nf_func->
                                   f_co->co_names, -1, &pstr);
            if ((retval) != PM_RET_OK)
            {
                const char* unable_str = "  Unable to get native func name.";
                usUsartPutString(serialPortHdlr, unable_str, strlen(unable_str));
                return;
            }
            else
            {
                snprintf(output, 200, "  %s() __NATIVE__\n", ((pPmString_t)pstr)->val);
                usUsartPutString(serialPortHdlr, output, strlen(output));
            }

            /* Get the frame that called the native frame */
            pframe = (pPmObj_t)gVmGlobal.nativeframe.nf_back;
        }

        /* Print the remaining frame stack */
        for (;
             pframe != C_NULL;
             pframe = (pPmObj_t)((pPmFrame_t)pframe)->fo_back)
        {
            /* The last name in the names tuple of the code obj is the name */
            retval = tuple_getItem((pPmObj_t)((pPmFrame_t)pframe)->
                                   fo_func->f_co->co_names, -1, &pstr);
            if ((retval) != PM_RET_OK) break;

            snprintf(output, 200, "  %s()\n", ((pPmString_t)pstr)->val);
            usUsartPutString(serialPortHdlr, output, strlen(output));
        }
        const char* module_str = "  <module>.\n";
        usUsartPutString(serialPortHdlr, module_str, strlen(module_str));
    }
}

