/*!
 * \file timer.h
 * \brief Header file for timer service module.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 2011-08-18
 *
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/types.h>

/*! The command task stack size. */
#define TIMER_STACK_SIZE              ( configMINIMAL_STACK_SIZE + 2048 )
/*! The command task priority. */
#define TIMER_TASK_PRIORITY       ( tskIDLE_PRIORITY + 5 )

/*!
 * \brief Structure holding time in processor specific way - as cpu cycles & system ticks
 */
typedef struct ts_time_s {
    uint32_t system_ticks; //! stores system ticks as calculated by FreeRTOS
    uint16_t cpu_counter;  //! stores cpu counter from cpu register
} __attribute__((__packed__)) ts_time_t;

/*!
 * \brief Timer service callback type for easier use.
 */
typedef void (*ts_callback_t)(void);


/*!
 * \brief Identification of message type.
 */
typedef enum ts_message_type_e {
    TS_EXECUTION,
    TS_SCHEDULE,
} ts_message_type_t;

/*!
 * \brief Structure of the messages traveling inside this module.
 */
typedef struct timer_queue_msg_s {
        ts_message_type_t type;
        ts_time_t departure_time;
        union {
            struct {
                time_t execution_time;   //! time to execute callback as period from moment of request
                ts_callback_t callback;  //! callback to execute
            } request;
            struct {
                uint32_t id;       //! id of request to execute
            } execution;
        };
} __attribute__((__packed__)) timer_queue_msg_t;


extern ts_time_t  ts_get_current_time();
extern ts_time_t  ts_diff_time(ts_time_t first, ts_time_t second);
extern void       ts_set_test_period(long test_period);
extern void       timer_start(unsigned portBASE_TYPE priority);

#endif /* TIMER_H_ */
