#include <avr32/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pwm.h"
#include "gpio.h"

#include "motor.h"

#define MOTOR_NO_PWM_MODE 0

/* Some globals */
xQueueHandle motor_queue;

/* Pin mappings */
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

/**
 * Function returns the structure of avr32_pwm_channel_t type containing
 * default configuration for PWM modified for duty cycle.
 */
int get_pwm_structure(avr32_pwm_channel_t* p_pwm_channel, int duty_cycle)
{
	p_pwm_channel->CMR.calg = PWM_MODE_LEFT_ALIGNED;       // Channel mode.
	p_pwm_channel->CMR.cpol = PWM_POLARITY_HIGH;            // Channel polarity.
	p_pwm_channel->CMR.cpd = PWM_UPDATE_DUTY;              // Not used the first time.
	p_pwm_channel->CMR.cpre = AVR32_PWM_CPRE_MCK_DIV_512;  // Channel prescaler.
	p_pwm_channel->cdty = 1;  // Channel duty cycle, should be < CPRD.
	p_pwm_channel->cprd = 100;  // Channel period.
	p_pwm_channel->cupd = 0;   // Channel update is not used here.
	// With these settings, the output waveform period will be :
	// (66000000/512)/100 == 1.289,0625 kHz [wrong]
	// (33000000/256)/100 == 1.289,0625 kHz [6.05.2009]
	// (66000000/256)/100 == 2.578,125 kHz
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

void motor_move_routine(/* subject to add direction */)
{
	static char status_move = 0;
	
	if (0 == status_move) {
		/* step forward */
		gpio_clr_gpio_pin(motor_pins_pwm[MOTOR_LEFT_LEG]);
		gpio_set_gpio_pin(motor_pins_logic[MOTOR_LEFT_LEG]);
		gpio_set_gpio_pin(motor_pins_pwm[MOTOR_RIGHT_LEG]);
		gpio_clr_gpio_pin(motor_pins_logic[MOTOR_RIGHT_LEG]);
		status_move = 1;
	} else {
		/* step backward */
		gpio_set_gpio_pin(motor_pins_pwm[MOTOR_LEFT_LEG]);
		gpio_clr_gpio_pin(motor_pins_logic[MOTOR_LEFT_LEG]);
		gpio_clr_gpio_pin(motor_pins_pwm[MOTOR_RIGHT_LEG]);
		gpio_set_gpio_pin(motor_pins_logic[MOTOR_RIGHT_LEG]);
		status_move = 0;
	}
}

unsigned int motor_process_queue()
{
	static motor_queue_msg_t msg;
	static unsigned int pin_number;
	static unsigned int state_time;
	avr32_pwm_channel_t pwm_channel;
	
	while (0 < uxQueueMessagesWaiting(motor_queue)) {
		if (pdTRUE == xQueueReceive(motor_queue, &msg, 200)) {
			if (7 > msg.pin) {
				pin_number = motor_pins_pwm[msg.pin];
			} else if (14 > msg.pin) {
				pin_number = motor_pins_logic[msg.pin - 7];
			} else {
				msg.value = 255; // Force switch below to ignore
			}
			state_time = msg.time;
			switch(msg.value) {
			case 1:
				gpio_enable_gpio_pin(pin_number);
				gpio_set_gpio_pin(pin_number);
				break;
			case 0:
				gpio_enable_gpio_pin(pin_number);
				gpio_clr_gpio_pin(pin_number);
				break;
			default:
				if ((100 <= msg.value) && (msg.value <= 200) && (7 > msg.pin))
				{
					/* Handle PWM duty cycle values update */
					get_pwm_structure(&pwm_channel, msg.value - 100);
					//pwm_channel_init(msg.pin/*motor_pins_pwm[msg.pin]*/, &pwm_channel);
					gpio_enable_module_pin(motor_pins_pwm[msg.pin], motor_pins_pwm_func[msg.pin]);
					pwm_sync_update_channel(msg.pin/*motor_pins_pwm[msg.pin]*/, &pwm_channel);
					//pwm_start_channels(1 << msg.pin);
				}
				break;
			}
		}
	}
	return state_time;
}

static portTASK_FUNCTION(motor_task, p_parameters)
{
	portTickType xDelayLength = ((portTickType) 100 / portTICK_RATE_MS);
	portTickType xLastFocusTime;
	    
	avr32_pwm_channel_t pwm_channel;  // One channel config.
	
	unsigned int new_duty_cycle = 1;
	unsigned int duty_cycle_step = 1;
	unsigned int pin_logic = 0;
	   
    pwm_start_channels(0x0000007F); // start all seven PWM pins
    
    /* Initialize PWM structure */
    get_pwm_structure(&pwm_channel, 50);
    
    /* We need to initialise xLastFlashTime prior to the first call to vTaskDelayUntil(). */
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
		
#if MOTOR_NO_PWM_MODE == 0
		unsigned int channel_id;
		for(channel_id = 0; channel_id < 7; ++channel_id)
    	{
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
		
		new_duty_cycle += duty_cycle_step;
    }
}

void motor_gpio_init()
{
	int idx = 0;
	
	// Prepare GPIO pins for PWM output
#if MOTOR_NO_PWM_MODE == 0
	//gpio_enable_module_pin(AVR32_PWM_0_PIN, AVR32_PWM_0_FUNCTION);
	//gpio_enable_module_pin(AVR32_PWM_1_PIN, AVR32_PWM_1_FUNCTION);
	//gpio_enable_module_pin(AVR32_PWM_2_PIN, AVR32_PWM_2_FUNCTION);
	//gpio_enable_module_pin(AVR32_PWM_3_PIN, AVR32_PWM_3_FUNCTION);
	//gpio_enable_module_pin(AVR32_PWM_4_1_PIN, AVR32_PWM_4_1_FUNCTION); // do not use UART pins
    //gpio_enable_module_pin(AVR32_PWM_5_1_PIN, AVR32_PWM_5_1_FUNCTION); // do not use UART pins
    //gpio_enable_module_pin(AVR32_PWM_6_PIN, AVR32_PWM_6_FUNCTION);
#endif /* MOTOR_NO_PWM_MODE == 0 */
    
    // Prepare GPIO pins for PWM output
    for(idx = 0; idx < 7; ++idx)
    {
#if MOTOR_NO_PWM_MODE == 0
    	gpio_enable_module_pin(motor_pins_pwm[idx], motor_pins_pwm_func[idx]);
#else /* MOTOR_NO_PWM_MODE != 0 */
    	gpio_enable_gpio_pin(motor_pins_pwm[idx]);
#endif
    	gpio_enable_pin_pull_up(motor_pins_pwm[idx]);
    	gpio_set_gpio_pin(motor_pins_pwm[idx]);
    }
    
    // Prepare GPIO pins for logic output
    for(idx = 0; idx < 7; ++idx)
    {
    	gpio_enable_gpio_pin(motor_pins_logic[idx]);
    	gpio_enable_pin_pull_up(motor_pins_logic[idx]);
    	gpio_set_gpio_pin(motor_pins_logic[idx]);
    }
}

void motor_pwm_init()
{
	pwm_opt_t pwm_opt;                // PWM option config.
	avr32_pwm_channel_t pwm_channel;  // One channel config.
	// The channel number and instance is used in several functions.
	// It's defined as local variable for ease-of-use.
	unsigned int channel_id;

	//channel_id = 0;

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

void motor_start(unsigned portBASE_TYPE priority)
{
	/* Init GPIO */
	motor_gpio_init();

#if MOTOR_NO_PWM_MODE == 0  
	/* Init PWM */
	motor_pwm_init();
#endif /* MOTOR_NO_PWM_MODE == 0 */
	
	/* Init queue for motor control. */
	motor_queue = xQueueCreate(2, sizeof(motor_queue_msg_t));
	
	/* Spawn the motor task. */
	xTaskCreate(motor_task, (const signed portCHAR*)"MOTOR",
                MOTOR_STACK_SIZE, NULL, priority, (xTaskHandle*)NULL);
}
