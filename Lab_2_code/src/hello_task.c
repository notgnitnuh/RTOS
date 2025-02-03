#include <hello_task.h>
#include <uart.h>

// "screen /dev/ttyUSB1 9600"

void hello_task(void *pvParameters)
{
  while(1)
    {
      uart_write_string("Hello World\n\r");
      vTaskDelay(pdMS_TO_TICKS( 1000 ));
    }
}

