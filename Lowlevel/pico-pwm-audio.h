#ifndef _PICO_PWM_AUDIO_H
#define _PICO_PWM_AUDIO_H
#include "config.h"


#define countSamplesPrePlay 44000

void pwm_interrupt_handler(void);
void audio_main(void);
void playAudio(uint8_t sample_id,bool autoMundwerk);
void show_samples();
void randomPlay();

// int is only for some voodoo
// https://stackoverflow.com/questions/11535124/calling-functions-automatically-in-c-c
int registerSample(uint8_t* base, uint32_t len, char* name,bool randomChooseable);

void changeVolumeConfig(uint8_t volumelevel);

void mundwerkSwitcheroo(bool enable);
bool getMundwerkState();

#endif