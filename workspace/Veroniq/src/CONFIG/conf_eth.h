/* This header file is part of the ATMEL AVR32-SoftwareFramework-1.3.0-AT32UC3A Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Ethernet module configuration file.
 *
 * This file contains the possible external configuration of the Ethernet module.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ***************************************************************************/

/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _CONF_ETH_H_
#define _CONF_ETH_H_

/*! define stack size for WEB server task */
#define lwipBASIC_WEB_SERVER_STACK_SIZE   384

/*! define stack size for TFTP server task */
#define lwipBASIC_TFTP_SERVER_STACK_SIZE  100

/*! define stack size for SMTP Client task */
#define lwipBASIC_SMTP_CLIENT_STACK_SIZE  320

/*! define stack size for lwIP task */
#define lwipINTERFACE_STACK_SIZE          512

/*! define stack size for netif task */
#define netifINTERFACE_TASK_STACK_SIZE    256

/*! define WEB server priority */
#define ethWEBSERVER_PRIORITY             ( tskIDLE_PRIORITY + 2 )

/*! define TFTP server priority */
#define ethTFTPSERVER_PRIORITY            ( tskIDLE_PRIORITY + 3 )

/*! define SMTP client priority */
#define ethSMTPCLIENT_PRIORITY            ( tskIDLE_PRIORITY + 5 )

/*! define lwIP task priority */
#define lwipINTERFACE_TASK_PRIORITY       ( configMAX_PRIORITIES - 1 )

/*! define netif task priority */
#define netifINTERFACE_TASK_PRIORITY      ( configMAX_PRIORITIES - 1 )

/*! Number of threads that can be started with sys_thread_new() */
#define SYS_THREAD_MAX                      6

/*! LED used by the ethernet task, toggled on each activation */
#define ethTASK_LED      ( 5 )

/*! ethernet task rate (in ms) */
#define eth_TASK_RATE    1000

/*! location of net config file on the file system */
#define ETHERNET_CONFIG_FILE "A:/CFG/net.txt"

/*! Phy Address (set through strap options) */
#define ETHERNET_CONF_PHY_ADDR             0x01
#define ETHERNET_CONF_PHY_ID               0x20005C90

/*! Number of receive buffers. Max ethernet frame size is 1526. A Rx buffer is
128 Bytes long. So 12 Rx buffers are necessary to store one max sized frame.
Multiply that by 2 for performance. */
#define ETHERNET_CONF_NB_RX_BUFFERS        24

/*! USE_RMII_INTERFACE must be defined as 1 to use an RMII interface, or 0
to use an MII interface. */
#define ETHERNET_CONF_USE_RMII_INTERFACE   1

/*! Number of Transmit buffers */
#define ETHERNET_CONF_NB_TX_BUFFERS        10

/*! Size of each Transmit buffer. */
#define ETHERNET_CONF_TX_BUFFER_SIZE       512

/*! Clock definition */
#define ETHERNET_CONF_SYSTEM_CLOCK         48000000

/*! Allow Auto Negociation */
#define ETHERNET_CONF_AN_ENABLE            1

/*! Use auto cross capability. Unused because not supported by the DP83848 phy
    on the EVK1100. */
#define ETHERNET_CONF_AUTO_CROSS_ENABLE    0
/*! use direct cable */
#define ETHERNET_CONF_CROSSED_LINK         0

/* ethernet default parameters */
/*! MAC address definition.  The MAC address must be unique on the network. */
#define ETHERNET_CONF_ETHADDR0   0x00
#define ETHERNET_CONF_ETHADDR1   0x04
#define ETHERNET_CONF_ETHADDR2   0x25
#define ETHERNET_CONF_ETHADDR3   0x00
#define ETHERNET_CONF_ETHADDR4   0x01
#define ETHERNET_CONF_ETHADDR5   0x02

/*! The IP address being used. */
#define ETHERNET_CONF_IPADDR0          192
#define ETHERNET_CONF_IPADDR1          168
#define ETHERNET_CONF_IPADDR2            0
#define ETHERNET_CONF_IPADDR3            2

/*! The gateway address being used. */
#define ETHERNET_CONF_GATEWAY_ADDR0    192
#define ETHERNET_CONF_GATEWAY_ADDR1    168
#define ETHERNET_CONF_GATEWAY_ADDR2      0
#define ETHERNET_CONF_GATEWAY_ADDR3      1

/*! The network mask being used. */
#define ETHERNET_CONF_NET_MASK0        255
#define ETHERNET_CONF_NET_MASK1        255
#define ETHERNET_CONF_NET_MASK2        255
#define ETHERNET_CONF_NET_MASK3          0

/*! set to 1 if Phy status changes handle an interrupt */
#define ETHERNET_CONF_USE_PHY_IT           0

#endif
