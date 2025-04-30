
#include <sounds.h>
#include <sound_effects.h>
#include <stddef.h>
#include <queue.h>
#include <pulse_modulator.h>

#define CHANNEL 0

EventGroupHandle_t effect_events;

// Each sound effect task will send audio buffers (actually just
// pointers) to the mixer using a dedicated queue
static QueueHandle_t effect_to_mixer_queues[NUM_EFFECTS];

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
const uint16_t sineLookupTable2[] = {
  0x200, 0x20a, 0x213, 0x21d, 0x227, 0x230, 0x23a, 0x243, 0x24d, 0x257, 0x260, 0x26a, 0x273, 0x27c, 0x286, 0x28f, 0x298, 0x2a1, 0x2ab, 0x2b4, 0x2bd, 0x2c6, 0x2ce, 0x2d7, 0x2e0, 0x2e9, 0x2f1, 0x2fa, 0x302, 0x30a,
  0x313, 0x31b, 0x323, 0x32b, 0x332, 0x33a, 0x342, 0x349, 0x350, 0x358, 0x35f, 0x366, 0x36d, 0x373, 0x37a, 0x380, 0x387, 0x38d, 0x393, 0x399, 0x39f, 0x3a4, 0x3aa, 0x3af, 0x3b4, 0x3b9, 0x3be, 0x3c2, 0x3c7, 0x3cb,
  0x3d0, 0x3d4, 0x3d7, 0x3db, 0x3df, 0x3e2, 0x3e5, 0x3e8, 0x3eb, 0x3ee, 0x3f0, 0x3f2, 0x3f5, 0x3f6, 0x3f8, 0x3fa, 0x3fb, 0x3fc, 0x3fd, 0x3fe, 0x3ff, 0x400, 0x400, 0x400, 0x400, 0x400, 0x3ff, 0x3ff, 0x3fe, 0x3fd,
  0x3fc, 0x3fb, 0x3f9, 0x3f7, 0x3f6, 0x3f3, 0x3f1, 0x3ef, 0x3ec, 0x3ea, 0x3e7, 0x3e4, 0x3e0, 0x3dd, 0x3d9, 0x3d5, 0x3d2, 0x3cd, 0x3c9, 0x3c5, 0x3c0, 0x3bb, 0x3b6, 0x3b1, 0x3ac, 0x3a7, 0x3a1, 0x39c, 0x396, 0x390,
  0x38a, 0x384, 0x37d, 0x377, 0x370, 0x369, 0x362, 0x35b, 0x354, 0x34d, 0x345, 0x33e, 0x336, 0x32f, 0x327, 0x31f, 0x317, 0x30e, 0x306, 0x2fe, 0x2f5, 0x2ed, 0x2e4, 0x2dc, 0x2d3, 0x2ca, 0x2c1, 0x2b8, 0x2af, 0x2a6,
  0x29d, 0x294, 0x28a, 0x281, 0x278, 0x26e, 0x265, 0x25b, 0x252, 0x248, 0x23f, 0x235, 0x22b, 0x222, 0x218, 0x20e, 0x205, 0x1fb, 0x1f2, 0x1e8, 0x1de, 0x1d5, 0x1cb, 0x1c1, 0x1b8, 0x1ae, 0x1a5, 0x19b, 0x192, 0x188,
  0x17f, 0x176, 0x16c, 0x163, 0x15a, 0x151, 0x148, 0x13f, 0x136, 0x12d, 0x124, 0x11c, 0x113, 0x10b, 0x102, 0xfa, 0xf2, 0xe9, 0xe1, 0xd9, 0xd1, 0xca, 0xc2, 0xbb, 0xb3, 0xac, 0xa5, 0x9e, 0x97, 0x90,
  0x89, 0x83, 0x7c, 0x76, 0x70, 0x6a, 0x64, 0x5f, 0x59, 0x54, 0x4f, 0x4a, 0x45, 0x40, 0x3b, 0x37, 0x33, 0x2e, 0x2b, 0x27, 0x23, 0x20, 0x1c, 0x19, 0x16, 0x14, 0x11, 0x0f, 0x0d, 0x0a,
  0x09, 0x07, 0x05, 0x04, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x0a, 0x0b, 0x0e, 0x10, 0x12, 0x15, 0x18, 0x1b, 0x1e, 0x21,
  0x25, 0x29, 0x2c, 0x30, 0x35, 0x39, 0x3e, 0x42, 0x47, 0x4c, 0x51, 0x56, 0x5c, 0x61, 0x67, 0x6d, 0x73, 0x79, 0x80, 0x86, 0x8d, 0x93, 0x9a, 0xa1, 0xa8, 0xb0, 0xb7, 0xbe, 0xc6, 0xce,
  0xd5, 0xdd, 0xe5, 0xed, 0xf6, 0xfe, 0x106, 0x10f, 0x117, 0x120, 0x129, 0x132, 0x13a, 0x143, 0x14c, 0x155, 0x15f, 0x168, 0x171, 0x17a, 0x184, 0x18d, 0x196, 0x1a0, 0x1a9, 0x1b3, 0x1bd, 0x1c6, 0x1d0, 0x1d9,
  0x1e3, 0x1ed, 0x1f6};
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
  uint16_t *buffer;
  uint16_t *buffer2;
  unsigned divisions = 2;
  for(int i=1; i<DEPTH; i++)
    divisions = divisions * 2;

  // Initialization:
  static int t1, t2= 0;
  static int buffPos[NUM_EFFECTS];

  // Put the pointers to the NUM_MIXER_BUFFERS mixer_buffers in the PM_to_mixer queue
  for(int i=0; i<NUM_MIXER_BUFFERS; i++){
    buffer = (uint16_t*)mixer_buffers[i];
    xQueueSend(ISRToMixerqueue, &buffer, portMAX_DELAY);
  }
  
  // configure and enable the pulse modulator
  PM_acquire(CHANNEL);
  PM_set_handler(CHANNEL, audio_handler);
  PM_set_cycle_time(CHANNEL,divisions,FREQ);
  PM_set_duty(CHANNEL,0);
  PM_set_PWM_mode(CHANNEL);
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
    
    if(xQueueReceive(ISRToMixerqueue, &buffer, portMAX_DELAY) == pdPASS){
      if(t1 > NUM_explosion1_BUFFERS){
        t1 = 0;
        vTaskDelay(pdMS_TO_TICKS( 500 ));
      }
      for(int i=0; i<EFFECT_BUFFER_SIZE; i++){
        buffer[i] = explosion1[t1].data[i];
      }
      xQueueSend(MixerToISRqueue, &buffer, portMAX_DELAY) != pdPASS;
      t1 += 1;
    }

    // Part 2: (comment out part 1)
    //   Get a mixer buffer pointer from the ISR to mixer queue
    //   Get incoming data pointers from all of the sound effects queues.
    //   Add all of the incoming data streams and store the results in the mixer buffer. 
    //   Send the mixer buffer pointer to the mixer to ISR queue

    // if(xQueueReceive(ISRToMixerqueue, &buffer, portMAX_DELAY) == pdPASS){
    //   for(int i=0; i<NUM_EFFECTS; i++){
    //     if(xQueueReceive(effect_to_mixer_queues[i], &buffer2, portMAX_DELAY == pdPASS)){
    //       if(buffPos[i] >= effect_task_params[i].num_buffers){
    //         buffPos[i] = 0;
    //       }
    //       for(int j=0; j<EFFECT_BUFFER_SIZE; j++){
    //         buffer[j] = effect_task_params[i].buffers[buffPos[i]].data[j];
    //       }
    //       if(xQueueSend(MixerToISRqueue, &buffer, portMAX_DELAY) != pdPASS)
    //         while(1);
    //       buffPos[i] += 1;
    //     }
    //   }
    // }
  }
}


// Each sound effect is managed by an instance of this task.
static void effect_task(void *params)
{
  // typecast the params pointer so we can access our effect data
  effect_param_t *my_effect = (effect_param_t*)params;

  static EventBits_t uxBits;

  while(1)
  {
    // // Block until my event occurs.
    // uxBits = xEventGroupWaitBits(effect_events, my_effect->event, pdTRUE, pdFALSE, 100);
    // // loop:
    // //   send pointers to my buffers to my send queue
    // //   until I have sent all of my buffers.
    // if(uxBits & my_effect->event == my_effect->event){
    for(int i=0; i<my_effect->num_buffers; i++){
      if(uxQueueSpacesAvailable(my_effect->sendqueue) > 0)
        xQueueSend(my_effect->sendqueue, &my_effect->buffers[i],portMAX_DELAY);
    }
    vTaskDelay(pdMS_TO_TICKS( 2000 ));
    // }
  }
}



// declare storage for the mixer task
#define MIXER_STACK_SIZE 512
static TaskHandle_t mixer_task_handle;
static StackType_t  mixer_stack[MIXER_STACK_SIZE];
static StaticTask_t mixer_TCB;

// declare storage for the effect tasks TODO: verify reasonability
#define EFFECT_STACK_SIZE 512
static TaskHandle_t effect_handles[NUM_EFFECTS];
static StackType_t effect_stacks[NUM_EFFECTS][EFFECT_STACK_SIZE];
static StaticTask_t effect_TCB[NUM_EFFECTS];
const char* effect_name[] = {"explosion1","fastinvader1","fastinvader2","fastinvader3","fastinvader4",
                            "invaderkilled","shoot","ufo_highpitch","ufo_lowpitch"};


// define storage for the ISR to Mixer and Mixer to ISR queues
static StaticQueue_t MixerToISRqueue_QCB, ISRToMixerqueue_QCB;
static uint16_t *MixerToISRqueue_buf[NUM_MIXER_BUFFERS];
static uint16_t *ISRToMixerqueue_buf[NUM_MIXER_BUFFERS];

void effect_init() // main should call this function to set up the sound effects
{
  int i;
  
  // create all of the queues that will be used by the effects tasks
  // to send data to the mixer. Store their handles in the
  // effect_to_mixer_queues array
  for(i=0; i<NUM_EFFECTS; i++){
    // effect_to_mixer_queues[i] = xQueueCreate(effect_task_params[i].buffers, EFFECT_BUFFER_SIZE);
    effect_task_params[i].sendqueue = xQueueCreate(effect_task_params[i].buffers, EFFECT_BUFFER_SIZE);
  }
  // effect_to_mixer_queues[0] = xQueueCreate(NUM_explosion1_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[1] = xQueueCreate(NUM_fastinvader1_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[2] = xQueueCreate(NUM_fastinvader2_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[3] = xQueueCreate(NUM_fastinvader3_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[4] = xQueueCreate(NUM_fastinvader4_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[5] = xQueueCreate(NUM_invaderkilled_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[6] = xQueueCreate(NUM_shoot_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[7] = xQueueCreate(NUM_ufo_highpitch_BUFFERS, EFFECT_BUFFER_SIZE);
  // effect_to_mixer_queues[8] = xQueueCreate(NUM_ufo_lowpitch_BUFFERS, EFFECT_BUFFER_SIZE);

  // create the two queues to communicate between the mixer and the ISR
  MixerToISRqueue = xQueueCreateStatic(NUM_MIXER_BUFFERS,sizeof(uint16_t*), (uint8_t*)MixerToISRqueue_buf, &MixerToISRqueue_QCB);
  ISRToMixerqueue = xQueueCreateStatic(NUM_MIXER_BUFFERS,sizeof(uint16_t*), (uint8_t*)ISRToMixerqueue_buf, &ISRToMixerqueue_QCB);
  
  // create all of the effect tasks, giving them each a unique queue handle and
  // other parameters (effect_params)
  for(i=0; i<NUM_EFFECTS; i++){  
    effect_handles[i] = xTaskCreateStatic(effect_task, effect_name[i], EFFECT_STACK_SIZE,
                        &effect_task_params[i], 2, effect_stacks[i], &effect_TCB[i]);
  }

  // create the mixer task
  mixer_task_handle = xTaskCreateStatic(effect_mixer_task, "mixer task", MIXER_STACK_SIZE, 
    NULL,3,mixer_stack,&mixer_TCB);	
}