#include <stdlib.h>
#include "IO_Driver.h"

#include "timerDebug.h"

/*
to verify function, make a dummy function in this file that has its own rtc to do a busy wait for 20 micro sec and then time that. 

RTC timer is adds around 5-7 microseconds. 
*/
void TimerDebug_new(TimerDebug *me)
{
    me->prevTime = 0;
    me->RTCTimer = 0;
    me->timerUpdateIntervalCounter = TIMER_UPDATE_INTERVAL_LIM ; //to start timer right away set to TIMER_UPDATE_INTERVAL_LIM 
}

void TimerDebug_startTimer(TimerDebug *me)
{
    if (me->timerUpdateIntervalCounter == TIMER_UPDATE_INTERVAL_LIM)
    {
        me->timerUpdateIntervalCounter = 0;
        if (IO_RTC_StartTime(&(me->RTCTimer)) != IO_E_OK)
        {
            me->RTCTimer = 0xFFFF;
        }
    }
    else {
        me->timerUpdateIntervalCounter++;
    }
}

void TimerDebug_stopTimer(TimerDebug *me)
{
    if (me->timerUpdateIntervalCounter == 0)
    {
        me->prevTime = IO_RTC_GetTimeUS(me->RTCTimer);
        me->RTCTimer = 0;
    }
}

void TimerDebug_testTimer()
{
    ubyte4 dummy = 0;
    IO_RTC_StartTime(&dummy);
    while (IO_RTC_GetTimeUS(dummy) < 0x45);    
}

ubyte4 TimerDebug_getTime(TimerDebug *me)
{

    return me->prevTime;
}

ubyte2 TimerDebug_getIntervalCounter(TimerDebug *me)
{
    return me->timerUpdateIntervalCounter;
}