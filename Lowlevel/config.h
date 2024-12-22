#ifndef _CONFIG_H
#define _CONFIG_H

//#define DEBUG_MODE
//#define SILENCIO

#define SIGNAL_PIN 14
#define SIGNAL_PIN_FAST 13
#define OVERRIDE_MODE_PIN 15
#define MUNDWERK_PIN 12

#define DEFAULT_VOLUME 1

#define AUDIO_PIN 16  // you can change this to whatever you like
#ifdef DEBUG_MODE
#define MODE_PIN 17  //SWITCH BACK AFTER TEST //17 for DBG
#else
#define MODE_PIN 22
#endif

#define DEBUG_PIN 18 //FUCKYOU 16 tut nicht du rektal gefiedelte analfistel.....
#define DEBUG_PIN2 19 //FUCKYOU 16 tut nicht du rektal gefiedelte analfistel.....
#endif