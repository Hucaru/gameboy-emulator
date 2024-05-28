#include "emulator.h"
#include <cstdio>

// Number of T cycles each clock mode takes
constexpr u32 CLOCK_TICK_CYCLES[4] = {256 * 4, 4 * 4, 16 * 4, 64 * 4};

void timers_set_tac(Timers *timers, u8 tac)
{
    u8 new_mode = tac & 0x03;

    if (timers->mode != new_mode)
    {
        timers->tima_cycles_remaining = CLOCK_TICK_CYCLES[new_mode];
        timers->mode = new_mode;
    }

    timers->enabled = tac & 0x04;
}

void timers_cycle(Timers *timers, Memory_Bus *memory_bus)
{
    --timers->div_cycles_remaining;

    if (timers->div_cycles_remaining == 0)
    {
        timers->div_cycles_remaining = CLOCK_TICK_CYCLES[3];
        memory_bus->memory[DIV]++;
    }

    if (timers->enabled)
    {
        --timers->tima_cycles_remaining;
        if (timers->tima_cycles_remaining == 0)
        {
            timers->tima_cycles_remaining = CLOCK_TICK_CYCLES[timers->mode];

            if (memory_bus->read_u8(TIMA) == 0xFF)
            {
                memory_bus->write_u8(TIMA, memory_bus->read_u8(TMA));
                // TODO: Interrupt
            }
            else
            {
                memory_bus->write_u8(TIMA, memory_bus->read_u8(TIMA) + 1);
            }
        }
    }
}

void timers_init(Timers *timers, Memory_Bus *memory_bus)
{
    timers->enabled = true;
    timers->tima_cycles_remaining = CLOCK_TICK_CYCLES[0];
    timers->div_cycles_remaining = CLOCK_TICK_CYCLES[3];
}