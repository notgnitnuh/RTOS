
#include <PM_test_task.h>
#include <task.h>
#include <UART_16550.h>
#include <pulse_modulator.h>
#include <stdio.h>

// "screen /dev/ttyUSB1 9600"

void PM_test_task(void *pvParameters){
  long long int val = 0;
  unsigned basefrequency = 30000;
  unsigned divisions = 1000;
  unsigned dutyCycle = 75;

  TickType_t lastwake = xTaskGetTickCount();
  

  PM_acquire(0);
  PM_set_handler(0, PM_test_task_handler);
  PM_enable_FIFO(0);
  PM_set_cycle_time(0,divisions,basefrequency);
  PM_set_duty(0,divisions/100*dutyCycle);
  PM_set_PWM_mode(0);
  PM_enable_interrupt(0);
  PM_enable(0);

  PM_acquire(1);
  PM_set_handler(1, PM_test_task_handler);
  PM_enable_FIFO(1);
  PM_set_cycle_time(1,divisions,basefrequency);
  PM_set_duty(1,divisions/100*dutyCycle);
  PM_set_PWM_mode(1);
  PM_enable_interrupt(1);
  PM_enable(1);

  PM_acquire(2);
  PM_set_handler(2, PM_test_task_handler);
  PM_enable_FIFO(2);
  PM_set_cycle_time(2,divisions,basefrequency);
  PM_set_duty(2,divisions/100*dutyCycle);
  PM_set_PDM_mode(2);
  PM_enable_interrupt(2);
  PM_enable(2);

  unsigned clkdiv = (PM_CLOCK/(divisions * basefrequency)) - 1;
  unsigned baseclk = PM_CLOCK / (clkdiv + 1);
  unsigned BCR = baseclk / basefrequency - 1;

  char buffer[64];
  sprintf(buffer, "divisions: %d, base freq %d, CDR: %d, BCR: %d\r\n", divisions, basefrequency, clkdiv, BCR);
  UART_16550_write_string(UART0,buffer,portMAX_DELAY);
  while(1){
    vTaskDelayUntil(&lastwake,1000);
    sprintf(buffer, " %d\r", val);
    UART_16550_write_string(UART0,buffer,portMAX_DELAY);
    val += 1;
  }

}

void PM_test_task_handler(void *pvParameters){

}

/* Dimensions the buffer that the task being created will use as its
stack. NOTE: This is the number of words the stack will hold, not the
number of bytes. For example, if each stack item is 32-bits, and this
is set to 100, then 400 bytes (100 * 32-bits) will be allocated. */
#define PM_TEST_STACK_SIZE 256

/* Structure that will hold the TCB of the task being created. */
StaticTask_t PM_test_TCB;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
StackType_t PM_test_stack[ PM_TEST_STACK_SIZE ];



