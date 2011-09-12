/*!
 * \file statemachine.c
 * \brief Implementation of state machine responsible for control of elementary moves of the robot
 *        which are tied from atomic operations on motor driving outputs.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 12 Aug 2011
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timer.h"
#include "motor.h"
#include "statemachine.h"

typedef enum sm_state_e
{
    STATE_IDLE,
    STATE_STEP_RIGHT,
    STATE_STEP_LEFT,
    STATE_STEP_STOP,
    NUM_OF_STATES
} sm_state_t;

typedef struct sm_queue_msg_s
{
    sm_event_t event;
} sm_queue_msg_t;

typedef void (*sm_hdlr_func_t)(sm_state_t*, sm_event_t);

typedef struct state_def_s
{
    sm_hdlr_func_t handler;
    time_t         holdoff;
    sm_state_t     state_switches[NUM_OF_EVENTS];
} state_def_t;



static void sm_step_right (sm_state_t *next_state, sm_event_t event);
static void sm_step_left  (sm_state_t *next_state, sm_event_t event);
static void sm_step_stop  (sm_state_t *next_state, sm_event_t event);

const state_def_t sm_state_table[NUM_OF_STATES] =
{
                                                  /* EVENT_NONE        EVENT_STOP       EVENT_WALK */
    {/* STATE_IDLE       */ NULL,               0, { STATE_IDLE,       STATE_IDLE,      STATE_STEP_RIGHT, },},
    {/* STATE_STEP_RIGHT */ sm_step_right, 800000, { STATE_STEP_LEFT,  STATE_STEP_STOP, STATE_STEP_LEFT,  },},
    {/* STATE_STEP_LEFT  */ sm_step_left,  800000, { STATE_STEP_RIGHT, STATE_STEP_STOP, STATE_STEP_RIGHT, },},
    {/* STATE_STEP_STOP  */ sm_step_stop,  300000, { STATE_IDLE,       STATE_IDLE,      STATE_IDLE,       },},
};

static xQueueHandle sm_queue;

static sm_event_t current_state = STATE_IDLE;

static void sm_timed_cb(ts_callback_data_t* cdata_p)
{
    if (NULL != cdata_p)
    {
        xQueueSend(sm_queue, cdata_p->data_p, 0);
        free(cdata_p);
    }
}

static void schedule_event(sm_event_t event, time_t period)
{
    ts_callback_data_t* cdata_p = NULL;

    cdata_p = malloc(sizeof(ts_callback_data_t) + sizeof(sm_queue_msg_t));
    cdata_p->data_size = sizeof(sm_queue_msg_t);
    cdata_p->data_p = (char*)cdata_p + sizeof(ts_callback_data_t);

    ((sm_queue_msg_t*)cdata_p->data_p)->event = event;

    ts_period_schedule(sm_timed_cb, cdata_p, period);
}

/*!
 * \brief This routine passes the event to be processed by state machine.
 * \param event An event to be process by state machine
 */
void sm_send_event(sm_event_t event)
{
    sm_queue_msg_t msg = { .event = event };

    xQueueSend(sm_queue, &msg, 0);
}

static void process_state(sm_event_t event)
{
    sm_state_t next_state = sm_state_table[current_state].state_switches[event];
    sm_hdlr_func_t handler_p = sm_state_table[next_state].handler;

    if (NULL != handler_p)
    {
        (*handler_p)(&next_state, event);
        schedule_event(EVENT_NONE, sm_state_table[next_state].holdoff);
    }
    current_state = next_state;
}

static void sm_step_right (sm_state_t* next_state, sm_event_t event)
{
    motor_send_message(MOTOR_HIPS_DIR, 1, 0);
    motor_send_message(MOTOR_HIPS    , 1, 500);

    motor_send_message(MOTOR_LEFT_LEG_DIR, 1, 0);
    motor_send_message(MOTOR_LEFT_LEG    , 1, 400);

    motor_send_message(MOTOR_RIGHT_LEG_DIR, 0, 0);
    motor_send_message(MOTOR_RIGHT_LEG    , 1, 400);
}

static void sm_step_left  (sm_state_t* next_state, sm_event_t event)
{

    motor_send_message(MOTOR_HIPS_DIR, 0, 0);
    motor_send_message(MOTOR_HIPS,     1, 500);

    motor_send_message(MOTOR_RIGHT_LEG_DIR, 1, 0);
    motor_send_message(MOTOR_RIGHT_LEG,     1, 400);

    motor_send_message(MOTOR_LEFT_LEG_DIR, 0, 0);
    motor_send_message(MOTOR_LEFT_LEG    , 1, 400);
}

static void sm_step_stop  (sm_state_t* next_state, sm_event_t event)
{
    motor_send_message(MOTOR_HIPS,      0, 0);
    motor_send_message(MOTOR_LEFT_LEG,  0, 0);
    motor_send_message(MOTOR_RIGHT_LEG, 0, 0);
}



/*!process_state(sm_event_t event)
 * \fn sm_task
 * \brief Main function of state machine task.
 * \details The main function of 'state machine' task which handles atomic states of mechanics
 *          to achieve moves of the robotics from atomic operations on motors.
 * \param p_parameters Nothing useful in this case.
 */
static portTASK_FUNCTION(sm_task, p_parameters)
{
    static sm_queue_msg_t received_message = { };

    while(TRUE)
    {
        // Receive message
        // Select whether schedule interrupt or process callback as triggered by interrupt

        if (pdFALSE != xQueueReceive(sm_queue, &received_message, (portTickType)TASK_DELAY_S(2)))
        {
            process_state(received_message.event);
        }
    }
}

/*!
 * \brief Routine starting state machine task.
 * \author Michal Fita <michal.fita@gmail.com>
 * \date 12 Sep 2011
 * \param priority The initial priority at which created task will run.
 */
void sm_start(unsigned portBASE_TYPE priority)
{
    // Create queue for the task
    sm_queue = xQueueCreate(10, sizeof(sm_queue_msg_t));

    // Spawn the timer task
    xTaskCreate(sm_task, (const signed portCHAR*)"SM",
            SM_STACK_SIZE, NULL, priority, (xTaskHandle*)NULL);
}
