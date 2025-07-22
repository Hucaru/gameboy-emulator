#include "emulator.h"

void 
Memory_Bus::write_u8(u16 address, u8 v) 
{
    memory[address] = v;
}

u8 
Memory_Bus::read_u8(u16 address) 
{
    return memory[address];
}

void 
Memory_Bus::write_i8(u16 address, i8 v) 
{
    write_u8(address, static_cast<u8>(v));
}

i8 
Memory_Bus::read_i8(u16 address) 
{
    return static_cast<i8>(read_u8(address));
}

void 
Memory_Bus::write_u16(u16 address, u16 v) 
{
    write_u8(address, v & 0xFF);
    write_u8(address + 1, (v >> 8) & 0xFF);
}

u16 
Memory_Bus::read_u16(u16 address) 
{
    return static_cast<u16>(read_u8(address)) | static_cast<u16>(read_u8(address + 1)) << 8;
}

void
memory_bus_init(Memory_Bus *memory_bus, Timers *timers)
{
    memory_bus->timers = timers;
    memory_bus->memory[JOYPAD_REGISTER] = 0x3F;
    memory_bus->joypad.state = 0xFF;
    memory_bus->joypad.button = false;
    memory_bus->joypad.direction = false;
}