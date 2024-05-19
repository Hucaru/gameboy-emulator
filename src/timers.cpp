#include "emulator.h"

void timers_cycle(Timers *timers)
{
    // if clock disabled return

    --timers->cycles_remaining;

    if (timers->cycles_remaining != 0)
    {
        return;
    }
}

void timers_init(Timers *timers)
{
    
}