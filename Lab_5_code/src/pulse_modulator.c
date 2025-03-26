#include <pulse_modulator.h>
#include <device_addrs.h>
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
  pulse_modulator_t *dev;       // Base address of the pulse modulator
  unsigned interrupt_number;    // NVIC IRQ number for this channel
  SemaphoreHandle_t PM_mutex;   // Recursive mutex for this channel
}pulse_modulator_descriptor_t;

static pulse_modulator_descriptor_t PM[] = {
  {PMaudio_ctrl, PM_IRQ, NULL},
  {PM1_ctrl, PM_IRQ, NULL},
  {PM2_ctrl, PM_IRQ, NULL},
  {PM3_ctrl, PM_IRQ, NULL},
  {PM4_ctrl, PM_IRQ, NULL},
};


// Get exclusive access to the pulse modulator channel.
BaseType_t PM_acquire(int channel){
  return 1;
}

// Release the channel. This function also disables the channel.
void PM_release(int channel){

}

// Set the interrupt handler function for the channel.
void PM_set_handler(int channel, void (*handler)(void)){

}

// Set the base frequency and number of divisions for the
// channel.
void PM_set_cycle_time(int channel, int divisions, int base_frequency){

}

// Put the channel in PDM mode.
void PM_set_PDM_mode(int channel){

}

// Put the channel in PWM mode.
void PM_set_PWM_mode(int channel){

}


// Enable the FIFO. Return zero if the handler, cycle time, or
// mode have not been set.
int PM_enable_FIFO(int channel){
  return 1;
}

// Disable the FIFO.
void PM_disable_FIFO(int channel){

}

// Enable output on the channel. Return zero if the channel is
// not fully configured.
int PM_enable(int channel){
  return 1;
}

// Disable output on the channel.
void PM_disable(int channel){

}

// Enable interrupts on the channel.
int PM_enable_interrupt(int channel){
  return 1;
}

// Disable interrupts on the channel.
void PM_disable_interrupt(int channel){

}

// Set the duty cycle for the channel. If the channel is in
// FIFO mode, this function writes to the FIFO. Otherwise,
// it writes directly to the Duty Cycle Register.
void PM_set_duty(int channel,int duty){

}

// If the channel is in FIFO mode, this function returns 1 if
// the FIFO is full. In all other cases, it returns zero.
int PM_FIFO_full(int channel){
  return 1;
}