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

/*!
 * \brief Modes of state machine operation.
 */
typedef enum sm_mode_e
{
    MODE_FORWARD,                                                                      //!< MODE_FORWARD
    MODE_BACKWARD,                                                                     //!< MODE_BACKWARD
    NUM_OF_MODES,                                                                      //!< NUM_OF_MODES
    MODE_NO_CHANGE /**< Always last, not a mode, but indication mode will not change *///!< MODE_NO_CHANGE
} sm_mode_t;


typedef enum sm_event_e
{
    EVENT_TICK,
    EVENT_STOP,
    EVENT_WALK,
    NUM_OF_EVENTS
} sm_event_t;



extern void sm_go(sm_mode_t mode);
extern void sm_stop();
extern void sm_send_event(sm_event_t event);

#endif /* STATEMACHINE_H_ */
