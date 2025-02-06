#include <hello_task.h>
#include <stats_task.h>
#include <task.h>
#include <uart.h>

// "screen /dev/ttyUSB1 9600"

int main( void )
{
  TaskHandle_t hello_handle = NULL;
  TaskHandle_t stats_handle = NULL;

  // configure the uart for 9600/N/8/2
  uart_init(9600);
  
  /* Create the task without using any dynamic memory allocation. */
  hello_handle = xTaskCreateStatic(hello_task,"hello",HELLO_STACK_SIZE,
				   NULL,2,hello_stack,&hello_TCB);
  

  stats_handle = xTaskCreateStatic(stats_task,"stats",STATS_STACK_SIZE,
				   NULL,1,stats_stack,&stats_TCB);
			      
  /* start the scheduler */
  vTaskStartScheduler();

  /* we should never get to this point, but if we do, go into infinite
     loop */
  while(1);
}



/* Blatantly stolen from
   https://www.freertos.org/a00110.html#include_parameters
   and I really don't understand it yet.
*/

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application
must provide an implementation of vApplicationGetIdleTaskMemory() to
provide the memory that is used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside
this function then they must be declared static - otherwise they will
be allocated on the stack and so not exists after this function
exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the
    Idle task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/
