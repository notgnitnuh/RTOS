
#include <device_addrs.h>
// by including our header, we make sure that the header and the C
// file agree about the function definitions.
#include <uart.h>


void uart_init(int baud_rate)
{
  int divisor = UART_clkin / (baud_rate << 4);
  *UART0_LCR = 0x80;  // prepare to write baud rate
  *UART0_DLL = divisor & 0xFF;  // write low byte
  *UART0_DLH = divisor >> 8;    // write high byte
  // set baud rate, no parity, 8 data bits, 2 stop bits.
  // enable transmit holding register and receive buffer register.
  *UART0_LCR = 0x17; 
  // enable interrupts
  // UART_IER = 0x11; 
}

void uart_put_char(char c)
{
  char status;
  do
    status = *UART0_LSR;
  while (!(status & 0x20));
  *UART0_THR = c;
}

void uart_write_string(char *s)
{
  if(s != NULL)
    while (*s != 0)
      uart_put_char(*(s++));
}

char uart_get_char()
{
  int status;
  char c;
  do
    status = *UART0_LSR;
  while (!(status & 0x01));
  c = *UART0_RBR;
  uart_put_char(c);  // echo the character
  if( c == '\r')
    uart_put_char('\n');
  return c;
}

void uart_read_string(char *s, int maxLength)
{
  char c;
  int length=0;
  do
    {
      c = uart_get_char();
      *(s++) = (c == '\n') || (c == '\r') ? 0 : c;
      length++;
    }
  while (c != '\n' && c != '\r' && length < maxLength);
}
