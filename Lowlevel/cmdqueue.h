#ifndef _CMDQUEUE_H
#define _CMDQUEUE_H

#include "config.h"
#include <pico.h>
typedef enum{
    INVALID = 0, //
    SLEEP_LONG,
    SLEEP,
    //toggle_override
    TOGGLE_MUNDWERK,
    TRIGGER_SERVO,
    TRIGGER_AUDIO,
    PLAY_CUSTOM_AUDIO,
    STREAM_START,
    //volume_up
    //volume_down    
}commands_t;

#define AUDIO_SEGMENT_LENGTH 64
//32768
#define AUDIO_BUFFER_SIZE (AUDIO_SEGMENT_LENGTH*512)



typedef struct 
{
    uint16_t command;
    uint16_t param;
} command_element;

bool enqueue(const command_element e);
command_element dequeue();

uint16_t insert_audio(uint8_t* data);
bool audio_available();
uint8_t* getAudioBuffer();
void setReadPosition(uint16_t index);
void finish_stream();
#endif