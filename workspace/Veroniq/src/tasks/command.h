/*!
 * \file
 * \brief Header file for command module.
 * \author Michał Fita <michal.fita@gmail.com>
 */
#ifndef COMMAND_H_
#define COMMAND_H_

/*! The command task stack size. */
#define COMMAND_STACK_SIZE	        ( configMINIMAL_STACK_SIZE + 2048 )
/*! The command task priority. */
#define COMMAND_TASK_PRIORITY       ( tskIDLE_PRIORITY + 10 )
/*! Speed of serial interface */
#define COMMAND_UART_SPEED          ( 115200 )

void command_start(unsigned portBASE_TYPE priority);

#endif /*COMMAND_H_*/
