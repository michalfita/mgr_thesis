/*!
 * \file
 * \brief Implementation of high precision timer service.
 * \author Michał Fita <michal.fita@gmail.com>
 */

#include <stdint.h>
#include <sys/types.h>
#include <avr32/io.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "serial.h"
#include "gpio.h"
#include "tc.h"
#include "timer.h"

/*!
 * \brief Channel of Timer Counter peripheral used in this module.
 * \notice This is an ugly workaround for widely well known issue with constants in C.
 */
enum { TimerService_TC_Channel = 1 };

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
    .cpcstop  = FALSE,  //TRUE,                    /* Counter clock stopped with RC compare. */

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

/*!ticks
 * Store values for latency counting
 */
static long ts_latency = 0;
static long ts_last_period = 0;
static long ts_test_period = 50;
static long ts_compensation = 0;
static ts_time_t ts_period_between[4] = { };

static xQueueHandle ts_queue = NULL;

static void __attribute__((__noinline__)) vTimerTickTest(void);
static inline void prepare_timer(long period, bool_t interrupt);

/* Handler for serial port used for command entry */
extern xComPortHandle serialPortHdlr;

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
 * \notice Later time have to be passed as second argument.
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
 * \brief Schedule some function to be executed after specific period.
 * \param[in] callback Function to be called after specified period.
 * \param[in] period   The period after which callback has to executed (10 us gradation; > 20 us)
 * \return             \c TRUE on success, \c FALSE on failure
 */
bool_t ts_period_schedule(ts_callback_t callback, long period)
{
    timer_queue_msg_t schedule_message = { };

    schedule_message.departure_time = ts_get_current_time();

    if (NULL != callback)
    {
        schedule_message.request.callback = callback;
        schedule_message.request.execution_time = period;

        if (pdFALSE != xQueueSend(ts_queue, &schedule_message, 0))
        {
            return TRUE; /* OK if send successful */
        }
    }

    return FALSE;
}

/*!
 * \brief Schedules the execution of callback function internally
 */
static void ts_internal_schedule(ts_callback_t callback, ts_time_t departure_time, long period)
{
    static ts_time_t schedule_time = { };
    static ts_time_t delta_time = { };

    schedule_time = ts_get_current_time();
    delta_time = ts_diff_time(departure_time, schedule_time);


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

    static timer_queue_msg_t interrupt_message = { };

    // Clear the interrupt flag. This is a side effect of reading the TC SR.
    tc_read_sr(tc, TimerService_TC_Channel);

    // Store departure time
    interrupt_message.departure_time = ts_get_current_time();

    // Take ID of first callback awaiting execution
    interrupt_message.execution.id = 0;

    // Do the job! Send the message to the queue.
    portENTER_CRITICAL();
    qstatus = xQueueSendToFrontFromISR(ts_queue, &interrupt_message, &task_woken);
    portEXIT_CRITICAL();
}

/*!
 * \brief Interrupt handler for timer service.
 * \author Michał Fita <michal.fita@gmail.com>
 * \date 19 Aug 2011
 * \notice In result of queue usage, the interrupt handler has to cope with possible
 *         context switch, which may occur after we fed the queue.
 */
static void __attribute__((__naked__)) vTimerTick(void)
{
    portENTER_SWITCHING_ISR();
    //vTimerTick_non_naked();
    vTimerTickTest();
    portEXIT_SWITCHING_ISR();
}

static void __attribute__((__noinline__)) vTimerTickTest(void)
//static void __attribute__((interrupt)) vTimerTickTest(void)
{
    volatile avr32_tc_t *tc = &AVR32_TC;

    //portENTER_CRITICAL();

    ts_period_between[0] = ts_period_between[1];
    ts_period_between[1] = ts_get_current_time();

    tc_read_sr(tc, TimerService_TC_Channel);

    prepare_timer(ts_test_period, TRUE);

    gpio_enable_gpio_pin(AVR32_PIN_PB21);
    gpio_tgl_gpio_pin(AVR32_PIN_PB21);

    //portEXIT_CRITICAL();
}

/*!
 * \brief Prepares timer to launch after specific period
 * \author Michał Fita <amf018@gmail.com>
 * \date 19 Aug 2011
 * \param period Time after which the timer interrupt will be triggered
 */
static inline void prepare_timer(long period, bool_t interrupt)
{
    volatile avr32_tc_t *tc = &AVR32_TC;

    //long calculated_period = (((period < 20 ? 20 : period / 10) * (configPBA_CLOCK_HZ / (configTICK_RATE_HZ * 100)))
    //                - (1 * (configPBA_CLOCK_HZ / (configTICK_RATE_HZ * 200)))) / 8;

    period = period < 20 ? 20 : period;

    // Calculate timer cycles for given period having rounding of integer math in mind
    long calculated_period = (period * (configPBA_CLOCK_HZ / 8)) / (configTICK_RATE_HZ * 100) /10;

    // =LICZBA.CAŁK(QUOTIENT(period*QUOTIENT(configPBA_CLOCK_HZ;8);(configTICK_RATE_HZ*100))/10)

    ts_last_period = calculated_period;

    ts_period_between[2] = ts_get_current_time();
    ts_compensation = ts_diff_time(ts_period_between[1], ts_period_between[2]).cpu_counter
                     / ((configCPU_CLOCK_HZ / configPBA_CLOCK_HZ) * 8);

    portENTER_CRITICAL();

    // Program timer comparator to be launched after given period in tenth of us
    tc_write_rc(tc, TimerService_TC_Channel, calculated_period - ts_compensation);

    // Start counting
    if (FALSE /*interrupt == TRUE*/)
    {
        tc_software_trigger(tc, TimerService_TC_Channel);
    }
    else
    {
        tc_start(tc, TimerService_TC_Channel);
    }


    // Calculate real time spent on processing interrupt
    ts_period_between[3] = ts_diff_time(ts_period_between[0], ts_period_between[1]);

    portEXIT_CRITICAL();
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

    while(1)
    {
        // Receive message
        // Select whether schedule interrupt or process callback as triggered by interrupt

        if (pdFALSE != xQueueReceive(ts_queue, &received_message, (portTickType)TASK_DELAY_S(2)))
        {
            switch (received_message.type)
            {
                case TS_SCHEDULE:
                    break;
                case TS_EXECUTION:
                    break;
            }
        }

        portENTER_CRITICAL();
        period_to_print = ts_period_between[3];
        portEXIT_CRITICAL();

        snprintf(output, 200, "Latency %ld, last period %ld; pp.ticks = %ld, pp.cpu = %d; comp = %ld\r",
                ts_latency, ts_last_period, period_to_print.system_ticks, period_to_print.cpu_counter, ts_compensation);
        usUsartPutString(serialPortHdlr, output, strlen(output));
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
    ts_test_period = test_period;
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

    portDISABLE_INTERRUPTS();
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

    prepare_timer(200, FALSE);

    portENABLE_INTERRUPTS();

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
