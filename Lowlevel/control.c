#include <stdio.h>
#include "pico.h"
#include "pico/stdlib.h"   // stdlib 
#include "pico/stdio.h"
#include "hardware/irq.h"  // interrupts
#include "hardware/sync.h" // wait for interrupt
#include "hardware/clocks.h" //FICKT EUCH FÜR DAS VERLEGEN DIESER DETAILS OHNE KOMMENTAR
#include <pico/time.h>
#include <pico/multicore.h>
#include "pico/bootrom.h"
#include "control.h"
#include <pico/rand.h>
#include "pico-pwm-audio.h"
#include "hardware/pio.h"

#include "cmdqueue.h"

volatile uint64_t timerTicksSignal1 = 0;
volatile uint8_t sendBits = 0;
volatile uint8_t sendBitsFast = 0;
volatile bool signal2EdgeDirection = true;
volatile uint8_t sentBitsSignal2 = 0;

volatile int64_t debug = 0;
volatile uint64_t debug2 = 0;
volatile uint64_t debug3 = 0;



char* autoexec =  "*";//"S6G*3e8W*SCG*3e8W*SAG*";//MSCGWWWWM //SCG

bool randomEnabled = false;

uint8_t volume=DEFAULT_VOLUME;
char* klatschi = "NOPE";
char* randompool[] =  {
"*SCG*",
"*MSCGWWWWM*",
"*M0v7D0WM*",
"*SCG*1f4WSCG*1f4WSCG*",
"*M1v*BB8WM*",
"*S6G*3e8W*SAG*"
"*SCG*",
"*SCG*",
"*SCG*",
"*M1f4WM*",
"*M1f4WM*",
"*M1f4WM*",
"*MSCGWWWWM*",
"*MSCGWWWWM*",
"*SCG*1AV*FFW*SCG*"
//"-"
};
uint8_t randsize=sizeof(randompool)/sizeof(randompool[0]);
char* autoexec_reset = NULL;
char peekedButNotTaken = 0;
volatile bool randomTickReached = false;
volatile uint64_t targetTicks = 0;
volatile uint64_t timerTicks = 1;


struct repeating_timer timer;

bool protocolAudioIC=false;
bool protocolServo1=false;

bool override_state=true;

uint64_t fuckyou = 0;
uint64_t internal_haxx = 0;

typedef enum state 
{
  IDLE,
  COMMAND,
  NUMERIC_PARAM,
  COMMIT,
} State;


uint64_t accumulator=0;

State state = IDLE;




void setup(void)
{
    /* Overclocking for fun but then also so the system clock is a 
     * multiple of typical audio sampling rates.
     */
    stdio_init_all();
    //sleep_ms(5000);
    printf("GO!");    
    set_sys_clock_khz(176000, true); 
    autoexec_reset = autoexec; // ,,|,,
    gpio_init(OVERRIDE_MODE_PIN);
    gpio_set_dir(OVERRIDE_MODE_PIN, GPIO_OUT);
    gpio_put(OVERRIDE_MODE_PIN,0);//LOW
    
    
    multicore_launch_core1(audio_main); //

    long usPeriod = -1*TIMER_INTERVAL_US;
    printf("timer setzen");
    rearmTimerAndReturnRandomUsed();
    printf("setup done");
    
    gpio_init(DEBUG_PIN);
    gpio_set_dir(DEBUG_PIN, GPIO_OUT);
    gpio_put(DEBUG_PIN,0);//LOW

    gpio_init(DEBUG_PIN2);
    gpio_set_dir(DEBUG_PIN2, GPIO_OUT);
    gpio_put(DEBUG_PIN2,0);//LOW
}

int serial_char = -1;


void seedNextSerialChar()
{
    if(serial_char<0)
    {
        serial_char = getchar_timeout_us(0);
    }
}

bool charAvailable ()
{
  seedNextSerialChar();
  /*
  Serial.println(Serial.available());
  Serial.println(peekedButNotTaken);*/
  //Serial.println((int)autoexec[0]);
  bool retval = (serial_char >=0) || (peekedButNotTaken!=0)||(autoexec[0]!=0);
  //bool retval = "Serial.available() >0 || peekedButNotTaken!=0||autoexec[0]==0;";
  //Serial.println(retval?"true":"false");
  return retval;
}

char peekNext ()
{
  seedNextSerialChar();
  if(autoexec[0]!=0)
  {
    return autoexec[0];
  }
  if(peekedButNotTaken!=0)
  {
    return peekedButNotTaken;
  }
  char c = (char)(serial_char&0xFF);
  return c;
}

char getNext()
{
  seedNextSerialChar();
  if(autoexec[0]!=0)
  {
    char rv =  autoexec[0];
    //printf("Autoexec taking: %d",(int)autoexec[0]);
    //printf("Klatschi%s",klatschi);
    autoexec = autoexec+1;
    return rv;
  }
  if(peekedButNotTaken!=0)
  {
    char retval = peekedButNotTaken;
    peekedButNotTaken=0;
    return retval;
  }
  char c = (char)(serial_char&0xFF);
  serial_char=-1;
  return c;
}

// Never use Serial.print inside this mbed ISR. Will hang the system
int64_t TimerHandler(alarm_id_t id, __unused void *user_data)
{
    uint32_t randomVal = rearmTimerAndReturnRandomUsed();
    printf("rearmed_random");
    if(randomEnabled) //no random as autoexec;
    {
      uint32_t randomChoice = randomVal%randsize;
      //printf("randomPick:%d\n",randomChoice);
      autoexec=randompool[randomChoice];
      klatschi=autoexec;
      //printf("LOA= %d",(uint32_t)&(randompool[randomChoice]));
      printf("%s\n",randompool[randomChoice]);
    }
    //autoexec=autoexec_reset;
      //return true;
      timerTicks++;
      debug++;
      return 0;
}


uint32_t rearmTimerAndReturnRandomUsed()
{
      uint32_t random_raw = get_rand_32();
      uint32_t randomClamped = random_raw%(RANDOM_TIME);
      uint32_t targetMs=RANDOM_TIME_MIN+randomClamped;
      add_alarm_in_ms(targetMs, TimerHandler, (void*)0x00, &timer); 
      internal_haxx++;
      return random_raw;
}

uint8_t segment_buffer[AUDIO_SEGMENT_LENGTH] __attribute__ ((aligned (16)));
int main(void) 
{
    setup();
    while(1)
    {

        bool commitSend = false;
        // put your main code hererial.available() >0e, to run repeatedly:
        if (charAvailable()) 
        {
            char c = getNext();  //gets one byte from serial buffer
            printf("nxtChar:%d\n",c);
            
            if(c=='>')
            {
                gpio_put(DEBUG_PIN,1);//LOW
                int bufoffset = 0;
                while(bufoffset<64)
                {
                   int got = stdio_get_until((char*)segment_buffer+bufoffset, 64-bufoffset, at_the_end_of_time);
                   bufoffset +=got;
                }
                gpio_put(DEBUG_PIN,0);//LOW
                int bufferdelta = insert_audio(segment_buffer);
                gpio_put(DEBUG_PIN2,1);//LOW
                if(bufferdelta&0x8000)
                {
                    printf("<DLY=%d\n",bufferdelta&0x7FFF);   
                }
                else
                {
                    printf("~A_OK=%d\n",bufferdelta);
                }
                gpio_put(DEBUG_PIN2,0);//LOW
            }
            if(c=='|')
            {
                command_element elem;
                elem.command=STREAM_START;
                enqueue(elem);
            }
            if(c>='0'&&c<='9')
            {
            sendBits = c-'0';
            accumulator = accumulator<<4|sendBits;
            }
            if(c>='A'&&c<='F')
            {
            sendBits = (c-'A')+10;
            accumulator = accumulator<<4|sendBits;
            }
            if(c>='a'&&c<='f')
            {
            sendBits = (c-'a')+10;
            accumulator = accumulator<<4|sendBits;
            }



            if(c=='T'||c=='t')
            {
            protocolAudioIC=true;
            protocolServo1=false;
            }
            if(c=='S'||c=='s')
            {
            protocolAudioIC=false;
            protocolServo1=true;
            }




            if(c=='G'||c=='g')
            {
            commitSend=true;
            }
            if(c=='W'||c=='w')
            {
                uint16_t sleepDur = 100;
              if(accumulator!=0)
              {
                sleepDur = (uint16_t)accumulator;//TODO long sleep
              }
              
                command_element elem;
                elem.command=SLEEP;
                elem.param=sleepDur;
                enqueue(elem);
            }
            if(c=='V'||c=='v')
            {
              
                uint16_t index = (accumulator&0xFF); 
                if(c=='V')
                {
                    index+=512; //bit eintwiddeln
                }
                
                command_element elem;
                elem.command=PLAY_CUSTOM_AUDIO;
                elem.param=index;
                enqueue(elem);
              
              
              accumulator=0;
            }
            if(c=='R'||c=='r')
            {
              randomPlay();
            }



            if(c=='M'||c=='m')
            {
                command_element elem;
                elem.command=TOGGLE_MUNDWERK;
                enqueue(elem);
            }
            if(c=='O'||c=='o')
            {
                override_state=!override_state;
                gpio_put(OVERRIDE_MODE_PIN,override_state?LOW:HIGH);
            }


            if(c=='Z'||c=='z')
            {
                show_samples();
            }

            if(c=='$')
            {
                reset_usb_boot(1<<PICO_DEFAULT_LED_PIN,0); //invokes reset into bootloader mode
            }

            if(c=='*')
            {
                accumulator=0;
            }

            if(c=='?')
            {
                randomEnabled=!randomEnabled;
            }

            if(c=='-')
            {
              if(volume>0)
              {
                volume--;
                changeVolumeConfig(volume);
              }
            }
            if(c=='+')
            {
              if(volume<8)
              {
                volume++;
                changeVolumeConfig(volume);
              }
            }
            if(c!='\n'&&c!='>')
            {
#ifndef SILENCIO
              if(c=='L'||c=='l')
              {
                 fprintf(stdout,"timerTickRaw=%d\n",timerTicks);
                 fprintf(stdout,"TimerTicksRandomTarget=%d\n",targetTicks);
              }

              fprintf(stdout,"Settings:\n");
              fprintf(stdout,"ProtocolAudio (T)=%s\n",protocolAudioIC?"True":"False");
              fprintf(stdout,"ProtocolServo1 (S)=%s\n",protocolServo1?"True":"False");
              fprintf(stdout,"BitsToSend=%d\n",sendBits);
              fprintf(stdout,"BitsToSendF=%d\n",sendBitsFast);
              fprintf(stdout,"Debug2=%d\n",debug2);
              fprintf(stdout,"Debug3=%d\n",debug3);
              fprintf(stdout,"Override enabled (O) %d\n",override_state);
              fprintf(stdout,"Mundwerk (M) %d\n",getMundwerkState());
              fprintf(stdout,"Accumulator (reset: *) %d\n",accumulator);
              fprintf(stdout,"Random (?) %d\n",randomEnabled);
              fprintf(stdout,"volumeLevel (+ -) %d\n",volume);
              fflush(stdout);
#else
              if(c=='L'||c=='l')
              {
                 fprintf(stdout,"timerTickRaw=%d\n",timerTicks);
                 fprintf(stdout,"TimerTicksRandomTarget=%d\n",targetTicks);
                  fprintf(stdout,"Settings:\n");
                  fprintf(stdout,"ProtocolAudio (T)=%s\n",protocolAudioIC?"True":"False");
                  fprintf(stdout,"ProtocolServo1 (S)=%s\n",protocolServo1?"True":"False");
                  fprintf(stdout,"BitsToSend=%d\n",sendBits);
                  fprintf(stdout,"BitsToSendF=%d\n",sendBitsFast);
                  fprintf(stdout,"Debug2=%d\n",debug2);
                  fprintf(stdout,"Debug3=%d\n",debug3);
                  fprintf(stdout,"Override enabled (O) %d\n",override_state);
                  fprintf(stdout,"Mundwerk (M) %d\n",getMundwerkState());
                  fprintf(stdout,"Accumulator (reset: *) %d\n",accumulator);
                  fprintf(stdout,"Random (?) %d\n",randomEnabled);
                  fprintf(stdout,"volumeLevel (+ -) %d\n",volume);
                            }
              fflush(stdout);
#endif

            }
        }
        if(commitSend)
        {
            printf("Commited...");
            sendBitsFast = sendBits|((sendBits<<4)&0xF0); //faulpelz.exe, high 4 are duped
            if(protocolAudioIC)
            {
                command_element elem;
                elem.command=TRIGGER_AUDIO;
                elem.param=(uint8_t)sendBits;
                enqueue(elem);
            }
            if(protocolServo1)
            {
                command_element elem;
                elem.command=TRIGGER_SERVO;
                elem.param=(uint8_t)sendBits;
                enqueue(elem);
            }
            //Serial.println(sendBitsFast,BIN);
            fflush(stdout);
            sleep_ms(50);
            debug2=0;
        } 
    }
}