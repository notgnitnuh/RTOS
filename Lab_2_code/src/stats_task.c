#include <stats_task.h>
#include <AXI_timer.h>

// "screen /dev/ttyUSB1 9600"
void stats_task(void *pvParameters);

/* Structure that will hold the TCB of the task being created. */
StaticTask_t stats_TCB;

static int timer_number = 0;
static int timer_count = 0;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
StackType_t stats_stack[ STATS_STACK_SIZE ];

void stats_task(void *pvParameters)
{
  static char stats_report_buffer[1024];

  while(1)
    {
      // do task thing.
      vTaskGetRunTimeStats(stats_report_buffer);
      uart_write_string(stats_report_buffer);
      vTaskDelay(pdMS_TO_TICKS( 5000 ));
    }
}

int get_stats_counter()
{
  return timer_count;
}

void timer_interrupt_handler() {
  timer_count ++;
}

void setup_stats_timer(void)
{
  timer_number = AXI_TIMER_allocate();
  AXI_TIMER_set_handler(timer_number, timer_interrupt_handler);
  AXI_TIMER_set_repeating(timer_number, AXI_TIMER_HZ_TO_COUNT(10000));
  AXI_TIMER_enable_interrupt(timer_number);
  // AXI_TIMER_enable(timer_number);
  // also enable interrupt in the NVIC using CMSIS

}