#ifndef _COMMAND_H
#define _COMMAND_H

#include "config.h"


#define TIMER_INTERVAL_US        100000L

//milliseconds, using this now for a timer offset instead of ticks. timer interrupt GOTO HELL;
#define SECONDS_IN_TIMER_TICKS (1000)

//*60
#define RANDOM_TIME (15*60*SECONDS_IN_TIMER_TICKS)
#define RANDOM_TIME_MIN (2*60*SECONDS_IN_TIMER_TICKS)

//python -m serial.tools.miniterm --raw /dev/ttyACM1 115200

#define HIGH 1
#define LOW 0





void setup(void);

void setup(void);
void seedNextSerialChar(void);
bool charAvailable (void);
char peekNext (void);
char getNext(void);
int64_t TimerHandler(alarm_id_t id, __unused void *user_data);
uint32_t rearmTimerAndReturnRandomUsed(void);


int main(void);
#endif