#include <echo_task.h>
#include <task.h>
#include <UART_16550.h>
#include <stdio.h>

// "screen /dev/ttyUSB1 9600"

void echo_task(void *pvParameters)
{
  char buffer[64];
  char test;
  const TickType_t period = pdMS_TO_TICKS(100);
  
  TickType_t lastwake = xTaskGetTickCount();
  while(1)
    {
      // wait until the timeout
      //   vTaskDelayUntil(&lastwake,period);

      // UART_16550_get_char(UART0,buffer,portMAX_DELAY);
      // buffer[1] = '\0';
      UART_16550_read_string(UART0,buffer,64,portMAX_DELAY);
      UART_16550_write_string(UART0,buffer,portMAX_DELAY);
    }
}

/* Dimensions the buffer that the task being created will use as its
stack. NOTE: This is the number of words the stack will hold, not the
number of bytes. For example, if each stack item is 32-bits, and this
is set to 100, then 400 bytes (100 * 32-bits) will be allocated. */
#define STACK_SIZE 256

/* Structure that will hold the TCB of the task being created. */
StaticTask_t echo_TCB;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
StackType_t echo_stack[ ECHO_STACK_SIZE ];

