#include "emulator.h"
#include <cstdio>

// Bit set to 1 means not interrested therefore swap the values
const u8 JOYPAD_DIRECTION_REQUEST = 0x20;
const u8 JOYPAD_BUTTON_REQUEST = 0x10;

void 
dma_transfer(Memory_Bus *memory_bus, u16 address)
{
    for (u8 i = 0; i < 0xA0; ++i)
    {
        memory_bus->write_i8(0xFE00+i, memory_bus->read_u8(address + i));
    }
}

void 
Memory_Bus::write_u8(u16 address, u8 v) 
{
    if (address < 0x8000) // ROM bank 00
    {
        // printf("[Memory bus] attempting to write to read only memory %u\n", address);
    }
    else if (address <= 0x7FFF) // ROM Bank 0..N (cartridge switchable bank)
    {
        memory[address] = v;
    }
    else if (address <= 0x9FFF) // VRAM (switchable bank 0-1 in CGB Mode)
    {
        memory[address] = v;
    }
    else if (address <= 0xBFFF) // External RAM (in cartridge, switchable bank, if any)
    {
        memory[address] = v;
    }
    else if(address <= 0xDFFF) // WRAM Bank 1 (switchable bank 1-7 in CGB Mode)
    {
        memory[address] = v;
    }
    else if (address >= 0xE000 && address < 0xFE00) // ECHO
    {
        memory[address] = v;
        write_u8(address - 0x2000, v);
    }
    else if(address >= 0xFEA0 && address <= 0xFEFF) // Not usable
    {
        // printf("[Memory bus] write to unusable area\n");
    }
    else if (address == DIV)
    {
        memory[address] = 0;
    }
    else if (address == TAC)
    {
        memory[address] = v;
        timers_set_tac(timers, v);
    }
    else if (address == LY_REGISTER)
    {
        memory[address] = 0;
    }
    else if (address == DMA_REGISTER)
    {
        dma_transfer(this, static_cast<u16>(v) << 8);
    }
    else if (address == JOYPAD_REGISTER)
    {
        memory[address] = (v & 0x30) | 0x0F;
    }
    else if (address == SERIAL_DATA_TRANSFER)
    {
        printf("%c", (char)v);
    }
    else
    {
        memory[address] = v;
    }
}

u8 
Memory_Bus::read_u8(u16 address) 
{
    if (address == JOYPAD_REGISTER)
    {
        u8 req = memory[address];

        if (req & JOYPAD_DIRECTION_REQUEST)
        {
            return (memory[address] & 0xF0) | ((joypad_state >> 4) & 0x0F);
        }
        else if (req & JOYPAD_BUTTON_REQUEST)
        {
            return (memory[address] & 0xF0) | (joypad_state & 0x0F);
        }
        else
        {
            return memory[address];
        }
    }
    else
    {
        return memory[address];
    }
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