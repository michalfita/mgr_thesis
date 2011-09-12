/*!
 * \file
 * \brief Implementation of motor driving operations through GPIO and PWM.
 * \author Michał Fita <michal.fita@gmail.com>
 */

#include <nlao_io.h>
#include <stdio.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pwm.h"
#include "gpio.h"
#include "utlist.h"
#include "timer.h"

#include "motor.h"

#define MOTOR_NO_PWM_MODE 0

/* Some globals */
xQueueHandle motor_queue;

/*! Pin mappings */
unsigned char const motor_pins_logic[] = {
        AVR32_PIN_PB29,
        AVR32_PIN_PB30,
        AVR32_PIN_PB31,
        AVR32_PIN_PB04,
        AVR32_PIN_PB17,
        AVR32_PIN_PA25,
        AVR32_PIN_PA26
    };
unsigned char const motor_pins_pwm[]   = {
		AVR32_PIN_PB19,
		AVR32_PIN_PB20,
		AVR32_PIN_PB21,
		AVR32_PIN_PB22,
		AVR32_PIN_PB27,
		AVR32_PIN_PB28,
		AVR32_PIN_PB18
    };
unsigned char const motor_pins_pwm_func[] = {
		AVR32_PWM_0_FUNCTION,
		AVR32_PWM_1_FUNCTION,
		AVR32_PWM_2_FUNCTION,
		AVR32_PWM_3_FUNCTION,
		AVR32_PWM_4_1_FUNCTION, // do not use UART pins
	    AVR32_PWM_5_1_FUNCTION, // do not use UART pins
	    AVR32_PWM_6_FUNCTION,
};

/*!
 * Structure holding data to be processed as deferred motor order.
 */
typedef struct motor_deffered_order_s {
    uint8_t  pin;
    uint8_t  value;
	uint32_t time;
} motor_deffered_order_t;

/*!
 * Stores current setting of pin values kept here instead of reading them from
 * Peripherals current setting. This approach is easier but requires update
 * always when pin value is updated.
 */
static unsigned int motor_pins_memory[14] = { 0 };

/*!
 * Function returns the structure of avr32_pwm_channel_t type containing
 * default configuration for PWM modified for duty cycle.
 * \param avr32_pwm_channel_t* Pointer to the PWM channel structure.
 * \param int New duty cycle for PWM on given channel.
 * \return Currently always zero.
 */
int get_pwm_structure(avr32_pwm_channel_t* p_pwm_channel, int duty_cycle)
{
	p_pwm_channel->CMR.calg = PWM_MODE_LEFT_ALIGNED;       // Channel mode.
	p_pwm_channel->CMR.cpol = PWM_POLARITY_HIGH;            // Channel polarity.
	p_pwm_channel->CMR.cpd = PWM_UPDATE_DUTY;              // Not used the first time.
	p_pwm_channel->CMR.cpre = AVR32_PWM_CPRE_MCK_DIV_256;  // Channel prescaler.
	p_pwm_channel->cdty = 1;  // Channel duty cycle, should be < CPRD.
	p_pwm_channel->cprd = 100;  // Channel period.
	p_pwm_channel->cupd = 0;   // Channel update is not used here.
	// With these settings, the output waveform period will be :
	// (66000000/512)/100 == 1.2890625 kHz [wrong]
	// (33000000/256)/100 == 1.2890625 kHz [6.05.2009]
	// (66000000/256)/100 == 2.578125 kHz  [10.09.2011]
	// (48000000/256)/100 == 1.875 kHz == (MCK/prescaler)/period,
	// with MCK == 48MHz, prescaler == 256, period == 100
	// (115200/256)/20 == 22.5Hz == (MCK/prescaler)/period, with MCK == 115200Hz,
	// prescaler == 256, period == 20.
	if (0 < duty_cycle)
	{
		p_pwm_channel->CMR.cpol = PWM_POLARITY_LOW;
		p_pwm_channel->cdty = duty_cycle;
		p_pwm_channel->cupd = duty_cycle;
	}
	else
	{
		p_pwm_channel->CMR.cpol = PWM_POLARITY_HIGH;
		p_pwm_channel->cdty = -duty_cycle;
		p_pwm_channel->cupd = -duty_cycle;
	}
	return 0;
}
/*!
 * This function was intended to implement robot moves.
 * \deprecated Idea was bad, used only for testing purposes. Left as reference.
 */
void motor_move_routine(/* subject to add direction */)
{
	static char status_move = 0;

    if (0 == status_move)
    {
        /* step forward */
        gpio_clr_gpio_pin(motor_pins_pwm[MOTOR_LEFT_LEG]);
        gpio_set_gpio_pin(motor_pins_logic[MOTOR_LEFT_LEG]);
        gpio_set_gpio_pin(motor_pins_pwm[MOTOR_RIGHT_LEG]);
        gpio_clr_gpio_pin(motor_pins_logic[MOTOR_RIGHT_LEG]);
        status_move = 1;
    }
    else
    {
        /* step backward */
        gpio_set_gpio_pin(motor_pins_pwm[MOTOR_LEFT_LEG]);
        gpio_clr_gpio_pin(motor_pins_logic[MOTOR_LEFT_LEG]);
        gpio_clr_gpio_pin(motor_pins_pwm[MOTOR_RIGHT_LEG]);
        gpio_set_gpio_pin(motor_pins_logic[MOTOR_RIGHT_LEG]);
        status_move = 0;
    }
}

/**
 * This function prepares and sends basic message to the motor task to set
 * proper value of the pin for particular time.
 *
 * @note To be used outside motor task.
 * @return Status of sending message.
 */
bool_t motor_send_message(uint8_t pin, uint8_t value, uint16_t time)
{
	motor_queue_msg_t 	msg;
	bool_t            	status = TRUE;

	msg.pin = pin;
	msg.value = value;
	msg.time = time;

	if (pdTRUE != xQueueSend(motor_queue, &msg, 200))
	{
		printf("Error: Failed to send message to motor task.\n");
		status = FALSE;
	}

	return status;
}

/*!
 * \brief Sets pin value to high or low or set PWM duty cycle.
 * \param pin_id
 * \param value
 * \return Status of execution.
 */
bool_t motor_setpin_values(uint8_t pin_id, uint8_t value)
{
	unsigned int pin_number;
	avr32_pwm_channel_t pwm_channel;
	bool_t status = TRUE;

	if (7 > pin_id) {
		pin_number = motor_pins_pwm[pin_id];
	} else if (14 > pin_id) {
		pin_number = motor_pins_logic[pin_id - 7];
	} else {
		value = 255; // Force switch below to ignore
	}
	switch(value) {
	case 1:
		gpio_enable_gpio_pin(pin_number);
		gpio_set_gpio_pin(pin_number);
		motor_pins_memory[pin_id] = 1;
		break;
	case 0:
		gpio_enable_gpio_pin(pin_number);
		gpio_clr_gpio_pin(pin_number);
		motor_pins_memory[pin_id] = 0;
		break;
	default:
		if ((100 <= value) && (value <= 200) && (7 > pin_id))
		{
			/* Handle PWM duty cycle values update */
			get_pwm_structure(&pwm_channel, value - 100);
			//pwm_channel_init(msg.pin/*motor_pins_pwm[msg.pin]*/, &pwm_channel);
			gpio_enable_module_pin(motor_pins_pwm[pin_id], motor_pins_pwm_func[pin_id]);
			/* Update channel. PWM channels are numbered, not pins ! */
			pwm_sync_update_channel(pin_id/*motor_pins_pwm[msg.pin]*/, &pwm_channel);
			//pwm_start_channels(1 << msg.pin);
			motor_pins_memory[pin_id] = value;
		}
		else
		{
			status = FALSE;
		}
		break;
	}
	return status;
}

void motor_defered_cb(ts_callback_data_t* cdata_p)
{
    if (NULL != cdata_p)
    {
        uint8_t pin = ((motor_deffered_order_t*)cdata_p->data_p)->pin;
        uint8_t value = ((motor_deffered_order_t*)cdata_p->data_p)->value;
        motor_setpin_values(pin, value);

        free(cdata_p);
    }
}

bool_t motor_setpin_defer(uint8_t pin_id, uint8_t value, uint16_t latency)
{
	// Add motor_setpin_values into the list of timing to process.
    ts_callback_data_t* cdata_p = malloc(sizeof(ts_callback_data_t) + sizeof(motor_deffered_order_t));
    cdata_p->data_size = sizeof(motor_deffered_order_t);
    cdata_p->data_p = (char*)cdata_p + sizeof(motor_deffered_order_t);

    ((motor_deffered_order_t*)cdata_p->data_p)->value = value;
    ((motor_deffered_order_t*)cdata_p->data_p)->pin = pin_id;

    ts_period_schedule(motor_defered_cb, cdata_p, latency*1000);

	return TRUE;
}

/**
 * This function process message waiting in motor_queue and sets pin value
 * according to value in the message - binary or from 100 to 200 reduced by
 * 100 as duty cycle for PWM.
 *
 * @todo Requires finishing timing support.
 *
 * @return Time for given state passed in message.
 */
unsigned int motor_process_queue()
{
	static motor_queue_msg_t msg;
	static unsigned int state_time;
	
	while (0 < uxQueueMessagesWaiting(motor_queue)) {
		if (pdTRUE == xQueueReceive(motor_queue, &msg, 200)) {
			state_time = msg.time;
			if (state_time > 0)
			{
				/* TODO: Put reset to previous on time */
				motor_setpin_defer(msg.pin,
						   motor_pins_memory[msg.pin],
						   state_time);
			}
			motor_setpin_values(msg.pin, msg.value);
		}
	}
	return state_time;
}

/*!
 * \fn motor_task
 * The task infinite function of motor module.
 */
static portTASK_FUNCTION(motor_task, p_parameters)
{
	portTickType xDelayLength = ((portTickType) 100 / portTICK_RATE_MS);
	portTickType xLastFocusTime;
	    
	avr32_pwm_channel_t pwm_channel;  //! One channel config.
	
	unsigned int new_duty_cycle = 1;
	unsigned int duty_cycle_step = 1;
	//unsigned int pin_logic = 0;
	   
    pwm_start_channels(0x0000007F); // start all seven PWM pins
    
    /* Initialize PWM structure */
    get_pwm_structure(&pwm_channel, 50);
    
    /* We need to initialize xLastFlashTime prior to the first call to vTaskDelayUntil(). */
    xLastFocusTime = xTaskGetTickCount();
    
    // Enter endless task loop
    while(1)
    {
    	motor_process_queue();
    	if (new_duty_cycle > 19) duty_cycle_step = -1;
    	else if (new_duty_cycle < 1)
    	{
    		duty_cycle_step = 1;
//    		motor_move_routine(); 		
    	}
    	
    	pwm_channel.cupd = new_duty_cycle;
    	
    	/* Delay for the flash period then check. */
        vTaskDelayUntil( &xLastFocusTime, xDelayLength );

#ifdef TO_BE_REMOVED /* TODO: Obvious */
#if MOTOR_NO_PWM_MODE == 0
	unsigned int channel_id;
	for(channel_id = 0; channel_id < 7; ++channel_id)
    	{3
    	    //pwm_sync_update_channel(channel_id, &pwm_channel); // Set channel configuration to all channels.
    	}
#else
            if (0 != pin_logic)
            {
                int pin_i;
                // Subject for optimization: massively clear GPIO pins
                for (pin_i = 0; pin_i < sizeof(motor_pins_logic); ++pin_i) {
                    //gpio_clr_gpio_pin(motor_pins_pwm[pin_i]);
                }
            } else {
                int pin_i;
                // Subject for optimization: massively set GPIO pins
                for (pin_i = 0; pin_i < sizeof(motor_pins_logic); ++pin_i) {
                    //gpio_set_gpio_pin(motor_pins_pwm[pin_i]);
                }
            }
#endif /* MOTOR_NO_PWM_MODE == 0 */
#endif /* TO_BE_REMOVED */

            new_duty_cycle += duty_cycle_step;
    }
}

/**
 * This function initializes GPIO module of CPU for PWM and binary output.
 */
void motor_gpio_init()
{
	int idx = 0;
    
    // Prepare GPIO pins for PWM output
    for(idx = 0; idx < 7; ++idx)
    {
#if MOTOR_NO_PWM_MODE == 0
    	gpio_enable_module_pin(motor_pins_pwm[idx], motor_pins_pwm_func[idx]);
#else /* MOTOR_NO_PWM_MODE != 0 */
    	gpio_enable_gpio_pin(motor_pins_pwm[idx]);
#endif
    	gpio_enable_pin_pull_up(motor_pins_pwm[idx]);
    	//gpio_set_gpio_pin(motor_pins_pwm[idx]);
    	gpio_clr_gpio_pin(motor_pins_pwm[idx]);
    	motor_pins_memory[idx] = 0 /* 1 */;
    }
    
    // Prepare GPIO pins for logic output
    for(idx = 0; idx < 7; ++idx)
    {
    	gpio_enable_gpio_pin(motor_pins_logic[idx]);
    	gpio_enable_pin_pull_up(motor_pins_logic[idx]);
    	//gpio_set_gpio_pin(motor_pins_logic[idx]);
    	gpio_clr_gpio_pin(motor_pins_logic[idx]);
    	motor_pins_memory[idx + 7] = 0 /* 1 */;
    }
}

/*!
 * PWM initialization routine.
 */
void motor_pwm_init()
{
    pwm_opt_t pwm_opt;                // PWM option config.
    avr32_pwm_channel_t pwm_channel;  // One channel config.
    // The channel number and instance is used in several functions.
    // It's defined as local variable for ease-of-use.
    unsigned int channel_id;

    // PWM controller configuration.
    pwm_opt.diva = AVR32_PWM_DIVA_CLK_OFF;
    pwm_opt.divb = AVR32_PWM_DIVB_CLK_OFF;
    pwm_opt.prea = AVR32_PWM_PREA_MCK;
    pwm_opt.preb = AVR32_PWM_PREB_MCK;

    pwm_init(&pwm_opt);

    get_pwm_structure(&pwm_channel, 50);

    for(channel_id = 0; channel_id < 7; ++channel_id)
    {
        pwm_channel_init(channel_id, &pwm_channel); // Set channel configuration to all channels.
    }
}

/*!
 * This function starts motor task.
 *
 * \note To be called only from init task.
 */
void motor_start(unsigned portBASE_TYPE priority)
{
	/* Initialize GPIO */
	motor_gpio_init();

#if MOTOR_NO_PWM_MODE == 0  
	/* Initialize PWM */
	motor_pwm_init();
#endif /* MOTOR_NO_PWM_MODE == 0 */
	
	/* Initialize queue for motor control. */
	motor_queue = xQueueCreate(10, sizeof(motor_queue_msg_t));
	
	/* Spawn the motor task. */
	xTaskCreate(motor_task, (const signed portCHAR*)"MOTOR",
                MOTOR_STACK_SIZE, NULL, priority, (xTaskHandle*)NULL);
}
