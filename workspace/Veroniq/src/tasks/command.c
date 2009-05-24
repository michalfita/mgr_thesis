#include <avr32/io.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "gpio.h"
#include "usart.h"
#include "hashmap.h"
#include "serial.h"
#include "command.h"
#include "version.h"
#include "motor.h"

#define COMMAND_USART               (&AVR32_USART1)
#define COMMAND_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#define COMMAND_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#define COMMAND_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#define COMMAND_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION

#define COMMAND_ECHO_MODE 1 /* 0 - off, 1 - enable simple echo, 2 - add _ to echo */

typedef int (*cmd_function_t)(int argc, char* argv[]);

typedef struct
{
	char* command;
	cmd_function_t call;
} command_entry_t;

/* Forward declarations for functions handling particular commands */
int cmd_identify(int argc, char* argv[]); // int cmd_identify(char* cmd, ...);
int cmd_setpin(int argc, char* argv[]); // int cmd_setpin(char* cmd, ...);

static const command_entry_t commands_table[] = {
		{"identify", cmd_identify},
		{"setpin", cmd_setpin},
		{NULL, NULL}
};

static hashmap_t commands_hashmap;

/* Handler for serial port used for command entry */
extern xComPortHandle serialPortHdlr;

/* Commands implementation */
int cmd_identify(int argc, char* argv[])
{
	const portCHAR* text = "Robot Firmware for AT32UC3A0512 codename \"Veroniq\"\n"\
                           "version "VERSION_STR" (build "BUILD_NUMBER_STR" date "BUILD_DATE")\n";
	usUsartPutString(serialPortHdlr, text, strlen(text));

	return 0;
}

int cmd_setpin(int argc, char* argv[])
{
	int status = 0;
	motor_queue_msg_t msg;
	
	if (argc > 2) {
		msg.pin = atoi(argv[1]);
		msg.value = atoi(argv[2]);
		if (argc > 3) {
			msg.time = atoi(argv[3]);
		}
		else {
			msg.time = 0; /* zero means no timeout of command */
		}
		
		if (pdTRUE != xQueueSend(motor_queue, &msg, 200))
		{
			printf("Error: Failed to send message to motor task.\n");
			status = 2;
		}
	} else {
		printf("Error: At least 2 arguments have to be provided.\n");
		status = 1;
	}
	return status;
}

/* Initialization of serial port */
void command_gpio_init()
{
	static const gpio_map_t USART_GPIO_MAP =
	{
	  {COMMAND_USART_RX_PIN, COMMAND_USART_RX_FUNCTION},
	  {COMMAND_USART_TX_PIN, COMMAND_USART_TX_FUNCTION}
	};

	// USART options.
	static const usart_options_t USART_OPTIONS =
	{
	  .baudrate     = 9600, //57600,
	  .charlength   = 8,
	  .paritytype   = USART_NO_PARITY,
	  .stopbits     = USART_1_STOPBIT,
	  .channelmode  = USART_NORMAL_CHMODE
	};

	// Assign GPIO to USART.
	//gpio_enable_module(USART_GPIO_MAP,
	//                   sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	
	// Initialize USART in RS232 mode.
	//usart_init_rs232(COMMAND_USART, &USART_OPTIONS, FOSC0);

	// Initialize serial driver
	serialPortHdlr = xUsartInit(serCOM1, COMMAND_UART_SPEED, 128, 128);
	
}

static portTASK_FUNCTION(command_task, p_parameters)
{
	static portCHAR c;
	static portCHAR* buffer_b = NULL; // begining marker
	static portCHAR* buffer_e = NULL; // end marker
	static portLONG  buffer_l = 0; // buffer length

	static portCHAR* line_tokenized[24] = {NULL};
	static portSHORT token_number = 0;
	static portCHAR* reentrant_token = NULL;
	
	command_entry_t* cmd_ptr = (command_entry_t*)&(commands_table[0]); // Init by begining of the table
	void* cmd_func_ptr;
	cmd_function_t cmd_func = NULL;
	
	/* Process inital commands hashing */
	commands_hashmap = hashmap_create(20);
	while(cmd_ptr->command != NULL)
	{
		hashmap_insert(commands_hashmap, cmd_ptr->command, &(cmd_ptr->call), sizeof(cmd_function_t));
		cmd_ptr++;
	}
	
	/* Enter endless task loop */
	while(1)
	{
		if(pdTRUE == xUsartGetChar(serialPortHdlr, &c, 400))
		{
#if COMMAND_ECHO_MODE >= 1
			xUsartPutChar(serialPortHdlr, c, 400);
#if COMMAND_ECHO_MODE >= 2
			xUsartPutChar(serialPortHdlr, '_', 400);
#endif /* COMMAND_ECHO_MODE >= 2 */
#endif /* COMMAND_ECHO_MODE >= 1 */
			if(('\n' != c) && ('\r' != c))
			{
				if(NULL == buffer_b)
				{
					buffer_b = malloc(128);
					buffer_e = buffer_b;
				}
				*buffer_e = c;
				buffer_e++;
				if ((buffer_e - buffer_b) > buffer_l) // increase buffer if it is not enough
				{
					int buff_len = buffer_e - buffer_b;
				  	buffer_l += 128;
				  	buffer_b = (portCHAR*)realloc(buffer_b, buffer_l);
				  	buffer_e = buffer_b + buff_len;
				}
			}
			else /* '\n' == c || '\r' == c */
			{
				xUsartPutChar(serialPortHdlr, '\n', 400);
				if(NULL != buffer_e)
				{
					*buffer_e = '\0'; // Finalize string in buffer
					if(strlen(buffer_b) > 0)
					{
						/* Parse buffer */
						token_number = 0;
						line_tokenized[token_number] = strtok_r(buffer_b, " \t", &reentrant_token);
	  					while(NULL != line_tokenized[token_number])
	  					{
	  						if ('\0' == *(line_tokenized[token_number])) continue;
	  						token_number++;
	  						line_tokenized[token_number] = strtok_r(NULL, " \t", &reentrant_token);
	  					}
						if(0 != hashmap_entry_by_key(commands_hashmap, line_tokenized[0], &cmd_func_ptr))
						{
							/* Have to cast - is it better way? */
							cmd_func = *(cmd_function_t*)cmd_func_ptr;
							/* Call command function */
							(*cmd_func)(token_number, line_tokenized);
#if COMMAND_DEBUG_MODE == 1
							static portCHAR output[200] = {'\0'};
							snprintf(output, 200, "Test %x, %x\r", (unsigned int)commands_table[0].call, (unsinged int)cmd_func);
							usUsartPutString(serialPortHdlr, output, strlen(output));
#endif /* COMMAND_DEBUG_MODE == 1 */
						}
						else
						{
							printf("Unknown command.\n");
						}
					}
					/* Free buffers after enter key */
					free(buffer_b);
				}
				buffer_e = buffer_b = NULL;
			}
		}
		taskYIELD();
	}
}

void command_start(unsigned portBASE_TYPE priority)
{
	/* Init GPIO */
	command_gpio_init();
	
	/* Spawn the command task. */
	xTaskCreate(command_task, (const signed portCHAR*)"COMMAND",
	            COMMAND_STACK_SIZE, NULL, priority, (xTaskHandle*)NULL);
}
