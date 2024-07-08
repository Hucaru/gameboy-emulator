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
    
    Input_events::KEY_STATE state = check_keyboard(events, Input_events::KEY_CODE::RIGHT);
    if (state == Input_events::KEY_STATE::DOWN || state == Input_events::KEY_STATE::HELD)
    {
        test &= ~(RIGHT);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= RIGHT;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::LEFT);
    if (state == Input_events::KEY_STATE::DOWN)
    {
        test &= ~(LEFT);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= LEFT;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::UP);
    if (state == Input_events::KEY_STATE::DOWN)
    {
        test &= ~(UP);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= UP;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::DOWN);
    if (state == Input_events::KEY_STATE::DOWN)
    {
        test &= ~(DOWN);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= DOWN;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::A);
    if (state == Input_events::KEY_STATE::DOWN || state == Input_events::KEY_STATE::HELD)
    {
        test &= ~(A);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= A;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::B);
    if (state == Input_events::KEY_STATE::DOWN)
    {
        test &= ~(B);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= B;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::BACK);
    if (state == Input_events::KEY_STATE::DOWN)
    {
        test &= ~(SELECT);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= SELECT;
    }

    state = check_keyboard(events, Input_events::KEY_CODE::RETURN);
    if (state == Input_events::KEY_STATE::DOWN)
    {
        test &= ~(START);
    }
    else if (state == Input_events::KEY_STATE::UP)
    {
        test |= START;
    }

    if (test != 0xFF)
    {
        memory_bus->joypad_state = test;
        perform_interrupt(memory_bus, INTERRUPT_JOYPAD);
    }
}