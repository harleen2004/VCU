#ifndef _WATCH_DOG_H
#define _WATCH_DOG_H

#include "IO_Driver.h"

typedef struct _WatchDog {
    ubyte4 timestamp;
    ubyte4 timeout;
    bool running;
    ubyte1 mood; // used as a counter to determine if we should clear the watchdog
} WatchDog;

void WatchDog_new(WatchDog* wd, ubyte4 timeout);
void WatchDog_pet(WatchDog* wd);
void WatchDog_reset(WatchDog* wd);
bool WatchDog_check(WatchDog* wd);

#endif // _WATCH_DOG_H