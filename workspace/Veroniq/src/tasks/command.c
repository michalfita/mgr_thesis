/*!
 * \file
 * \brief Implementation of textual command interface.
 * \author Michał Fita <michal.fita@gmail.com>
 */
#include <nlao_io.h>
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
#include "timer.h"
#include "pymite.h"
#include "statemachine.h"

#define HASH_FUNCTION HASH_FNV
#include "uthash.h"
#undef uthash_fatal
#define uthash_fatal(msg) goto hash_error;


#define COMMAND_USART               (&AVR32_USART1)
#define COMMAND_USART_RX_PIN        AVR32_USART1_RXD_0_0_PIN
#define COMMAND_USART_RX_FUNCTION   AVR32_USART1_RXD_0_0_FUNCTION
#define COMMAND_USART_TX_PIN        AVR32_USART1_TXD_0_0_PIN
#define COMMAND_USART_TX_FUNCTION   AVR32_USART1_TXD_0_0_FUNCTION

#define COMMAND_ECHO_MODE 1 /* 0 - off, 1 - enable simple echo, 2 - add _ to echo */

typedef int (*cmd_function_t)(int argc, char* argv[]);

/*!
 * \brief Type of structure holding single entry for command.
 */
typedef struct
{
    UT_hash_handle  hh;      /*!< Handle for UTHASH macros library */
	char*           command; /*!< Null terminated string of the command name used in MMI */
	cmd_function_t  call;    /*!< The pointer to the function tied to the command, executed when demanded from MMI */
} command_entry_t;

/* Forward declarations for functions handling particular commands */
static int cmd_identify(int argc, char* argv[]); // int cmd_identify(char* cmd, ...);
static int cmd_setpin(int argc, char* argv[]); // int cmd_setpin(char* cmd, ...);
static int cmd_arm(int argc, char* argv[]);
static int cmd_shoulder(int argc, char* argv[]);
static int cmd_set_test_period(int argc, char* argv[]);
static int cmd_kicktimer(int argc, char* argv[]);
static int cmd_python(int argc, char* argv[]);
static int cmd_walk(int argc, char* argv[]);
static int cmd_stop(int argc, char* argv[]);

/*!
 * The table contains mapping from command names to respective functions.
 */
static command_entry_t commands_table[] = {
		{ {}, "identify",        cmd_identify,         },
		{ {}, "setpin",          cmd_setpin,           },
		{ {}, "arm",             cmd_arm,              },
		{ {}, "shoulder",        cmd_shoulder,         },
		{ {}, "set_test_period", cmd_set_test_period,  },
		{ {}, "kicktimer",       cmd_kicktimer,        },
		{ {}, "python",          cmd_python,           },
		{ {}, "walk",            cmd_walk,             },
		{ {}, "stop",            cmd_stop,             },
		{ {}, NULL,              NULL,                 },
};

//static hashmap_t commands_hashmap;
static command_entry_t* commands_hashmap = NULL;

/* Handler for serial port used for command entry */
extern xComPortHandle serialPortHdlr;

/******************************
 *  Commands implementation   *
 ******************************/

/*!
 * Implements 'identify' command replying with version of the firmware.
 * \param argc Parameters counter
 * \param argv Parameters array
 * \return Always zero.
 */
static int cmd_identify(int argc, char* argv[])
{
	const portCHAR* text = "Robot Firmware for AT32UC3A0512 codename \"Veroniq\"\n"\
                           "version "VERSION_STR" (build "BUILD_NUMBER_STR" date "BUILD_DATE")\n";
	usUsartPutString(serialPortHdlr, text, strlen(text));

	return 0;
}

/*!
 * \brief Implements 'setpin' command setting particular pin to the value.
 *
 * For PWM pins and GPIO pins value can be 0 or 1 to lower or raise the pin
 * voltage; for PWM pins it can be from 100 to 200 where the value minus 100
 * is the duty cycle of PWM waveform.
 * \param argc Parameters counter
 * \param argv Parameters array
 * \return Always zero.
 */
static int cmd_setpin(int argc, char* argv[])
{
	int status = 0;
	int pin = 0;
	int value = 0;
	int time = 0;
	
	if (argc > 2) {
		pin = atoi(argv[1]);
		value = atoi(argv[2]);
		if (argc > 3) {
			time = atoi(argv[3]);
		}
		else {
			time = 0; /* zero means no timeout of command */
		}
		
		if (FALSE == motor_send_message(pin, value, time))
		{
			status = 2;
		}
	} else {
		const char error[] = "Error: At least 2 arguments have to be provided.\n";
		usUsartPutString(serialPortHdlr, error, sizeof(error));
		status = 1;
	}
	return status;
}

static int cmd_arm(int argc, char* argv[])
{
    return 0;
}

static int cmd_shoulder(int argc, char* argv[])
{
    return 0;
}

/*!
 * \brief Implements 'set_test_period' command setting particular period for timer interrupt.
 *
 * This is atoi(argv[1]for development use only.
 * \param argc Parameters counter
 * \param argv Parameters array
 * \return Always zero.
 */
static int cmd_set_test_period(int argc, char* argv[])
{
    if (argc >= 1)
    {
        ts_set_test_period(atoi(argv[1]));
    }
    else
    {
        const char error[] = "Error: Provide one argument: number of microseconds.";
        usUsartPutString(serialPortHdlr, error, sizeof(error));
    }
    return 0;
}

/*!
 * \brief Implements 'kicktimer' command scheduling some operation to be launched.
 *
 * This is atoi(argv[1]for development use only.
 * \param argc Parameters counter
 * \param argv Parameters array
 * \return Always zero.
 */
static int cmd_kicktimer(int argc, char* argv[])
{
    if (argc >= 1)
    {
        extern void ts_calculate_latency_cb(ts_callback_data_t* cdata_p);
        ts_period_schedule(ts_calculate_latency_cb, (void*)NULL, 500);
    }
    else
    {
        const char* error = "Error: Provide one argument: number of microseconds.";
        usUsartPutString(serialPortHdlr, error, strlen(error));
    }
    return 0;
}

/*!
 * \brief Command to execute Python example.
 * \param argc
 * \param argv
 * \return Status from Python execution.
 */
static int cmd_python(int argc, char* argv[])
{
    #define PM_HEAP_SIZE 0x2F00

    extern unsigned char usrlib_img[];
    static portCHAR output[200] = {'\0'};

    uint8_t* heap_p;
    PmReturn_t retval;

    heap_p = malloc(PM_HEAP_SIZE);

    retval = pm_init(heap_p, PM_HEAP_SIZE, MEMSPACE_PROG, usrlib_img);
    snprintf(output, 200, "* PyMite init result = 0x%x\r", retval);
    usUsartPutString(serialPortHdlr, output, strlen(output));

    PM_RETURN_IF_ERROR(retval);

    retval = pm_run((uint8_t *)"main");

    snprintf(output, 200, "* PyMite exec result = 0x%x\r", retval);
    usUsartPutString(serialPortHdlr, output, strlen(output));

    free(heap_p);

    return (int)retval;
}

static int cmd_walk(int argc, char* argv[])
{
    sm_send_event(EVENT_WALK);
}

static int cmd_stop(int argc, char* argv[])
{
    sm_send_event(EVENT_STOP);
}


/**
 * \brief Initialization of serial port
 */
void command_gpio_init()
{
//	static const gpio_map_t USART_GPIO_MAP =
//	{
//	  {COMMAND_USART_RX_PIN, COMMAND_USART_RX_FUNCTION},
//	  {COMMAND_USART_TX_PIN, COMMAND_USART_TX_FUNCTION}
//	};
//
//	// USART options.
//	static const usart_options_t USART_OPTIONS =
//	{
//	  .baudrate     = 9600, //57600,
//	  .charlength   = 8,
//	  .paritytype   = USART_NO_PARITY,
//	  .stopbits     = USART_1_STOPBIT,
//	  .channelmode  = USART_NORMAL_CHMODE
//	};

	// Assign GPIO to USART.
	//gpio_enable_module(USART_GPIO_MAP,
	//                   sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	
	// Initialize USART in RS232 mode.
	//usart_init_rs232(COMMAND_USART, &USART_OPTIONS, FOSC0);

	// Initialize serial driver
	serialPortHdlr = xUsartInit(serCOM1, COMMAND_UART_SPEED, 256, 256);
	
}

/*!
 * \fn command_task
 *atoi(argv[1]
 * The function of 'command' task which handles data incoming on serial port
 * and parses them into commands with arguments.
 *
 * \param p_parameters
 */
static portTASK_FUNCTION(command_task, p_parameters)
{
    static portCHAR c;
    static portCHAR* buffer_b = NULL; //! Beginning marker
    static portCHAR* buffer_e = NULL; //! End marker
    static portLONG  buffer_l = 0;    //! Buffer length

    static portCHAR* line_tokenized[24] = {NULL};
    static portSHORT token_number = 0;
    static portCHAR* reentrant_token = NULL;
    //label hash_error;

    //command_entry_t* cmd_ptr = (command_entry_t*)&(commands_table[0]); // Init by begining of the table
//    void* cmd_func_ptr;
//    cmd_function_t cmd_func = NULL;

    /* Process inital commands hashing */
//    commands_hashmap = hashmap_create(20);
//    while(cmd_ptr->command != NULL)
//    {
//        hashmap_insert(commands_hashmap, cmd_ptr->command, &(cmd_ptr->call), sizeof(cmd_function_t));
//        cmd_ptr++;
//    }

    /* Process initial commands hashing by uthash this time */
    command_entry_t* cmd_ptr = (command_entry_t*)&(commands_table[0]);
    while(cmd_ptr->command != NULL)
    {
        unsigned int cmd_len = strlen(cmd_ptr->command);
        HASH_ADD_KEYPTR(hh, commands_hashmap, cmd_ptr->command, cmd_len, cmd_ptr);
        cmd_ptr++;
    }


    /* Enter endless task loop */
    while(1)
    {
        //xUsartPutChar(serialPortHdlr, '*', 400);
        if(pdTRUE == xUsartGetChar(serialPortHdlr, &c, 400))
        {
#if COMMAND_ECHO_MODE >= 1
                xUsartPutChar(serialPortHdlr, c, 400);
#if COMMAND_ECHO_MODE >= 2
                xUsartPutChar(serialPortHdlr, '_', 400);
#endif /* COMMAND_ECHO_MODE >= 2 */
#endif /* COMMAND_ECHO_MODE >= 1 */
                if(('\n' != c) && ('\r' != c) && ('\b' != c)) /* TODO: not optimal conditions */
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
                else if ('\b' == c)
                {
                        if((NULL != buffer_b) && (buffer_e > buffer_b))
                        {
                            buffer_e--;
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
                                        unsigned int command_len = strlen(line_tokenized[0]);
                                        HASH_FIND(hh, commands_hashmap, line_tokenized[0], command_len, cmd_ptr);
                                        //HASH_FIND_STR(commands_hashmap, line_tokenized[0], cmd_ptr);
                                        //if(0 != hashmap_entry_by_key(commands_hashmap, line_tokenized[0], &cmd_func_ptr))
                                        if (NULL != cmd_ptr)
                                        {
                                                /* Have to cast - is it better way? */
                                                //cmd_func = *(cmd_function_t*)cmd_func_ptr;
                                                /* Call command function */
                                                //(*cmd_func)(token_number, line_tokenized);
                                                (*cmd_ptr->call)(token_number, line_tokenized);
#if COMMAND_DEBUG_MODE == 1
                                                static portCHAR output[200] = {'\0'};
                                                snprintf(output, 200, "Test %x, %x\r", (unsigned int)commands_table[0].call, (unsinged int)cmd_func);
                                                usUsartPutString(serialPortHdlr, output, strlen(output));
#endif /* COMMAND_DEBUG_MODE == 1 */
                                        }
                                        else
                                        {
                                            const char* error_string = "Unknown command.\n";
                                            usUsartPutString(serialPortHdlr, error_string, strlen(error_string));
                                            //printf("Unknown command.\n");
                                        }
                                        while(0)
                                        {
                                            const char* error_string = "UTHASH internal error.\n";
                                            hash_error:
                                            usUsartPutString(serialPortHdlr, error_string, strlen(error_string));
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
