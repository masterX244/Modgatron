#include "cmdqueue.h"
#include "string.h"

command_element queue[512];
uint16_t read_index = 0;
uint16_t write_index = 0;



bool enqueue(const command_element e)
{
   if(queue[write_index].command==INVALID)
   {
       queue[write_index] = e;
       write_index++;
       write_index %=512;
       return true;
   }
   return false;
}

command_element dequeue()
{
    command_element retval = queue[read_index];
    if(retval.command!=INVALID)
    {
        queue[read_index].command=INVALID;
        read_index++;
        read_index %=512;
    }
    return retval;
}



uint8_t audio_buffer[AUDIO_BUFFER_SIZE] __attribute__ ((aligned (16)));
uint16_t currentReadIndex;
uint16_t currentWriteIndex = 64;
bool streaming=false;

uint16_t insert_audio(uint8_t* data)
{
    uint16_t delta = (currentReadIndex - currentWriteIndex)&0x7FFF;
    if((delta<128)&&streaming)
    {
        return delta|=0x8000;
    }
    memcpy(audio_buffer+currentWriteIndex,data,64);
    currentWriteIndex+=64;
    currentWriteIndex=currentWriteIndex%32768;
    streaming=true;
    return delta-64;
}
bool audio_available()
{
    return currentReadIndex != currentWriteIndex;
}
uint8_t* getAudioBuffer()
{
    return audio_buffer;
}
void setReadPosition(uint16_t index)
{
    currentReadIndex = index;
}

void finish_stream()
{
    streaming=false;
    currentReadIndex=0;
    currentWriteIndex=64;
    //TODO Zeroize first 64 byte of buffer
}
