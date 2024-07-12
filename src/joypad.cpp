#include "emulator.h"
#include <cstdio>

const u8 A = 0x01;
const u8 RIGHT = 0x10;

const u8 B = 0x02;
const u8 LEFT = 0x20;

const u8 SELECT = 0x04;
const u8 UP = 0x40;

const u8 START = 0x08;
const u8 DOWN = 0x80;

void
handle_input_event(Input_events *events, Memory_Bus *memory_bus)
{
    u8 test = 0xFF;
    bool interrupt = false;
    
    if (keyboard_down(events, Input_events::KEY_CODE::RIGHT))
    {
        test &= ~(RIGHT);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::RIGHT))
    {
        test |= RIGHT;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::LEFT))
    {
        test &= ~(LEFT);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::LEFT))
    {
        test |= LEFT;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::UP))
    {
        test &= ~(UP);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::UP))
    {
        test |= UP;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::DOWN))
    {
        test &= ~(DOWN);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::DOWN))
    {
        test |= DOWN;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::A))
    {
        test &= ~(A);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::A))
    {
        test |= A;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::B))
    {
        test &= ~(B);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::B))
    {
        test |= B;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::BACK))
    {
        test &= ~(SELECT);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::BACK))
    {
        test |= SELECT;
    }

    if (keyboard_down(events, Input_events::KEY_CODE::RETURN))
    {
        test &= ~(START);
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::RETURN))
    {
        test |= START;
    }

    if (test != 0xFF)
    {
        memory_bus->joypad_state = test;
        perform_interrupt(memory_bus, INTERRUPT_JOYPAD);
    }
}