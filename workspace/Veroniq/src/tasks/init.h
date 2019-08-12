#ifndef INIT_H_
#define INIT_H_

/*! The init task stack size. */
#define INIT_STACK_SIZE	                 (configMINIMAL_STACK_SIZE + 128)
/*! The supervisor User Action task priority. */
#define INIT_TASK_PRIORITY   (tskIDLE_PRIORITY + 1)
/*! The init rate. Since the init task is in charge of updating
    the local time every second, this rate must be 1.*/
#define INIT_DEFAULT_PERIOD        ((portTickType) 100 / portTICK_RATE_MS)

void init_start(unsigned portBASE_TYPE priority);

#endif /*INIT_H_*/
