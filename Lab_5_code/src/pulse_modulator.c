#include <pulse_modulator.h>
#include <device_addrs.h>
#include <semphr.h>
#include <task.h>

typedef struct{
    volatile unsigned OE:1;
    volatile unsigned IO:1;
    volatile unsigned SLFM:1;
    volatile unsigned PDMM:1;
    volatile unsigned IE:1;
    volatile unsigned RF:1;
    volatile unsigned unused:2;
    volatile unsigned FF:1;
    volatile unsigned FE:1;
    volatile unsigned IA:1;
    volatile unsigned unused2:16;
    volatile unsigned FIL:5;
}CSR_t;

typedef volatile struct{
  volatile CSR_t CSR;      // Control and Status Register
  volatile uint32_t CDR;      // Clock Divisor Register
  volatile uint32_t BCR;      // Base Cycle Time Register
  volatile uint32_t DCR;      // Duty Cycle Register
}pulse_modulator_t;

typedef struct{
  pulse_modulator_t *dev; // Base address of the pulse modulator
  TaskHandle_t owner;     // The owner task handle
  void (*handler)();      // A pointrer to the channel's ISR handler
  
  // unsigned interrupt_number;    // NVIC IRQ number for this channel
  // SemaphoreHandle_t PM_mutex;   // Recursive mutex for this channel
}pulse_modulator_descriptor_t;

static pulse_modulator_descriptor_t PM[] = {
  {PMaudio_ctrl, NULL, NULL},
  {PM1_ctrl, NULL, NULL},
  {PM2_ctrl, NULL, NULL},
  {PM3_ctrl, NULL, NULL},
  {PM4_ctrl, NULL, NULL},
};

static SemaphoreHandle_t PM_mutex = xSemaphoreCreateRecursiveMutex();

// Get exclusive access to the pulse modulator channel.
BaseType_t PM_acquire(int channel){
  if(xSemaphoreTakeRecursive(PM_mutex)){
    // See if PM is already owned
    if(PM[channel].owner == NULL){
      // If not owned, assign to current task
      PM[channel].owner = xTaskGetCurrentTaskHandle();
      xSemaphoreGiveRecursive(PM_mutex);
      return pdPASS;
    }
    else{
      // If owned, catch in loop for debugging
      while(1);
    }
  }
  return pdFAIL;
}

// Release the channel. This function also disables the channel.
void PM_release(int channel){

  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  
  if(xSemaphoreTakeRecursive(PM_mutex) ){
    PM_disable(channel);
    PM[channel].owner = NULL;
    xSemaphoreGiveRecursive(PM_mutex);
  }
  
}

// Set the interrupt handler function for the channel.
void PM_set_handler(int channel, void (*handler)(void)){

  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].handler = handler;
  
}

// Set the base frequency and number of divisions for the
// channel.
void PM_set_cycle_time(int channel, int divisions, int base_frequency){

}

// Put the channel in PDM mode.
void PM_set_PDM_mode(int channel){

  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].CSR.PDMM = 1;
}

// Put the channel in PWM mode.
void PM_set_PWM_mode(int channel){

  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].CSR.PDMM = 0;
}

// Enable the FIFO. Return zero if the handler, cycle time, or
// mode have not been set.
int PM_enable_FIFO(int channel){

  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].CSR.SLFM = 1;

  // TODO: return 0 if handler, cycle time, or mode not set
  // if(PM[channel].handler == NULL || PM[channel].)

  return 1;
}

// Disable the FIFO.
void PM_disable_FIFO(int channel){
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].CSR.SLFM = 0;
}

// Enable output on the channel. Return zero if the channel is
// not fully configured.
int PM_enable(int channel){
  return 1;
}

// Disable output on the channel.
void PM_disable(int channel){

  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].CSR.OE = 0;
}

// Enable interrupts on the channel.
int PM_enable_interrupt(int channel){
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].CSR.IE = 1;

  return 0;
}

// Disable interrupts on the channel.
void PM_disable_interrupt(int channel){
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].CSR.IE = 0;

}

// Set the duty cycle for the channel. If the channel is in
// FIFO mode, this function writes to the FIFO. Otherwise,
// it writes directly to the Duty Cycle Register.
void PM_set_duty(int channel,int duty){

}

// If the channel is in FIFO mode, this function returns 1 if
// the FIFO is full. In all other cases, it returns zero.
int PM_FIFO_full(int channel){
  int result = 0;
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  if(PM[channel].CSR.SLRM == 1 && PM[channel].CSR.FF)
    result =  1;

  return result;
}