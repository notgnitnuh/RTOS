#ifndef NINVADERS_H
#define NINVADERS_H

#include <FreeRTOS.h>

void ninvaders(void *pvParameters);

/* Dimensions the buffer that the task being created will use as its
stack. NOTE: This is the number of words the stack will hold, not the
number of bytes. For example, if each stack item is 32-bits, and this
is set to 100, then 400 bytes (100 * 32-bits) will be allocated. */
#define NINVADERS_STACK_SIZE 1024

/* Structure that will hold the TCB of the task being created. */
extern StaticTask_t nInvaders_TCB;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
extern StackType_t nInvaders_stack[ NINVADERS_STACK_SIZE ];

#endif
