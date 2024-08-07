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
set_joypad_state(Input_events *events, Joypad *joypad)
{
    u8 test = 0;

    if (keyboard_down(events, Input_events::KEY_CODE::RIGHT))
    {
        test = RIGHT;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::RIGHT))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::LEFT))
    {
        test = LEFT;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::LEFT))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::UP))
    {
        test = UP;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::UP))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::DOWN))
    {
        test = DOWN;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::DOWN))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::A))
    {
        test = A;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::A))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::B))
    {
        test = B;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::B))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::BACK))
    {
        test = SELECT;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::BACK))
    {
        joypad->state = 0xFF;
    }
    else if (keyboard_down(events, Input_events::KEY_CODE::RETURN))
    {
        test = START;
    }
    else if (keyboard_up(events, Input_events::KEY_CODE::BACK))
    {
        joypad->state = 0xFF;
    }

    // Check if previously the corresponding bit was set to 1
    u8 prev_state = joypad->state & test;

    if (test > 0 && prev_state)
    {
        if (test < RIGHT)
        {
            joypad->button = true;
        }
        else
        {
            joypad->direction = true;
        }

        joypad->state = 0xFF & (~test);
    }
}

void
handle_input_event(Memory_Bus *memory_bus)
{
    u8 req = memory_bus->memory[JOYPAD_REGISTER];

    if (memory_bus->joypad.button && (req & JOYPAD_BUTTON_REQUEST) == 0)
    {
        perform_interrupt(memory_bus, INTERRUPT_JOYPAD);
        memory_bus->joypad.button = false;
    }
    else if (memory_bus->joypad.direction && (req & JOYPAD_DIRECTION_REQUEST) == 0)
    {
        perform_interrupt(memory_bus, INTERRUPT_JOYPAD);
        memory_bus->joypad.direction = false;
    }
}