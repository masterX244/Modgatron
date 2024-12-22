#include <stdio.h>
#include <pico.h>
#include "pico/stdlib.h"   // stdlib 
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "pico/rand.h"
#include "pico-pwm-audio.h"
#include "control.h"
#include "cmdqueue.h"
#include "generated/servo_bb.pio.h"
#include "generated/audio_bb.pio.h"
#include "hardware/pio.h"



#include "generated/servo_bb.pio.h"
#include "generated/audio_bb.pio.h"


typedef struct audio_record{
  uint8_t* data;
  uint32_t len;
  char* label;
  bool randomChooseable;
} audio_record;
uint16_t audio_count = 0; //next free slot counter

uint8_t* WAV_DATA = 0;
uint32_t WAV_DATA_LENGTH = 0;

audio_record records[256];
uint8_t randomSlots[256];
uint8_t random_count;
bool autoMundwerkInternal = false;

bool mundwerk_state = false;



/* 
 * This include brings in static arrays which contain audio samples. 
 * if you want to know how to make these please see the python code
 * for converting audio samples into static arrays. 
 */
bool no_playback = true;
signed int wav_position = 0;
volatile int wait = 0;
volatile bool playStuffOnNextCycle=false;
volatile int volumeMult=DEFAULT_VOLUME;

/*
 * PWM Interrupt Handler which outputs PWM level and advances the 
 * current sample. 
 * 
 * We repeat the same value for 8 cycles this means sample rate etc
 * adjust by factor of 8   (this is what bitshifting <<3 is doing)
 * 
 */
bool stream_mode = false;
uint8_t* audio_base;
uint32_t internal_counter = 0;

 
volatile bool testflag = false;
void pwm_interrupt_handler() {
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    
    testflag=!testflag;
    //gpio_put(PIO_TEST_PIN,testflag?1:0);//Haxx für nen test
    if(stream_mode)
    {
        if(audio_available())
        {
            pwm_set_gpio_level(AUDIO_PIN, audio_base[(internal_counter>>3)]*volumeMult); 
            internal_counter++;
            internal_counter = internal_counter%(AUDIO_BUFFER_SIZE<<3); //Zuweisen nicht vergessen du dödel, sonst Rampfuscher32.exe
            setReadPosition((uint16_t)(internal_counter>>3));
        }
        else
        {
            finish_stream();
            pwm_set_gpio_level(AUDIO_PIN, 127);
            pwm_set_mask_enabled(0);
            gpio_put(MODE_PIN, 1);
        }
        
    }
    else
    {
        if (wav_position < countSamplesPrePlay+((WAV_DATA_LENGTH<<3) - 1)) { 
            // set pwm level 
            // allow the pwm value to repeat for 8 cycles this is >>3 
            if(wav_position>=countSamplesPrePlay)
            {
                pwm_set_gpio_level(AUDIO_PIN, (WAV_DATA[(wav_position-countSamplesPrePlay)>>3])*volumeMult); 
            } 
            else
            {
                pwm_set_gpio_level(AUDIO_PIN, 127); //nullinie, pre-sample-pause 
            }
            wav_position++;
        } else {
            wav_position=0;
            pwm_set_gpio_level(AUDIO_PIN, WAV_DATA[0]);
            pwm_set_mask_enabled(0);
            no_playback = true; //!analog_passthrough;  //TODO false nach sample-trigger
            playStuffOnNextCycle=false;
            if(autoMundwerkInternal)
            {
                mundwerkSwitcheroo(false);
            }
            autoMundwerkInternal=false;
            /*if(playStuffOnNextCycle)
            {
                no_playback=false;
                playStuffOnNextCycle = false;
            }*/
            gpio_put(MODE_PIN, 1);
        }
    }
}

void playAudio(uint8_t sample_id,bool autoMundwerk)
{
    stream_mode=false;
    if((&records[sample_id].data)==NULL)
    {
        return; //TODO: GAH!
    }
    if(!playStuffOnNextCycle)
    {
        if(autoMundwerk)
        {
            mundwerkSwitcheroo(true);
            autoMundwerkInternal=true;
        }
        WAV_DATA = records[sample_id].data;
        WAV_DATA_LENGTH=records[sample_id].len;
        playStuffOnNextCycle = true; //TODO bessere Logik
        int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
        pwm_set_mask_enabled((1u << audio_pin_slice));
        pwm_set_gpio_level(AUDIO_PIN, 250*4);
        gpio_put(MODE_PIN, 0);
    }
}

pwm_config config;


void changeVolumeConfig(uint8_t volumelevel)
{ 
    if(volumelevel>8)
    {
        volumelevel=8;
    }
    volumeMult=volumelevel;
}

uint64_t sleep_target = 0;
bool in_sleep=false;

PIO pio = pio0;
uint sm = 0;
uint sm2 = 1;
bool testwiggler=true;

void audio_main(void) {
    
    audio_base = getAudioBuffer();
        
        
    gpio_init(MUNDWERK_PIN);
    gpio_set_dir(MUNDWERK_PIN, GPIO_OUT);
    gpio_put(MUNDWERK_PIN,HIGH);

    gpio_init(MODE_PIN);
    gpio_set_dir(MODE_PIN, GPIO_OUT);
    gpio_put(MODE_PIN, HIGH);

    //setting the PIO bitbang utils
    uint offset = pio_add_program(pio, &servo_bb_program);
    servo_bb_program_init(pio, sm, offset, SIGNAL_PIN_FAST);

    offset = pio_add_program(pio, &audio_bb_program);
    audio_bb_program_init(pio, sm2, offset, SIGNAL_PIN);
    
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    
    
   

    // PWM interrupt stuff
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler); 
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // Setup PWM for audio output
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); 
    pwm_config_set_wrap(&config, 250*8);
    pwm_init(audio_pin_slice, &config, false);

    //devicecontrol core main-loop
    while(1) {
        if(in_sleep)
        {
            uint64_t msSinceBoot = to_ms_since_boot(get_absolute_time());
            if(msSinceBoot>sleep_target)
            {
                in_sleep=false;
            }
        }
        else
        {
            command_element ele = dequeue();
            switch(ele.command)
            {
                case INVALID: 
                    {
                            //gpio_put(TEST_PIN, testwiggler?1:0);
                            //testwiggler=!testwiggler;
                    }
                break; //NOP NOP NOP;
                case SLEEP_LONG: 
                    {
                        uint32_t sleepMs = ele.param*1000;
                        uint64_t msSinceBoot = to_ms_since_boot(get_absolute_time());
                        sleep_target = msSinceBoot+sleepMs;
                        in_sleep=true;
                    }
                break;
                case SLEEP:
                    { 
                        uint32_t sleepMs = ele.param;
                        uint64_t msSinceBoot = to_ms_since_boot(get_absolute_time());
                        sleep_target = msSinceBoot+sleepMs;
                        in_sleep=true;
                    }
                break;
                case TOGGLE_MUNDWERK: 
                    mundwerk_state = !mundwerk_state;
                    gpio_put(MUNDWERK_PIN,mundwerk_state?LOW:HIGH);
                break;
                case TRIGGER_SERVO: 
                    {
                        uint8_t sendBits = (uint8_t) (ele.param&0x0F);
                        uint8_t sendBitsFast = sendBits|((sendBits<<4)&0xF0); //faulpelz.exe, high 4 are duped
                        servo_bb_send(pio,sm,sendBitsFast);
                    }
                break;
                case TRIGGER_AUDIO: 
                    {
                        uint8_t sendBits = (uint8_t) (ele.param&0x0F);
                        audio_bb_send(pio,sm2,sendBits);
                    }
                break;
                case PLAY_CUSTOM_AUDIO: 
                    uint16_t index = ele.param;
                    playAudio(index&0xff,index>256);
                break;
                case STREAM_START:
                    stream_mode=true;
                    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
                    pwm_set_mask_enabled((1u << audio_pin_slice));
                    pwm_set_gpio_level(AUDIO_PIN, 0);
                    gpio_put(MODE_PIN, 0);
                break;
            }
        }
    }
}

bool getMundwerkState()
{
    return mundwerk_state;
}


//
void randomPlay()
{
    uint32_t randomRaw = get_rand_32();
    command_element elem;
    elem.command=PLAY_CUSTOM_AUDIO;
    elem.param=(randomSlots[randomRaw%random_count])+512;
    enqueue(elem);
    
}

void show_samples()
{
    printf("--- Audio ---\n");
    for(int i=0;i<audio_count;i++)
    {
        printf("%d|%d|%s\n",i,records[i].randomChooseable,records[i].label);
    }
    printf("---  END  ---\n");
}

int registerSample(uint8_t* base, uint32_t len, char* name,bool randomChooseable)
{
    if(audio_count<256)
    {
        records[audio_count].data=base;
        records[audio_count].len=len;
        records[audio_count].label=name;
        records[audio_count].randomChooseable=randomChooseable;
        if(randomChooseable)
        {
            randomSlots[random_count]=audio_count;
            random_count++;
        }
        audio_count++;

        return audio_count-1;
    }
    else
    {
        return -1;
    }
}

//enable logic with builtin fallback to previous state for auto-mouthwiggling revert
void mundwerkSwitcheroo(bool enable)
{
  if(!enable)
  {
    gpio_put(MUNDWERK_PIN,mundwerk_state?LOW:HIGH);
  }
  else
  {
    gpio_put(MUNDWERK_PIN,LOW);
  }
}
