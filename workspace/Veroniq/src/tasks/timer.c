/*!
 * \file
 * \brief Implementation of high precision timer service.
 * \author Michał Fita <michal.fita@gmail.com>
 * \defgroup timer Timer Service
 * \ingroup veroniq
 * \{
 */

#include <stdint.h>
#include <sys/types.h>
#include <avr32/io.h>
#include <stdio.h>
#include <string.h>
#include <utlist.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "gpio.h"
#include "tc.h"
#include "timer.h"

#ifndef __GCC_POSIX__
#include "serial.h"
#endif /* __GCC_POSIX__ */

#undef TIMER_TEST_MODE

/*!
 * \brief Macro that calculates period in TC ticks from microseconds.
 * \warning Theory of numeric calculations is important here!
 */
#define CALCULATE_TC_PERIOD_FROM_US(period) (period < 900) \
    ? ((period * (configPBA_CLOCK_HZ / 8)) / (configTICK_RATE_HZ * 100) /10)  /* Good precision for small numbers */ \
    : ((configPBA_CLOCK_HZ / 8) / (configTICK_RATE_HZ * 100) * (period / 10)) /* Much less precision for great numbers */

/*!
 * \brief Channel of Timer Counter peripheral used in this module.
 * \note This is an ugly workaround for widely well known issue with constants in C.
 */
enum { TimerService_TC_Channel = 1 };//!< TimerService_TC_Channel

static enum {
    TS_Idle,
    TS_Awaiting,
    TS_Executing,
} ts_internal_state;

static enum {
	TS_Latency_Prev_Int,
	TS_Latency_Curr_Int,
	TS_Latency_Timer_Prep,
	TS_Latency_Spent_Int,
	TS_Latency_Schedule,
	TS_Latency_Execute,
	TS_Latency_Counters_Quantity /* Keep always last */
} __attribute__((__unused__)) ts_latency_timestamp_type;

/*!
 * \brief Internal list of scheduled events;
 */
typedef struct ts_sched_list_s {
    struct ts_sched_list_s*    next;             /*!< Pointer to the next element on the list */
    struct ts_sched_list_s*    prev;             /*!< Pointer to the previous element on the list */
    uint32_t                   id;               /*!< Identification of the scheduled element */
    ts_time_t                  departure_time;   /*!< When the request leave the originator */
    time_t                     execution_time;   /*!< When the element is scheduled (period) */
    ts_callback_t              callback;         /*!< Scheduled function to be called on time */
    ts_callback_data_t*        cdata_p;
} ts_sched_list_t;

/*!
 * \brief Interrupt number of Timer Counter peripheral used in this module.
 */
static const unsigned int TimerService_TC_Interrupt = AVR32_TC_IRQ1;

/*!
 * \brief Options for waveform generation.
 */
static const tc_waveform_opt_t ts_waveform_opt =
{
    .channel  = TimerService_TC_Channel,           /* Channel selection. */

    .bswtrg   = TC_EVT_EFFECT_NOOP,                /* Software trigger effect on TIOB. */
    .beevt    = TC_EVT_EFFECT_NOOP,                /* External event effect on TIOB. */
    .bcpc     = TC_EVT_EFFECT_NOOP,                /* RC compare effect on TIOB. */
    .bcpb     = TC_EVT_EFFECT_NOOP,                /* RB compare effect on TIOB. */

    .aswtrg   = TC_EVT_EFFECT_NOOP,                /* Software trigger effect on TIOA. */
    .aeevt    = TC_EVT_EFFECT_NOOP,                /* External event effect on TIOA. */
    .acpc     = TC_EVT_EFFECT_NOOP,                /* RC compare effect on TIOA: toggle. */
    .acpa     = TC_EVT_EFFECT_NOOP,                /* RA compare effect on TIOA: toggle (other possibilities are none, set and clear). */

    .wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,/* Waveform selection: Up mode without automatic trigger on RC compare. */
    .enetrg   = FALSE,                             /* External event trigger enable. */
    .eevt     = 0,                                 /* External event selection. */
    .eevtedg  = TC_SEL_NO_EDGE,                    /* External event edge selection. */
    .cpcdis   = TRUE,                              /* Counter disable when RC compare. */
    .cpcstop  = TRUE,                              /* Counter clock stopped with RC compare. */

    .burst    = TC_BURST_NOT_GATED,                /* Burst signal selection. */
    .clki     = TC_CLOCK_RISING_EDGE,              /* Clock inversion. */
    .tcclks   = TC_CLOCK_SOURCE_TC3                /* Internal source clock 3. */
};

static const tc_interrupt_t ts_tc_interrupt =
{
    .etrgs = 0,
    .ldrbs = 0,
    .ldras = 0,
    .cpcs  = 1,                                    /* Enable RC compare interrupt. */
    .cpbs  = 0,
    .cpas  = 0,
    .lovrs = 0,
    .covfs = 0
};


/*!
 * \brief Store values for latency counting
 */
static struct ts_latency_s
{
	long latency; /*!< Calculted latency */
	long last_period;
	long test_period;
	long compensation;
	ts_time_t timestamps[TS_Latency_Counters_Quantity]; /*! < Timestams used to calculate latency */
} ts_latency = { .latency = 0, .last_period = 0, .test_period = 500, .compensation = 0, .timestamps = { } };

static const int initial_sched_list_size = 20;
static ts_sched_list_t* sched_list = NULL;
static ts_sched_list_t* first_free = NULL;
static uint32_t current_id = 1; /**< ID to be used for new schedule. Intentionally not used 1 for first time */
static uint32_t last_sched_id = 1; /**< Last ID used for timer schedule. In fact this initialization does not matter */
static ts_callback_data_t latency_cb_data = { };

static xQueueHandle ts_queue = NULL;

/* Forward declarations */
static void __attribute__((__noinline__)) vTimerTickTest(void);
static inline void prepare_timer(time_t period, uint32_t id);
static inline bool_t check_reschedule(time_t period);
static bool_t ts_timer_reschedule(ts_time_t departure_time, long  period, uint32_t id);

#ifndef __GCC_POSIX__
/*! Handler for serial port used for command entry */
extern xComPortHandle serialPortHdlr;
#endif

/*!
 * \brief Get time defined as system ticks and processor cycle counts.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 19 Aug 2011
 * \details To check system tick uses xTaskGetTickCount() and to check
 *          processor high resolution tick uses COUNT register by
 *          calling Get_System_Register(AVR32_COUNT).
 */
ts_time_t ts_get_current_time()
{
    static ts_time_t current_time;
    current_time.system_ticks = xTaskGetTickCount();
    current_time.cpu_counter = Get_system_register(AVR32_COUNT);
    return current_time;
}

/*!
 * \brief Compare two times and return difference between them.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 24 Aug 2011
 * \note Later time have to be passed as second argument.
 * \param[in] first Earlier time to calculate difference
 * \param[in] second Later time to calculate difference
 * \return Delta between second and first
 */
ts_time_t ts_diff_time(ts_time_t first, ts_time_t second)
{
    ts_time_t diff;
    int32_t cpu_counter_diff;

    diff.system_ticks = second.system_ticks - first.system_ticks;
    cpu_counter_diff = (int32_t)second.cpu_counter - (int32_t)first.cpu_counter;
    if (cpu_counter_diff < 0)
    {
        diff.system_ticks--;
        cpu_counter_diff += (configCPU_CLOCK_HZ/configTICK_RATE_HZ);
    }
    diff.cpu_counter = cpu_counter_diff;
    return diff;
}

/*!
 * \brief Substract one time from another and return result.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 31 Aug 2011
 * \note Later time have to be passed as second argument.
 * \param[in] minuend Time from which it subtracts.
 * \param[in] subtrahend Time subtracted (should be < minuend).
 * \return Difference of subtrahend subtracted from minuend.
 */
ts_time_t inline ts_substract_time(ts_time_t minuend, ts_time_t subtrahend)
{
    ts_time_t diff;

    diff.system_ticks = minuend.system_ticks - subtrahend.system_ticks;
    diff.cpu_counter = minuend.cpu_counter - subtrahend.cpu_counter;

    return diff;
}

/*!
 * \brief Substract one time from period in us and return result in us.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 31 Aug 2011
 * \note If time expressed as \c subtrahend is higher than period, then
 *         returned value is 0 (zero).
 * \param[in] period Time in us from which it subtracts.
 * \param[in] subtrahend Time subtracted (should be < minuend).
 * \return Period in us reduced by subtrahend in us.
 */
long inline ts_substract_time_from_period(long period, ts_time_t subtrahend)
{
    const long cpu_ticks_in_us = (configCPU_CLOCK_HZ/configTICK_RATE_HZ) /* 1 ms */ / 1000; /* = t/(1 us) */
    const long us_in_system_ticks = 1000000 / configTICK_RATE_HZ; /* 10e6 us / system ticks in second */

    long us_subtrahend = (subtrahend.cpu_counter / cpu_ticks_in_us)
                       + (subtrahend.system_ticks * us_in_system_ticks);

    if (us_subtrahend > period)
    {
        return 0; /* Shall never be lower than 0 */
    }
    return period - us_subtrahend;
}

/*!
 * \brief Returns period remaining to execute from departure time of the request.
 * @param departure_time Time when request departed
 * @param period Time after which request has to be executed
 * @return Period remaining to execution
 */
static inline time_t ts_remaining_period(ts_time_t departure_time, long  period)
{
    ts_time_t schedule_time = ts_get_current_time();
    ts_time_t delta_time = ts_diff_time(departure_time, schedule_time);

    return ts_substract_time_from_period(period, delta_time);
}

/*!urrent_id
 * \brief Schedule some function to be executed after specific period.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 29 Aug 2011
 * \param[in] callback Function to be called after specified period.
 * \param[in] cdata_p  Pointer to the structure holding potential data for the callback.
 * \param[in] period   The period after which callback has to executed (10 us gradation; > 20 us)
 * \return             Handler of the schedule
 */
uint32_t ts_period_schedule(ts_callback_t callback, ts_callback_data_t* cdata_p, long period)
{
    timer_queue_msg_t schedule_message = { .type = TS_SCHEDULE };

    taskENTER_CRITICAL();

    schedule_message.departure_time = ts_get_current_time();

    if (NULL != callback)
    {
        schedule_message.request.callback = callback;
        schedule_message.request.execution_time = period;
        schedule_message.request.cdata_p = cdata_p;
        schedule_message.request.handle = current_id++;

        if (pdFALSE != xQueueSend(ts_queue, &schedule_message, 0))
        {
            return 0; /* OK if send successful */
        }
    }
    taskEXIT_CRITICAL();

    return schedule_message.request.handle;
}

/*!
 * \brief Checks if remaining period is shorter than current value of the comparator.
 * @param departure_time Time when request was departed to be executed.
 * @param period Period after which scheduled request has to be executed.
 * @param id Unique internal identifier of the request.
 * @return \c TRUE if reschedule take place, \c FALSE otherwise
 */
static bool_t ts_timer_reschedule(ts_time_t departure_time, long  period, uint32_t id)
{
    time_t remaining_period = ts_remaining_period(departure_time, period);

    if((ts_internal_state == TS_Idle) || (ts_internal_state == TS_Awaiting && check_reschedule(remaining_period)))
    {
        prepare_timer(remaining_period, id);
        return TRUE;
    }
    return FALSE;
}

/*!
 * \brief Schedules the execution of callback function internally.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 31 Aug 2011
 * \note I may silently ignore request for schedule if there is no room on
 *         schedule list for new entry.
 * \param callback Function to be called after given time.
 * \param cdata_p  Pointer to the structure holding potential data for the callback.
 * \param handle Identification number of scheduled task (given on scheduling in client).
 * \param departure_time Time when request was departed from client.
 * \param period Requested period after which the function has to be called.
 */
static void ts_internal_schedule(ts_callback_t callback,
                                 ts_callback_data_t* cdata_p,
                                 uint32_t handle,
                                 ts_time_t departure_time,
                                 long period)
{
    static ts_sched_list_t* sched_elem = NULL;

    if (TRUE == ts_timer_reschedule(departure_time, period, handle))
    {
        first_free = first_free->prev;
    }
    else
    {
        prepare_timer(period, handle);
    }

    CDL_FOREACH(first_free, sched_elem)
    {
        if (NULL != sched_elem->callback) continue;

        sched_elem->callback = callback;
        sched_elem->cdata_p = cdata_p;
        sched_elem->departure_time = departure_time;
        sched_elem->execution_time = ts_remaining_period(departure_time, period);
        sched_elem->id = handle;

        break; /* This would cause silent return without scheduling been done! */
    } //while  ((NULL != (sched_elem = sched_elem->next) && (first_free = sched_elem)) || !(first_free = sched_list));

    first_free = sched_elem->next;
}

/**
 * \brief This function executes function from the list of scheduled
 *        and prepares timer again if required for next element on the list
 * @param departure_time The time when interrupt sent the message
 * @param id Id of the element scheduled to be launched.
 */
static void ts_internal_execution(ts_time_t departure_time, uint32_t id)
{
    ts_sched_list_t* sched_elem = NULL;
    ts_sched_list_t* prev_sched_elem;
    //ts_time_t arrival_time = ts_get_current_time();

    sched_elem = sched_list;

    if (0 != id)
    {
        CDL_FOREACH(sched_list, sched_elem)
        {
           if ((NULL == sched_elem->callback) || id != sched_elem->id) continue;

           ts_time_t arrival_time = ts_get_current_time();

           //TODO: calculation of time spent on processing request from interrupt.

           sched_elem->callback(sched_elem->cdata_p);

           prev_sched_elem = sched_elem;
           sched_list = sched_elem->next; /* Next time execution will start search from next element */

           /* Clean up the element on the list */
           prev_sched_elem->callback = NULL;
           prev_sched_elem->cdata_p = NULL;
           prev_sched_elem->departure_time.system_ticks = 0;
           prev_sched_elem->departure_time.cpu_counter = 0;
           prev_sched_elem->execution_time = 0;

           break; /* This would cause silent return without scheduling been done! */
        }
    }

    ts_internal_state = TS_Idle; /* Intermediate state after possible execution */
    //sched_elem = prev_sched_elem;

    CDL_FOREACH(sched_list, sched_elem)
    {
        if (NULL != sched_elem->callback)
        {
           if (TRUE == ts_timer_reschedule(sched_elem->departure_time, sched_elem->execution_time, sched_elem->id))
           {
               break; /* If timer reschedule has taken place break further search */
           }
        }
    }

//    do {
//        if (NULL != sched_elem->next)
//        {
//            sched_elem = sched_elem->next;
//
//            if (sched_elem->callback != NULL)
//            {
//                ts_timer_reschedule(sched_elem->departure_time, sched_elem->execution_time, sched_elem->id);
//            }
//        }
//        else
//        {
//            /* Move one element from the front to the back of the list */
//            sched_elem->next = sched_list;
//            sched_list = sched_list->next;
//            sched_elem = sched_elem->next;
//            sched_elem->next = NULL;
//        }
//
//    } while  ((NULL != sched_elem->next));

    /* Silent leave */
}

/*!
 * \brief Interrupt handler for timer service.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 18 Aug 2011
 */
static void __attribute__((__noinline__)) vTimerTick_non_naked(void)
{
    volatile avr32_tc_t *tc = &AVR32_TC;
    portBASE_TYPE qstatus, task_woken;

    static timer_queue_msg_t interrupt_message = { .type = TS_EXECUTION };

    // Clear the interrupt flag. This is a side effect of reading the TC SR.
    tc_read_sr(tc, TimerService_TC_Channel);

    portENTER_CRITICAL();

    ts_latency.timestamps[TS_Latency_Prev_Int] = ts_latency.timestamps[TS_Latency_Curr_Int];
    ts_latency.timestamps[TS_Latency_Curr_Int] = ts_get_current_time();

    // Take ID of first callback awaiting execution
    interrupt_message.execution.id = last_sched_id;

    // Store departure time
    interrupt_message.departure_time = ts_get_current_time();

    // Do the job! Send the message to the queue.
    ts_internal_state = TS_Executing;
    qstatus = xQueueSendToFrontFromISR(ts_queue, &interrupt_message, &task_woken);
    portEXIT_CRITICAL();
}

/*!
 * \brief Interrupt handler for timer service.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 19 Aug 2011
 * \note In result of queue usage, the interrupt handler has to cope with possible
 *         context switch, which may occur after we fed the queue.
 */
#ifndef __GCC_POSIX__
static void __attribute__((__naked__)) vTimerTick(void)
{
    portENTER_SWITCHING_ISR();
    vTimerTick_non_naked();
    //vTimerTickTest();
    portEXIT_SWITCHING_ISR();
}
#else /* __GCC_POSIX__ */
static void __attribute__((__noinline__)) vTimerTick(void)
{
    //vTimerTick_non_naked();
    vTimerTickTest();
    portEND_SWITCHING_ISR(TRUE);
}
#endif /* __GCC_POSIX__ */

static void __attribute__((__noinline__)) vTimerTickTest(void)
//static void __attribute__((interrupt)) vTimerTickTest(void)
{
    static volatile avr32_tc_t *tc = &AVR32_TC;

    //portENTER_CRITICAL();

    ts_latency.timestamps[TS_Latency_Prev_Int] = ts_latency.timestamps[TS_Latency_Curr_Int];
    ts_latency.timestamps[TS_Latency_Curr_Int] = ts_get_current_time();

    tc_read_sr(tc, TimerService_TC_Channel);

    prepare_timer(ts_latency.test_period, 0);

    //gpio_enable_gpio_pin(AVR32_PIN_PB21);
    //gpio_tgl_gpio_pin(AVR32_PIN_PB21);

    //portEXIT_CRITICAL();
}

/*!
 * \brief Checks if time left to trigger an interrupt is longer than given
 *        periodts_internal_execution(time_t departure_time, uint32_t id)
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 31 Aug 2011
 * \param period Time after which the timer interrupt will be triggered
 * \return \c True if we should reschedule because we need shorter time or
 *         \c False otherwise
 * \todo Add support for latency
 */
static inline bool_t check_reschedule(time_t period)
{
    static volatile avr32_tc_t *tc = &AVR32_TC;

    // Calculate timer cycles for given period having rounding of integer math in mind
    time_t calculated_period = CALCULATE_TC_PERIOD_FROM_US(period);

    // Read the current timer counter and comparator
    int tc_ticks_left = tc_read_rc(tc, TimerService_TC_Channel) - tc_read_tc(tc, TimerService_TC_Channel);

    return ((tc_ticks_left <= 0) || (tc_ticks_left > calculated_period));
}

/*!
 * \brief Prepares timer to launch after specific period
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 19 Aug 2011
 * \param period Time after which the timer interrupt will be triggered
 */
static inline void prepare_timer(time_t period, uint32_t id)
{
    static volatile avr32_tc_t *tc = &AVR32_TC;

    //long calculated_period = (((period < 20 ? 20 : period / 10) * (configPBA_CLOCK_HZ / (configTICK_RATE_HZ * 100)))
    //                - (1 * (configPBA_CLOCK_HZ / (configTICK_RATE_HZ * 200)))) / 8;

    period = period < 20 ? 20 : period;

    // Calculate timer cycles for given period having rounding of integer math in mind
    time_t calculated_period = CALCULATE_TC_PERIOD_FROM_US(period);

    // =LICZBA.CAŁK(QUOTIENT(period*QUOTIENT(configPBA_CLOCK_HZ;8);(configTICK_RATE_HZ*100))/10)

    //ts_latency.last_period = calculated_period;

    ts_latency.timestamps[TS_Latency_Timer_Prep] = ts_get_current_time();
    ts_latency.compensation = ts_diff_time(ts_latency.timestamps[TS_Latency_Curr_Int], ts_latency.timestamps[TS_Latency_Timer_Prep]).cpu_counter
                     / ((configCPU_CLOCK_HZ / configPBA_CLOCK_HZ) * 8);

    // Internal timer & comparator is 16 bit only, so the trick is to schedule interrupt with no callback it should be
    // rescheduled then for further execution after one of next ticks.
    if ((calculated_period - ts_latency.compensation) > 0xFFF0)
    {
        calculated_period = 0xFFF0;
        id = 0; // Resetting id will cause reschedule
    }

    portENTER_CRITICAL();

    // Timer is counting for the given ID
    last_sched_id = id;

    // Program timer comparator to be launched after given period in tenth of us
    tc_write_rc(tc, TimerService_TC_Channel, calculated_period - ts_latency.compensation);

    // Start counting
    tc_start(tc, TimerService_TC_Channel);

    // Calculate real time spent on processing interrupt
    ts_latency.timestamps[TS_Latency_Spent_Int] = ts_diff_time(ts_latency.timestamps[TS_Latency_Prev_Int], ts_latency.timestamps[TS_Latency_Curr_Int]);

    // Mark internal state as awaiting for triggered interrupt
    ts_internal_state = TS_Awaiting;
    portEXIT_CRITICAL();
}

/*!
 * \brief Callback called first which allows measure of current latency.
 */
void ts_calculate_latency_cb(ts_callback_data_t* cdata_p)
{
	portENTER_CRITICAL();
	ts_latency.timestamps[TS_Latency_Execute] = ts_get_current_time();
	portEXIT_CRITICAL();

	ts_time_t processing_time = ts_diff_time(ts_latency.timestamps[TS_Latency_Schedule], ts_latency.timestamps[TS_Latency_Execute]);

	// calculate latency
#ifdef TIMER_TEST_MODE
	//xUsartPutChar(serialPortHdlr, '$', 0);

	ts_period_schedule(ts_calculate_latency_cb, latency_cb_data, ts_latency.test_period);

    gpio_enable_gpio_pin(AVR32_PIN_PB21);
    gpio_tgl_gpio_pin(AVR32_PIN_PB21);
#endif /* TIMER_TEST_MODE */
}

/*!
 * \fn timer_task
 * \brief Main function of timer service task.
 * \details The function of 'timer' task which handles requests about executing callbacks after given.
 *          It is designed to operate very precisely with higher resolution that system tick.
 * \param p_parameters Nothing useful in this case.
 */
static portTASK_FUNCTION(timer_task, p_parameters)
{
    static portCHAR output[200] = {'\0'};
    static ts_time_t period_to_print = {};

    static timer_queue_msg_t received_message = { };

    /* First go requires to launch latency calculation callback */
    ts_latency.timestamps[TS_Latency_Schedule] = ts_get_current_time();
    ts_period_schedule(ts_calculate_latency_cb, &latency_cb_data, 500);

    while(TRUE)
    {
        // Receive message
        // Select whether schedule interrupt or process callback as triggered by interrupt

        if (pdFALSE != xQueueReceive(ts_queue, &received_message, (portTickType)TASK_DELAY_S(2)))
        {
            switch (received_message.type)
            {
                case TS_SCHEDULE:
                    ts_internal_schedule(received_message.request.callback,
                            received_message.request.cdata_p,
                            received_message.request.handle,
                            received_message.departure_time,
                            received_message.request.execution_time);
                    break;
                case TS_EXECUTION:
                    ts_internal_execution(received_message.departure_time,
                                          received_message.execution.id);
                    break;
            }
        }

        portENTER_CRITICAL();
        period_to_print = ts_latency.timestamps[TS_Latency_Spent_Int];
        portEXIT_CRITICAL();

        //snprintf(output, 200, "Latency %ld, last period %ld; pp.ticks = %lu, pp.cpu = %hu; comp = %ld\r",
        //        ts_latency.latency, ts_latency.last_period,
        //        period_to_print.system_ticks, period_to_print.cpu_counter,
        //        ts_latency.compensation);
        //usUsartPutString(serialPortHdlr, output, strlen(output));

        //gpio_enable_gpio_pin(AVR32_PIN_PB21);
        //gpio_tgl_gpio_pin(AVR32_PIN_PB21);
        //vTaskDelay((portTickType)TASK_DELAY_S(2));
        //taskYIELD();
    }
}

/*!
 * \brief Set test period
 */

void ts_set_test_period(long test_period)
{
    portENTER_CRITICAL();
    ts_latency.test_period = test_period;
    portEXIT_CRITICAL();
}

/*!
 * \brief Initialization routine for whole timer service
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 18 Aug 2011
 */
static void init_timer_service()
{
    volatile avr32_tc_t *tc = &AVR32_TC;
    int list_idx;

    /* Init list initially */
    sched_list = malloc(initial_sched_list_size * sizeof(ts_sched_list_t));
    memset(sched_list, 0, initial_sched_list_size * sizeof(ts_sched_list_t));

    for (list_idx = 0; list_idx < initial_sched_list_size; ++list_idx)
    {
        CDL_PREPEND(first_free, (sched_list + list_idx));
    }

    taskENTER_CRITICAL();
    // Register interrupt
    INTC_register_interrupt(&vTimerTick/*Test*/, TimerService_TC_Interrupt, AVR32_INTC_INT0);


    // Initialize the timer/counter.
    tc_init_waveform(tc, &ts_waveform_opt);

    // Initial kick after 1 ms
    //tc_write_rc(tc, TimerService_TC_Channel, ( configPBA_CLOCK_HZ + 4 * configTICK_RATE_HZ ) /
    //                                                     ( 8 * configTICK_RATE_HZ ) );

    // Set the timer to be launched after 10 us
    //tc_write_rc(tc, TimerService_TC_Channel, (configPBA_CLOCK_HZ / (configTICK_RATE_HZ * 100))/ 8);

    // Set up the interrupt to be triggered by counter
    tc_configure_interrupts(tc, TimerService_TC_Channel, &ts_tc_interrupt );

    // Start the timer/counter for the first time
    //tc_start(tc, TimerService_TC_Channel);

    //prepare_timer(200, 0);

    taskEXIT_CRITICAL();

    //tc_init_capture();
}

/*!
 * \brief Routine starting high resolution timer service task.
 * \author Michal Fita <michal.fita@gmail.com>
 * \date 19 Aug 2011
 * \param priority The initial priority at which created task will run.
 */
void timer_start(unsigned portBASE_TYPE priority)
{
    // Create queue for the task
    ts_queue = xQueueCreate(10, sizeof(timer_queue_msg_t));

    // Initialize peripherals for high precision timer
    init_timer_service();

    // Spawn the timer task
    xTaskCreate(timer_task, (const signed portCHAR*)"TIMER",
                TIMER_STACK_SIZE, NULL, priority, (xTaskHandle*)NULL);
}

/**\}*/ /* %doxygen endgroup% */
