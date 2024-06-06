#include "emulator.h"

#include <cstring>
#include <cstdio>

constexpr u16 OAM_CYCLES = 20 * 4;
constexpr u16 PIXEL_TRANSFER_CYCLES = 43 * 4;
constexpr u16 HBLANK_CYCLES = 51 * 4;
constexpr u16 VBLANK_CYCLES = 114 * 4;

constexpr u16 LCD_CONTROL_REGISTER = 0xFF40;
constexpr u16 LCD_STATUS_REGISTER = 0xFF41;

constexpr u8 LCD_STATUS_PPU_MODE_2 = 0x02; // OAM scan
constexpr u8 LCD_STATUS_PPU_MODE_3 = 0x03; // pixel transfer
constexpr u8 LCD_STATUS_PPU_MODE_0 = 0x0; // hblank
constexpr u8 LCD_STATUS_PPU_MODE_1 = 0x1; // vblank

constexpr u8 LCD_STATUS_LYC_EQ_LY = 0x04;
constexpr u8 LCD_STATUS_MODE_0_INT_SELECT = 0x08;
constexpr u8 LCD_STATUS_MODE_1_INT_SELECT = 0x10;
constexpr u8 LCD_STATUS_MODE_2_INT_SELECT = 0x20;
constexpr u8 LCD_STATUS_LYC_INT_SELECT = 0x40;

// Backgroyund viewport Y/X position
constexpr u16 SCY_REGISTER = 0xFF42;
constexpr u16 SCX_REGISTER = 0xFF43;

constexpr u16 LYC_REGISTER = 0xFF45;

// Window Y/X position +7
constexpr u16 WY_REGISTER = 0xFF4A;
constexpr u16 WX_REGISTER = 0xFF4B;

const u16 VRAM_TILE_DATA = 0x8000;

bool
lcd_enabled(Memory_Bus *memory_bus)
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x80;
}

u16 
bg_tile_start_address(Memory_Bus *memory_bus)
{
    return (memory_bus->read_u8(LCD_CONTROL_REGISTER) >> 3) & 0x01 ? 0x9C00 : 0x9800;
}

u16
tile_start_address(Memory_Bus *memory_bus)
{
    return (memory_bus->read_u8(LCD_CONTROL_REGISTER) >> 4) & 0x01 ? 0x9000 : 0x8000;
}

void 
draw_vram_tiles(PPU *ppu, Memory_Bus *memory_bus)
{
    static u32 pallet_colours[] = {0x00FFFFFF, 0x00FFAAAA, 0x00FF5555, 0x00000000};

    u16 start_address = VRAM_TILE_DATA;

    u16 tile = 0;
    for (u16 col = 0; col < TILE_WINDOW_WIDTH / 8; ++col)
    {
        for (u16 row = 0; row < TILE_WINDOW_HEIGHT / 8; ++row)
        {
            for (u16 tile_y = 0; tile_y < 16; tile_y += 2)
            {
                u16 stride = (TILE_WINDOW_WIDTH * (tile_y / 2)) + (TILE_WINDOW_WIDTH * row * 8);

                u8 hi = memory_bus->read_u8(start_address + (tile * 16) + tile_y);
                u8 lo = memory_bus->read_u8(start_address + (tile * 16) + tile_y + 1);

                for (i8 bit = 7; bit >= 0; --bit)
                {
                    u8 index = !!(hi & (1 << bit) << 1) | !!(lo & (1 << bit));
                    ppu->tile_buffer[(col * 8) + bit + stride] = pallet_colours[index];
                    
                }
            }

            ++tile;
        }
    }
}

const u16 oam_start_address = 0xFE00;

void
set_lcd_status_ppu_mode(Memory_Bus *memory_bus, u8 mode, u8 lcd_status)
{
    if ((lcd_status & 0x03) == mode)
    {
        return;
    }

    lcd_status &= 252;
    lcd_status |= mode;
    memory_bus->write_u8(LCD_STATUS_REGISTER, lcd_status);

    switch (mode)
    {
        case LCD_STATUS_PPU_MODE_0:
            if (lcd_status & 0x08)
            {
                perform_interrupt(memory_bus, INTERRUPT_LCD);
            }
            break;
        case LCD_STATUS_PPU_MODE_1:
            if (lcd_status & 0x10)
            {
                perform_interrupt(memory_bus, INTERRUPT_LCD);
            }
            break;
        case LCD_STATUS_PPU_MODE_2:
            if (lcd_status & 0x20)
            {
                perform_interrupt(memory_bus, INTERRUPT_LCD);
            }
            break;
    }
}

void 
ppu_cycle(PPU *ppu, Memory_Bus *memory_bus)
{
    u8 lcd_status = memory_bus->read_u8(LCD_STATUS_REGISTER);

    // if (!lcd_enabled(memory_bus))
    // {
    //     memory_bus->memory[LY_REGISTER] = 0;

    //     lcd_status &= 252;
    //     lcd_status |= 0x01;
    //     memory_bus->write_u8(LCD_STATUS_REGISTER, lcd_status);

    //     ppu->cycles = 0;
    //     return;
    // }

    ppu->cycles++;

    u8 current_line = memory_bus->memory[LY_REGISTER];

    if (current_line == memory_bus->read_u8(LYC_REGISTER))
    {
        lcd_status |= 0x02;
        memory_bus->write_u8(LCD_STATUS_REGISTER, lcd_status);

        if (lcd_status & 0x40)
        {
            perform_interrupt(memory_bus, INTERRUPT_LCD);
        }
    }
    else
    {
        lcd_status &= ~0x02;
        memory_bus->write_u8(LCD_STATUS_REGISTER, lcd_status);
    }

    switch (ppu->mode)
    {
        case PPU::Mode::OAM:
            set_lcd_status_ppu_mode(memory_bus, LCD_STATUS_PPU_MODE_2, lcd_status);

            if (ppu->cycles == OAM_CYCLES)
            {
                u8 sprite_height = 8; // TODO: Normal mode: 8, Tall-sprite-mode: 16
                u8 count = 0;

                for (u8 i = 0; i < 40; ++i)
                {
                    // TODO: Need to sub 16 from x and y or does that interfere with calculation below?
                    u8 offset = i * 4;
                    ppu->oam_object[i].y_pos = memory_bus->read_u8(oam_start_address + offset);
                    ppu->oam_object[i].x_pos = memory_bus->read_u8(oam_start_address + offset + 1);
                    ppu->oam_object[i].tile = memory_bus->read_u8(oam_start_address + offset + 2);
                    ppu->oam_object[i].properties.byte = memory_bus->read_u8(oam_start_address  + offset + 3);

                    if (count < 10 &&
                        ppu->oam_object[i].x_pos >= 0 &&
                        ppu->oam_object[i].y_pos >= (current_line + 16) &&
                        (ppu->oam_object[i].y_pos + sprite_height) > (current_line + 16))
                    {
                        ppu->valid_oam_objects[i] = true;
                        ++count;
                    }
                    else
                    {
                        ppu->valid_oam_objects[i] = false;
                    }
                }

                ppu->pixels_emitted = 0;
                u8 tile_line = current_line % 8;

                ppu->mode = PPU::Mode::PIXEL_TRANSFER;
            }
            break;
        case PPU::Mode::PIXEL_TRANSFER:
            set_lcd_status_ppu_mode(memory_bus, LCD_STATUS_PPU_MODE_3, lcd_status);

            // FIFO emit onto buffer
            ppu->pixels_emitted++;

            if (ppu->pixels_emitted == 160)
            {
                ppu->mode = PPU::Mode::HBLANK;
                ppu->pixels_emitted = 0;
            }
            break;
        case PPU::Mode::HBLANK:
            // Pixel transfer can take arbitary cycle amount but we know the maximum cycle count a line takes
            // which is the same amount of cycles vblank takes
            set_lcd_status_ppu_mode(memory_bus, LCD_STATUS_PPU_MODE_0, lcd_status);

            if (ppu->cycles == (HBLANK_CYCLES + PIXEL_TRANSFER_CYCLES + OAM_CYCLES))
            {
                if (current_line == 143)
                {
                    ppu->mode = PPU::Mode::VBLANK;
                    perform_interrupt(memory_bus, INTERRUPT_VBLANK);
                    ppu->draw_game_view = true;
                }
                else
                {
                    ppu->mode = PPU::Mode::OAM;
                }
                
                memory_bus->memory[LY_REGISTER]++;
                ppu->cycles = 0;
            }
            break;
        case PPU::Mode::VBLANK:
            set_lcd_status_ppu_mode(memory_bus, LCD_STATUS_PPU_MODE_1, lcd_status);

            if (ppu->cycles == VBLANK_CYCLES)
            {
                if (current_line == 153)
                {
                    memory_bus->memory[LY_REGISTER] = 0;
                    ppu->mode = PPU::Mode::OAM;

                    draw_vram_tiles(ppu, memory_bus);
                    ppu->draw_tile_buffer = true;
                }
                else
                {
                    memory_bus->memory[LY_REGISTER]++;
                }

                ppu->cycles = 0;
            }
            break;
    }
}

void ppu_init(PPU *ppu, Memory_Bus *memory_bus)
{
    printf("[PPU] reset state\n");
    ppu->draw_game_view = false;
    ppu->draw_tile_buffer = false;
}