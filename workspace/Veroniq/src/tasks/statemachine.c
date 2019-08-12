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
#include "serial.h"

typedef enum sm_state_e
{
    STATE_IDLE,
    STATE_START,      /**< Initial reset */
    STATE_STEP_RIGHT, /**< Right foot forward */
    STATE_STEP_LEFT,  /**< Left foot forward */
    STATE_HIP_RIGHT,  /**< Hip on right */
    STATE_HIP_LEFT,   /**< Hip on left */
    STATE_STEP_STOP,  /**< Freeze */
    NUM_OF_STATES
} sm_state_t;

typedef struct sm_queue_msg_s
{
    sm_event_t event;
    sm_mode_t mode;
} sm_queue_msg_t;

typedef void (*sm_hdlr_func_t)(sm_state_t*, sm_event_t);

typedef struct state_def_s
{
    sm_hdlr_func_t handler;
    time_t         holdoff;
    sm_state_t     state_switches[NUM_OF_EVENTS];
} state_def_t;

typedef state_def_t state_def_table_t[NUM_OF_STATES];

static void sm_step_start (sm_state_t *next_state, sm_event_t event);
static void sm_step_right (sm_state_t *next_state, sm_event_t event);
static void sm_step_left  (sm_state_t *next_state, sm_event_t event);
static void sm_hip_right  (sm_state_t *next_state, sm_event_t event);
static void sm_hip_left   (sm_state_t *next_state, sm_event_t event);
static void sm_step_stop  (sm_state_t *next_state, sm_event_t event);

const state_def_table_t sm_fwd_state_table =
{
                                                  /* EVENT_TICK        EVENT_STOP       EVENT_WALK */
    {/* STATE_IDLE       */ NULL,               0, { STATE_IDLE,       STATE_IDLE,      STATE_START,  },},
    {/* STATE_START      */ sm_step_start, 400000, { STATE_HIP_LEFT,   STATE_IDLE,      STATE_START,  },},
    {/* STATE_STEP_RIGHT */ sm_step_right, 600000, { STATE_HIP_RIGHT,  STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_STEP_LEFT  */ sm_step_left,  600000, { STATE_HIP_LEFT,   STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_HIP_RIGHT  */ sm_hip_right,  300000, { STATE_STEP_LEFT,  STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_HIP_LEFT   */ sm_hip_left,   300000, { STATE_STEP_RIGHT, STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_STEP_STOP  */ sm_step_stop,  600000, { STATE_IDLE,       STATE_IDLE,      STATE_START,  },},
};

const state_def_table_t sm_bck_state_table =
{
                                                  /* EVENT_TICK        EVENT_STOP       EVENT_WALK */
    {/* STATE_IDLE       */ NULL,               0, { STATE_IDLE,       STATE_IDLE,      STATE_START,  },},
    {/* STATE_START      */ sm_step_start, 400000, { STATE_HIP_LEFT,   STATE_IDLE,      STATE_START,  },},
    {/* STATE_STEP_RIGHT */ sm_step_right, 600000, { STATE_HIP_LEFT,   STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_STEP_LEFT  */ sm_step_left,  600000, { STATE_HIP_RIGHT,  STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_HIP_RIGHT  */ sm_hip_right,  300000, { STATE_STEP_RIGHT, STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_HIP_LEFT   */ sm_hip_left,   300000, { STATE_STEP_LEFT,  STATE_STEP_STOP, STATE_START,  },},
    {/* STATE_STEP_STOP  */ sm_step_stop,  600000, { STATE_IDLE,       STATE_IDLE,      STATE_START,  },},
};

const state_def_table_t* sm_mode_state_table[NUM_OF_MODES] =
{
    &sm_fwd_state_table,
    &sm_bck_state_table,
};

static xQueueHandle sm_queue;

static sm_event_t current_state = STATE_IDLE;
static sm_mode_t  current_mode = MODE_FORWARD;

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
    ((sm_queue_msg_t*)cdata_p->data_p)->mode = MODE_NO_CHANGE;

    ts_period_schedule(sm_timed_cb, cdata_p, period);
}

/*!
 * \brief This routine starts walking in forward or backward backward direction appropriately.
 * \param mode A mode of SM operation, it means direction.
 */
void sm_go(sm_mode_t mode)
{
    sm_queue_msg_t msg = { .event = EVENT_WALK, .mode = mode };

    xQueueSend(sm_queue, &msg, 0);
}

/*!
 * \brief This routing stops walking.
 */
void sm_stop()
{
    sm_queue_msg_t msg = { .event = EVENT_STOP, .mode = MODE_NO_CHANGE };

    xQueueSend(sm_queue, &msg, 0);
}

/*!
 * \brief This routine passes the event to be processed by state machine.
 * \param event An event to be process by state machine.
 * \deprecated Use sm_go() and sm_stop() instead.
 */
void sm_send_event(sm_event_t event)
{
    sm_queue_msg_t msg = { .event = event, .mode = MODE_NO_CHANGE };

    xQueueSend(sm_queue, &msg, 0);
}

static void process_state(sm_event_t event)
{
    const state_def_table_t* current_mode_state_table = sm_mode_state_table[current_mode];
    sm_state_t next_state = (*current_mode_state_table)[current_state].state_switches[event];
    sm_hdlr_func_t handler_p = (*current_mode_state_table)[next_state].handler;

    if (NULL != handler_p)
    {
        (*handler_p)(&next_state, event);
        schedule_event(EVENT_TICK, (*current_mode_state_table)[next_state].holdoff);
    }
    current_state = next_state;
}

static void sm_step_start(sm_state_t *next_state, sm_event_t event)
{
    motor_send_message(MOTOR_HIPS,          0, 0);
    motor_send_message(MOTOR_LEFT_LEG,      0, 0);
    motor_send_message(MOTOR_RIGHT_LEG,     0, 0);
}

static void sm_step_right (sm_state_t* next_state, sm_event_t event)
{
    motor_send_message(MOTOR_LEFT_LEG_DIR, 0, 0); /* Left leg 0 = Backward */
    motor_send_message(MOTOR_LEFT_LEG    , 1, 0);

    motor_send_message(MOTOR_RIGHT_LEG_DIR, 0, 0); /* Right leg 0 = Forward */
    motor_send_message(MOTOR_RIGHT_LEG    , 1, 0);

    motor_send_message(MOTOR_HIPS,          130, 0/*500*/);
}

static void sm_step_left  (sm_state_t* next_state, sm_event_t event)
{
    motor_send_message(MOTOR_RIGHT_LEG_DIR, 1, 0); /* Right leg 1 = Backward */
    motor_send_message(MOTOR_RIGHT_LEG,     1, 0);

    motor_send_message(MOTOR_LEFT_LEG_DIR,  1, 0); /* Left leg 1 = Forward */
    motor_send_message(MOTOR_LEFT_LEG    ,  1, 0);

    motor_send_message(MOTOR_HIPS,          130, 0/*500*/);
}

static void sm_hip_right  (sm_state_t *next_state, sm_event_t event)
{
    motor_send_message(MOTOR_HIPS_DIR,      0, 0); /* Hip right = right */
    motor_send_message(MOTOR_HIPS,          1, 0/*500*/);
}

static void sm_hip_left   (sm_state_t *next_state, sm_event_t event)
{
    motor_send_message(MOTOR_HIPS_DIR,      1, 0); /* Hip 1 = Left */
    motor_send_message(MOTOR_HIPS    ,      1, 0/*500*/);
}

static void sm_step_stop  (sm_state_t* next_state, sm_event_t event)
{
    motor_send_message(MOTOR_HIPS,          0, 0);
    motor_send_message(MOTOR_LEFT_LEG,      0, 0);
    motor_send_message(MOTOR_RIGHT_LEG,     0, 0);
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
            if ((received_message.mode >= 0) && (received_message.mode < NUM_OF_MODES))
            {
                current_mode = received_message.mode;
            }
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
