#include <pulse_modulator.h>
#include <device_addrs.h>
#include <task.h>
#include <stream_buffer.h>
#include <semphr.h>

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
  void (*handler)();      // A pointer to the channel's ISR handler
}pulse_modulator_descriptor_t;

static pulse_modulator_descriptor_t PM[] = {
  {PMaudio_ctrl, NULL, NULL},
  {PM1_ctrl, NULL, NULL},
  {PM2_ctrl, NULL, NULL},
  {PM3_ctrl, NULL, NULL},
  {PM4_ctrl, NULL, NULL},
};

static StaticSemaphore_t PM_mutex_buffer;
static SemaphoreHandle_t PM_mutex = NULL;

// Get exclusive access to the pulse modulator channel.
BaseType_t PM_acquire(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)

  // Create the mutex if it is null
  if(PM_mutex == NULL)
    PM_mutex = xSemaphoreCreateRecursiveMutexStatic(&PM_mutex_buffer);

  if(xSemaphoreTakeRecursive(PM_mutex, 0)){
    // See if PM is already owned
    if(PM[channel].owner == NULL){
      // If not owned, assign to current task
      PM[channel].owner = xTaskGetCurrentTaskHandle();
      PM[channel].dev->CSR.FIL = 1;
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

  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  
  if(xSemaphoreTakeRecursive(PM_mutex,0) ){
    PM_disable(channel);
    PM[channel].owner = NULL;
    xSemaphoreGiveRecursive(PM_mutex);
  }
  
}

// Look for pending interrupts and call their appropriate handler
void PM_handler(){
  BaseType_t hptw=0;

  for(int i=0 ; i<NUM_PM_CHANNELS; i++){
    if(PM[i].dev->CSR.IA == 1)
      PM[i].handler(hptw);
  }

  NVIC_ClearPendingIRQ(PM_IRQ);

  // return hptw;
}

// Set the interrupt handler function for the channel.
void PM_set_handler(int channel, void (*handler)(BaseType_t *)){

  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].handler = handler;
  
}

// Set the base frequency and number of divisions for the
// channel.
void PM_set_cycle_time(int channel, int divisions, int base_frequency){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  unsigned clkdiv = (PM_CLOCK/(divisions * base_frequency)) - 1;
  // ASSERT(clkdiv < (1<<16))

  PM[channel].dev->CDR = clkdiv;
  unsigned baseclk = PM_CLOCK / (clkdiv + 1);
  PM[channel].dev->BCR = baseclk / base_frequency - 1;
}

// Put the channel in PDM mode.
void PM_set_PDM_mode(int channel){

  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].dev->CSR.PDMM = 1;
}

// Put the channel in PWM mode.
void PM_set_PWM_mode(int channel){

  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].dev->CSR.PDMM = 0;
}

// Enable the FIFO. Return zero if the handler, cycle time, or
// mode have not been set.
int PM_enable_FIFO(int channel){

  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].dev->CSR.SLFM = 1;

  // TODO: how check?
  if(PM[channel].handler == NULL || PM[channel].dev->BCR == 0)// || PM[channel].dev->CSR.PDMM == undefined)
    return 0;

  return 1;
}

// Disable the FIFO.
void PM_disable_FIFO(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  PM[channel].dev->CSR.SLFM = 0;
}

// Enable output on the channel. Return zero if the channel is
// not fully configured.
int PM_enable(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())
  // TODO: check if fully configured
  if (PM[channel].dev->CSR.SLFM == 0){
    return 0;
  }
  PM[channel].dev->CSR.OE = 1;

  return 1;
}

// Disable output on the channel.
void PM_disable(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].dev->CSR.OE = 0;
}

// Enable interrupts on the channel.
int PM_enable_interrupt(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  NVIC_EnableIRQ(PM_IRQ);
  PM[channel].dev->CSR.IE = 1;
  return 0;
}

// Disable interrupts on the channel.
void PM_disable_interrupt(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
  ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].dev->CSR.IE = 0;
}

// Set the duty cycle for the channel. If the channel is in
// FIFO mode, this function writes to the FIFO. Otherwise,
// it writes directly to the Duty Cycle Register.
void PM_set_duty(int channel,int duty){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
// ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  PM[channel].dev->DCR = duty;
}

// If the channel is in FIFO mode, this function returns 1 if
// the FIFO is full. In all other cases, it returns zero.
int PM_FIFO_full(int channel){
  ASSERT(channel >= 0 && channel < NUM_PM_CHANNELS)
// ASSERT(PM[channel].owner == xTaskGetCurrentTaskHandle())

  if(PM[channel].dev->CSR.SLFM == 1 && PM[channel].dev->CSR.FF)
    return 1;

  return 0;
}