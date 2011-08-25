/*!
 * \file
 * \brief Module initializing all other tasks
 * \author Michał Fita <michal.fita@gmail.com>
 * \todo Aggregate some system statistics
 */

#include "FreeRTOS.h"
#include "task.h"

#include "init.h"
#include "motor.h"
#include "command.h"
#include "timer.h"

static portTASK_FUNCTION(init_task, p_parameters)
{
    portTickType xDelayLength = INIT_DEFAULT_PERIOD;
    portTickType xLastFocusTime;

    // Launch tasks

    motor_start(MOTOR_TASK_PRIORITY);
    command_start(COMMAND_TASK_PRIORITY);
    timer_start(TIMER_TASK_PRIORITY);
	   
    /* We need to initialize xLastFlashTime prior to the first call to vTaskDelayUntil(). */
    xLastFocusTime = xTaskGetTickCount();
	
    // Enter endless task loop
    while(1)
    {
            /* Delay for the flash period then check. */
            vTaskDelayUntil( &xLastFocusTime, xDelayLength );

    }
}

void init_start(unsigned portBASE_TYPE priority)
{
   /* Spawn the initial task. */
   xTaskCreate(init_task, (const signed portCHAR*)"INIT",
                INIT_STACK_SIZE, NULL, priority, (xTaskHandle*)NULL);
}
