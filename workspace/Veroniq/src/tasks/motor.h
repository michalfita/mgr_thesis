#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

/*! The motor task stack size. */
#define MOTOR_STACK_SIZE	        ( configMINIMAL_STACK_SIZE + 128 )
/*! The motor task priority. */
#define MOTOR_TASK_PRIORITY 		( tskIDLE_PRIORITY + 15 )

/*! The motors for particular element of the robot (index in pin table). 
 *  Board with drivers has swapped outputs by design. */
#define MOTOR_LEFT_HAND             6
#define MOTOR_LEFT_ARM              5
#define MOTOR_LEFT_LEG              4
#define MOTOR_HIPS                  3
#define MOTOR_RIGHT_LEG             2
#define MOTOR_RIGHT_ARM             1
#define MOTOR_RIGHT_HAND			0

extern xQueueHandle motor_queue;

typedef struct {
	uint8_t pin;
	uint8_t value;
	uint16_t time;
} __attribute__((__packed__)) motor_queue_msg_t;

/* Declaration of the API */
void   motor_start(unsigned portBASE_TYPE priority);
bool_t motor_send_message(uint8_t pin, uint8_t value, uint16_t time);

#endif /*MOTOR_H_*/
