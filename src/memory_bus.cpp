#include "emulator.h"
#include <cstdio>

void 
dma_transfer(Memory_Bus *memory_bus, u16 address)
{
    for (u8 i = 0; i < 0xA0; ++i)
    {
        memory_bus->write_i8(0xFE00+i, memory_bus->read_u8(address + i));
    }
}

void
handle_banking(Cartridge *cartridge, u16 address, u8 v)
{
    if (address < 0x2000)
    {
        if (cartridge->mbc1 || cartridge->mbc2)
        {
            if (cartridge->mbc2 && ((address >> 4) & 0x01) == 1)
            {
                return;
            }

            u8 test = v & 0x0F;

            if (test == 0x0A)
            {
                cartridge->ram_bank_enabled = true;
            }
            else
            {
                cartridge->ram_bank_enabled = false;
            }

            printf("[Cartridge] RAM bank changed to: %d\n", cartridge->current_ram_bank);
        }
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        if (cartridge->mbc1 || cartridge->mbc2)
        {
            if (cartridge->mbc2)
            {
                cartridge->current_rom_bank = v & 0x0F;
            }
            else
            {
                u8 lower_5 = v & 0x1F;
                cartridge->current_rom_bank &= 0xE0;
                cartridge->current_rom_bank |= lower_5;
            }

            if (cartridge->current_rom_bank == 0)
            {
                cartridge->current_rom_bank++;
            }

            printf("[Cartridge] ROM bank changed to: %d\n", cartridge->current_rom_bank);
        }
    }
    else if (address >= 0x4000 && address < 0x6000)
    {
        // NOTE: MBC2 has no ram bank
        if (cartridge->mbc1)
        {
            if (cartridge->rom_bank_enabled)
            {
                cartridge->current_rom_bank &= 0x1F;
                cartridge->current_rom_bank |= (v & 0xE0);

                if (cartridge->current_rom_bank == 0)
                {
                    cartridge->current_rom_bank++;
                }

                printf("[Cartridge] ROM bank changed to: %d\n", cartridge->current_rom_bank);
            }
            else
            {
                cartridge->current_ram_bank = v & 0x03;
                printf("[Cartridge] RAM bank changed to: %d\n", cartridge->current_ram_bank);
            }
        }
    }
    else if (address >= 0x6000 && address < 0x8000)
    {
        if (cartridge->mbc1)
        {
            u8 data = v & 0x01;
            cartridge->rom_bank_enabled = (data == 0) ? true : false;

            if (cartridge->rom_bank_enabled)
            {
                cartridge->current_ram_bank = 0;
            }
        }
    }
}

void 
Memory_Bus::write_u8(u16 address, u8 v) 
{
    if (address < 0x8000)
    {
        handle_banking(&cartridge, address, v);
    }
    else if (address < 0xA000) // VRAM (switchable bank 0-1 in CGB Mode)
    {
        memory[address] = v;
    }
    else if (address < 0xC000)
    {
        if (cartridge.ram_bank_enabled)
        {
            u16 bank_address = address - 0xA000;
            cartridge.ram_banks[bank_address + (cartridge.current_ram_bank * 0x2000)] = v;
        }
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
    }
    else
    {
        memory[address] = v;
    }
}

u8 
Memory_Bus::read_u8(u16 address) 
{
    if (address < 0x4000) // ROM bank 00
    {
        return cartridge.data[address];
    }
    else if (address < 0x8000) // ROM bank  1 -> N
    {
        u16 bank_address = address - 0x4000;
        return cartridge.data[bank_address + (cartridge.current_rom_bank * 0x4000)];
    }
    else if (address >= 0xA000 && address < 0xC000)
    {
        u16 bank_address = address - 0xA000;
        return cartridge.ram_banks[bank_address + (cartridge.current_ram_bank * 0x2000)];
    }
    else if (address == JOYPAD_REGISTER)
    {
        u8 req = memory[address];

        if ((req & JOYPAD_DIRECTION_REQUEST) == 0)
        {
            return (memory[address] & 0xF0) | ((joypad.state >> 4) & 0x0F);
        }
        else if ((req & JOYPAD_BUTTON_REQUEST) == 0)
        {
            return (memory[address] & 0xF0) | (joypad.state & 0x0F);
        }
        else
        {
            return memory[address];    
        }
    }

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