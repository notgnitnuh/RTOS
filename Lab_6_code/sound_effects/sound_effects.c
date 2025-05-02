
#include <sounds.h>
#include <sound_effects.h>
#include <stddef.h>
#include <queue.h>
#include <pulse_modulator.h>

#define CHANNEL 0

EventGroupHandle_t effect_events;

// Each sound effect task will send audio buffers (actually just
// pointers) to the mixer using a dedicated queue
// static QueueHandle_t effect_to_mixer_queues[NUM_EFFECTS];

// The mixer will send buffer pointers to the ISR.
static QueueHandle_t MixerToISRqueue;
// The Interrupt Handler will return the buffer pointers to the mixer
// after transferring the data to the PM device.
static QueueHandle_t ISRToMixerqueue;

// Each instance of the effect_task will be given a unique sound to
// play, and a unique trigger event using the following structure. The
// pointer to an effect task's structure will be passed to it using
// the "params" mechanism in xTaskCreateStatic.
typedef struct{
  effect_buffer *buffers;
  int num_buffers;
  EventBits_t event;
  QueueHandle_t sendqueue;
}effect_param_t;

// The audio data and events that trigger them are known at compile
// time, but the queue handles must be filled in at run time by the
// effect_init() function.
static effect_param_t effect_task_params[NUM_EFFECTS] = {
  {explosion1,NUM_explosion1_BUFFERS,EXPLOSION1_EVENT,NULL},
  {fastinvader1,NUM_fastinvader1_BUFFERS,FASTINVADER1_EVENT,NULL},
  {fastinvader2,NUM_fastinvader2_BUFFERS,FASTINVADER2_EVENT,NULL},
  {fastinvader3,NUM_fastinvader3_BUFFERS,FASTINVADER3_EVENT,NULL},
  {fastinvader4,NUM_fastinvader4_BUFFERS,FASTINVADER4_EVENT,NULL},
  {invaderkilled,NUM_invaderkilled_BUFFERS,INVADERKILLED_EVENT,NULL},
  {shoot,NUM_shoot_BUFFERS,SHOOT_EVENT,NULL},
  {ufo_highpitch,NUM_ufo_highpitch_BUFFERS,UFO_HIGHPITCH_EVENT,NULL},
  {ufo_lowpitch,NUM_ufo_lowpitch_BUFFERS,UFO_LOWPITCH_EVENT,NULL}
};

// The interrupt handler for the audio pulse modulator
void audio_handler(BaseType_t *HPTW)
{
  static uint16_t *buffer = NULL; // make it static so that it always exists.
  static int buff_pos = 0;
  static BaseType_t validBuffer = pdFAIL;

  // may need more static variables here

  // if buffer == null, then get a buffer from the queue.
  if (buffer == NULL)
    validBuffer = xQueueReceiveFromISR(MixerToISRqueue, &buffer, HPTW);

  // While the PM FIFO is not full,
  //   Transfer a data item from the buffer to the FIFO
  while(PM_FIFO_full(CHANNEL) == 0){

    //   You may not have a valid buffer (the mixer may not
    //   have run yet) In that case, write a zero .
    if(validBuffer == pdPASS)
      PM_set_duty(CHANNEL,buffer[buff_pos++]);
    else
      PM_set_duty(CHANNEL, 0);
  
    //   If you send the last item in your current buffer, then send the
    //   pointer back to the mixer and get another buffer from the
    //   mixer.
    if(buff_pos >= EFFECT_BUFFER_SIZE){
      buff_pos = 0;
      xQueueSendFromISR(ISRToMixerqueue, &buffer, HPTW);
      buffer = NULL;
      break;
    }
  }
}


// define the final audio depth (after the mixer) and sample frequency
#define DEPTH 10
#define FREQ  8000

// Create the audio buffers for communication between the mixer and ISR
// Using 4 buffers is only a suggestion.
#define NUM_MIXER_BUFFERS 4
static uint16_t mixer_buffers[NUM_MIXER_BUFFERS][EFFECT_BUFFER_SIZE];

// The mixer task receives data from the individual effect tasks, and
// mixes the audio data before sending it to the ISR.
static void effect_mixer_task(void *params)
{
  // Initialization:
  uint16_t *buffer;
  int8_t *buffer2;
  // unsigned divisions = 2;
  // for(int i=1; i<DEPTH; i++)
  //   divisions = divisions * 2;


  // Put the pointers to the NUM_MIXER_BUFFERS mixer_buffers in the PM_to_mixer queue
  for(int i=0; i<NUM_MIXER_BUFFERS; i++){
    buffer = (uint16_t*)mixer_buffers[i];
    xQueueSend(ISRToMixerqueue, &buffer, portMAX_DELAY);
  }
  
  // configure and enable the pulse modulator
  PM_acquire(CHANNEL);
  PM_set_handler(CHANNEL, audio_handler);
  PM_set_cycle_time(CHANNEL,1024,FREQ);
  PM_set_duty(CHANNEL,0);
  PM_set_PDM_mode(CHANNEL);
  PM_enable_FIFO(CHANNEL);
  PM_enable(CHANNEL);
  PM_enable_interrupt(CHANNEL);

  while (1)
  {
    // Part 1:
    // Pick one of the sound effects.  For each chunk of data in the sound effect:
    //   Get a mixer buffer pointer from the ISR to mixer queue
    //   Copy (making adjustments) the data from the sound effect into it.
    //   Send the mixer buffer pointer to the mixer to ISR queue
    
    // if(xQueueReceive(ISRToMixerqueue, &buffer, portMAX_DELAY) == pdPASS){
    //   if(t1 > NUM_explosion1_BUFFERS){
    //     t1 = 0;
    //     vTaskDelay(pdMS_TO_TICKS( 500 ));
    //   }
    //   for(int i=0; i<EFFECT_BUFFER_SIZE; i++){
    //     buffer[i] = explosion1[t1].data[i];
    //   }
    //   xQueueSend(MixerToISRqueue, &buffer, portMAX_DELAY) != pdPASS;
    //   t1 += 1;
    // }

    // Part 2: (comment out part 1)
    //   Get a mixer buffer pointer from the ISR to mixer queue
    //   Get incoming data pointers from all of the sound effects queues.
    //   Add all of the incoming data streams and store the results in the mixer buffer. 
    //   Send the mixer buffer pointer to the mixer to ISR queue

    if(xQueueReceive(ISRToMixerqueue, &buffer, portMAX_DELAY) == pdPASS){
      // Clear the buffer
      for(int i=0; i<EFFECT_BUFFER_SIZE; i++){
        buffer[i] = 0;
      }

      // for each effect, check for available buffer and add them all together
      for(int i=0; i<NUM_EFFECTS; i++){

        if(xQueueReceive(effect_task_params[i].sendqueue, &buffer2, 0)){
          for(int j=0; j<EFFECT_BUFFER_SIZE; j++){
            buffer[j] += (uint16_t)(buffer2[j]);
          }
        }
      }
      for(int i=0; i<EFFECT_BUFFER_SIZE; i++){
        buffer[i] += 512;
      }

      // Send mixed buff
      xQueueSend(MixerToISRqueue, &buffer, portMAX_DELAY);
    }
  }
}


// Each sound effect is managed by an instance of this task.
static void effect_task(void *params)
{
  // typecast the params pointer so we can access our effect data
  effect_param_t *my_effect = (effect_param_t*)params;
  static int8_t* effect_buffer;

  static EventBits_t uxBits;

  while(1)
  {
    // Block until my event occurs.
    uxBits = xEventGroupWaitBits(effect_events, my_effect->event, pdTRUE, pdTRUE, portMAX_DELAY);

    // loop:
    //   send pointers to my buffers to my send queue
    //   until I have sent all of my buffers.
    if((uxBits & my_effect->event)){

      for(int i=0; i<my_effect->num_buffers; i++){
        effect_buffer = my_effect->buffers[i].data;
        xQueueSend(my_effect->sendqueue, &effect_buffer,portMAX_DELAY);
      }
    }
  }
}


// declare storage for the mixer task
#define MIXER_STACK_SIZE 128
static TaskHandle_t mixer_task_handle;
static StackType_t  mixer_stack[MIXER_STACK_SIZE];
static StaticTask_t mixer_TCB;

// declare storage for the effect tasks TODO: verify reasonability
#define EFFECT_STACK_SIZE 64
static TaskHandle_t effect_handles[NUM_EFFECTS];
static StackType_t effect_stacks[NUM_EFFECTS][EFFECT_STACK_SIZE];
static StaticTask_t effect_TCB[NUM_EFFECTS];
const char* effect_name[] = {"explosion1","fastinvader1","fastinvader2","fastinvader3","fastinvader4",
                            "invaderkilled","shoot","ufo_highpitch","ufo_lowpitch"};


// define storage for the ISR to Mixer and Mixer to ISR queues
static StaticQueue_t MixerToISRqueue_QCB, ISRToMixerqueue_QCB;
static uint16_t *MixerToISRqueue_buf[NUM_MIXER_BUFFERS];
static uint16_t *ISRToMixerqueue_buf[NUM_MIXER_BUFFERS];

// #define PART2_STACK_SIZE 256
// static TaskHandle_t part2_task_handle;
// static StackType_t  part2_stack[MIXER_STACK_SIZE];
// static StaticTask_t part2_TCB;

// void part2_task()
// {
//   while(1){
//     for(int i=0; i<NUM_EFFECTS; i++){
//       xEventGroupSetBits(effect_events, effect_task_params[i].event);
//       vTaskDelay(pdMS_TO_TICKS( 500 ));
//     }
//   }

// }

// sendqueu stuffs
static StaticQueue_t effectTaskqueue_QCB[NUM_EFFECTS];
static uint16_t *EffectTaskqueue_buf[NUM_EFFECTS][NUM_MIXER_BUFFERS];


void effect_init() // main should call this function to set up the sound effects
{
  int i;
  
  // create all of the queues that will be used by the effects tasks
  // to send data to the mixer. Store their handles in the
  // effect_to_mixer_queues array
  for(i=0; i<NUM_EFFECTS; i++){
    effect_task_params[i].sendqueue = xQueueCreateStatic(NUM_MIXER_BUFFERS, sizeof(int8_t*), (uint8_t*)EffectTaskqueue_buf[i], &effectTaskqueue_QCB[i]);
  }

  // Create event group
  effect_events = xEventGroupCreate();

  // create the two queues to communicate between the mixer and the ISR
  MixerToISRqueue = xQueueCreateStatic(NUM_MIXER_BUFFERS,sizeof(uint16_t*), (uint8_t*)MixerToISRqueue_buf, &MixerToISRqueue_QCB);
  ISRToMixerqueue = xQueueCreateStatic(NUM_MIXER_BUFFERS,sizeof(uint16_t*), (uint8_t*)ISRToMixerqueue_buf, &ISRToMixerqueue_QCB);
  
  // create all of the effect tasks, giving them each a unique queue handle and
  // other parameters (effect_params)
  for(i=0; i<NUM_EFFECTS; i++){  
    effect_handles[i] = xTaskCreateStatic(effect_task, effect_name[i], EFFECT_STACK_SIZE,
                        &effect_task_params[i], 6, effect_stacks[i], &effect_TCB[i]);
  }

  // // create the mixer task
  mixer_task_handle = xTaskCreateStatic(effect_mixer_task, "mixer task", MIXER_STACK_SIZE, 
    NULL,6,mixer_stack,&mixer_TCB);	

  // create the part 2 task
  // part2_task_handle = xTaskCreateStatic(part2_task, "part2 task", PART2_STACK_SIZE, 
  //   NULL,3,part2_stack,&part2_TCB);	
}