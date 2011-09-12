/*!
 * \file statemachine.c
 * \brief Implementation of state machine responsible for control of elementary moves of the robot
 *        which are tied from atomic operations on motor driving outputs.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 12 Aug 2011
 */

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

/*! The command task stack size. */
#define SM_STACK_SIZE          ( configMINIMAL_STACK_SIZE + 1024 )
/*! The command task priority. */
#define SM_TASK_PRIORITY       ( tskIDLE_PRIORITY + 45 )

typedef enum sm_event_e
{
    EVENT_NONE,
    EVENT_STOP,
    EVENT_WALK,
    NUM_OF_EVENTS
} sm_event_t;

extern void sm_send_event(sm_event_t event);

#endif /* STATEMACHINE_H_ */
