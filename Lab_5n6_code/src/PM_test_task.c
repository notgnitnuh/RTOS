
#include <PM_test_task.h>
#include <task.h>
#include <UART_16550.h>
#include <pulse_modulator.h>
#include <stdio.h>

// "screen /dev/ttyUSB1 9600"
// https://deepbluembedded.com/sine-lookup-table-generator-calculator/
const uint16_t sineLookupTable[] = {
  0x800, 0x880, 0x900, 0x97f, 0x9fd, 0xa78, 0xaf1, 0xb67, 0xbda, 0xc49, 0xcb3, 0xd19, 0xd79, 0xdd4, 0xe29,
  0xe78, 0xec0, 0xf02, 0xf3c, 0xf6f, 0xf9b, 0xfbf, 0xfdb, 0xfef, 0xffb, 0xfff, 0xffb, 0xfef, 0xfdb, 0xfbf,
  0xf9b, 0xf6f, 0xf3c, 0xf02, 0xec0, 0xe78, 0xe29, 0xdd4, 0xd79, 0xd19, 0xcb3, 0xc49, 0xbda, 0xb67, 0xaf1,
  0xa78, 0x9fd, 0x97f, 0x900, 0x880, 0x800, 0x77f, 0x6ff, 0x680, 0x602, 0x587, 0x50e, 0x498, 0x425, 0x3b6,
  0x34c, 0x2e6, 0x286, 0x22b, 0x1d6, 0x187, 0x13f, 0xfd, 0xc3, 0x90, 0x64, 0x40, 0x24, 0x10, 0x04,
  0x00, 0x04, 0x10, 0x24, 0x40, 0x64, 0x90, 0xc3, 0xfd, 0x13f, 0x187, 0x1d6, 0x22b, 0x286, 0x2e6,
  0x34c, 0x3b6, 0x425, 0x498, 0x50e, 0x587, 0x602, 0x680, 0x6ff, 0x77f};

void PM_test_task(void *pvParameters){
  long long int val = 0;
  unsigned basefrequency = 440;
  unsigned divisions = 4095;
  unsigned dutyCycle = 50;

  TickType_t lastwake = xTaskGetTickCount();

  // Set up channel 0 (Audio Jack)
  PM_acquire(0);
  PM_set_handler(0, PM_test_task_handler);
  PM_set_cycle_time(0,divisions,basefrequency);
  PM_set_duty(0,divisions/100*dutyCycle);
  PM_set_PDM_mode(0);
  PM_enable_FIFO(0);
  PM_enable_interrupt(0);
  PM_enable(0);

  unsigned clkdiv = (PM_CLOCK/(divisions * basefrequency)) - 1;
  unsigned baseclk = PM_CLOCK / (clkdiv + 1);
  unsigned BCR = baseclk / basefrequency - 1;

  char buffer[64];
  sprintf(buffer, "divisions: %d, base freq: %d, CDR: %d, BCR: %d\r\n", divisions, basefrequency, clkdiv, BCR);
  UART_16550_write_string(UART0,buffer,portMAX_DELAY);
  while(1){
    // vTaskDelayUntil(&lastwake,1000 / 440);

    if(!PM_FIFO_full(0)){
      sprintf(buffer, " %d\r", val);
      UART_16550_write_string(UART0,buffer,portMAX_DELAY);
      val += 1;
      if (val > 99)
        val = 0;
      PM_set_duty(0,sineLookupTable[val]); 
    }

    // PM_set_duty(1,sineLookupTable[val]); 
    // PM_set_duty(2,sineLookupTable[val]);
    // PM_set_duty(2,val); 
  }
}

void PM_test_task_handler(void){
  static unsigned int val = 0;

  // val += 1; 
  // PM_set_duty(0,sineLookupTable[val]); 
  // if (val > 99) 
  //   val = 0;

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



