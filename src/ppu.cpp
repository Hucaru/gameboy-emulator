#include "emulator.h"

#include <cstring>
#include <cstdio>

constexpr u16 OAM_CYCLES = 20 * 4;
constexpr u16 PIXEL_TRANSFER_CYCLES = 43 * 4;
constexpr u16 HBLANK_CYCLES = 51 * 4;
constexpr u16 VBLANK_CYCLES = 114 * 4;

constexpr u8 LCD_STATUS_PPU_MODE_2 = 0x02; // OAM scan
constexpr u8 LCD_STATUS_PPU_MODE_3 = 0x03; // pixel transfer
constexpr u8 LCD_STATUS_PPU_MODE_0 = 0x00; // hblank
constexpr u8 LCD_STATUS_PPU_MODE_1 = 0x01; // vblank

constexpr u8 LCD_STATUS_LYC_EQ_LY = 0x04;
constexpr u8 LCD_STATUS_MODE_0_INT_SELECT = 0x08;
constexpr u8 LCD_STATUS_MODE_1_INT_SELECT = 0x10;
constexpr u8 LCD_STATUS_MODE_2_INT_SELECT = 0x20;
constexpr u8 LCD_STATUS_LYC_INT_SELECT = 0x40;

constexpr u16 LCD_CONTROL_REGISTER = 0xFF40;
constexpr u16 LCD_STATUS_REGISTER = 0xFF41;

// Backgroyund viewport Y/X position
constexpr u16 SCY_REGISTER = 0xFF42;
constexpr u16 SCX_REGISTER = 0xFF43;

constexpr u16 LYC_REGISTER = 0xFF45;

// Window Y/(X - 7) position
constexpr u16 WY_REGISTER = 0xFF4A;
constexpr u16 WX_REGISTER = 0xFF4B;

constexpr u16 VRAM_OBJ_DATA = 0x8000;

constexpr u16 OAM_START_ADDRESS = 0xFE00;
constexpr u16 SPRITE_DATA_START_ADDR = 0x8000;

// Every two bits map to a colour. Bit mapping:
// 0 - 1 -> id 00
// 2 - 3 -> id 01
// 4 - 5 -> id 10
// 6 - 7 -> id 11
constexpr u16 BG_COLOUR_PALETTE_ADDRESS = 0xFF47;
constexpr u16 SPRITE_COLOUR_PALETTE_ADDRESS[] = {0xFF48, 0xFF49};

constexpr u32 PALETTE_COLOURS[] = {0x00FFFFFF, 0x00FFAAAA, 0x00FF5555, 0x00000000};

constexpr u8 SPRITE_COUNT = 40;

enum PALLETE_COLOUR
{
    WHITE = 0,
    LIGHT_GRAY,
    DARK_GRAY,
    BLACK,
};

u32
determine_colour(Memory_Bus *memory_bus, u8 num, u16 address)
{
    u8 palette = memory_bus->read_u8(address);

    u8 hi, lo;

    switch(num)
    {
        case WHITE:
            hi = 1;
            lo = 0;
            break;
        case LIGHT_GRAY:
            hi = 3;
            lo = 2;
            break;
        case DARK_GRAY:
            hi = 5;
            lo = 4;
            break;
        case BLACK:
            hi = 7;
            lo = 6;
            break;
    }

    u8 colour = (((palette >> hi) & 0x01) <<  1) | ((palette >> lo) & 0x01);

    return PALETTE_COLOURS[colour];
}

bool
obj_enabled(Memory_Bus *memory_bus)
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x02;
}

u8
obj_height(Memory_Bus *memory_bus) // Width is always 8
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x04 ? 16 : 8;
}

u16
bg_tile_map_start_address(Memory_Bus *memory_bus)
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x08 ? 0x9C00 : 0x9800;
}

u16 
bg_window_tile_data_start_address(Memory_Bus *memory_bus, bool *signed_identifiers)
{
    if (memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x10)
    {
        *signed_identifiers = false;
        return 0x8000;
    }
    else
    {
        *signed_identifiers = true;
        return 0x8800;
    }
}

bool 
window_enabled(Memory_Bus *memory_bus)
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x20;
}

u16 
window_tile_map_start_address(Memory_Bus *memory_bus)
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x40 ? 0x9C00 : 0x9800;
}

bool
lcd_ppu_enabled(Memory_Bus *memory_bus)
{
    return memory_bus->read_u8(LCD_CONTROL_REGISTER) & 0x80;
}

void 
draw_vram_tiles(PPU *ppu, Memory_Bus *memory_bus)
{
    u16 start_address = VRAM_OBJ_DATA;

    u16 tile = 0;
    for (u16 col = 0; col < TILE_WINDOW_WIDTH / 8; ++col)
    {
        for (u16 row = 0; row < TILE_WINDOW_HEIGHT / 8; ++row)
        {
            for (u16 tile_y = 0; tile_y < 16; tile_y += 2)
            {
                u16 stride = (TILE_WINDOW_WIDTH * (tile_y / 2)) + (TILE_WINDOW_WIDTH * row * 8);

                u8 hi_byte = memory_bus->read_u8(start_address + (tile * 16) + tile_y);
                u8 lo_byte = memory_bus->read_u8(start_address + (tile * 16) + tile_y + 1);

                for (i8 bit = 7; bit >= 0; --bit)
                {
                    u8 hi = (hi_byte >> bit) & 0x01;
                    u8 lo = (lo_byte >> bit) & 0x01;
                    u8 index = (hi << 1) | lo;

                    ppu->tile_buffer[(col * 8) + bit + stride] = PALETTE_COLOURS[index];
                }
            }

            ++tile;
        }
    }
}

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

u32 
calculate_bg_pixel(PPU *ppu, Memory_Bus *memory_bus, u8 current_line)
{
    u8 scroll_x = memory_bus->read_u8(SCX_REGISTER);
    u8 scroll_y = memory_bus->read_u8(SCY_REGISTER);
    u8 window_x = memory_bus->read_u8(WX_REGISTER) - 7;
    u8 window_y = memory_bus->read_u8(WY_REGISTER);

    bool in_window_y = false;

    if(window_enabled(memory_bus))
    {
        if (window_y <= current_line)
        {
            in_window_y = true;
        }
    }

    bool tile_data_signed_id;
    u16 tile_data_start_addr = bg_window_tile_data_start_address(memory_bus, &tile_data_signed_id);

    u16 bg_data_start_addr = 0;

    if (in_window_y)
    {
        bg_data_start_addr = window_tile_map_start_address(memory_bus);
    }
    else
    {
        bg_data_start_addr = bg_tile_map_start_address(memory_bus);
    }

    u8 pos_y;

    if (in_window_y)
    {
        pos_y = current_line-window_y;
    }
    else
    {
        pos_y = current_line + scroll_y;
    }

    u16 tile_row = (pos_y / 8) * 32;
    
    u8 pos_x = ppu->pixel + scroll_x;

    if (in_window_y && (ppu->pixel >= window_x))
    {
        pos_x = ppu->pixel - window_x;
    }

    // which of the 32 horizontal tiles are we on
    u16 tile_column = pos_x / 8;
    i16 tile_address = bg_data_start_addr + tile_row + tile_column;

    u16 tile_data_addr;
    i16 tile_id;

    if (tile_data_signed_id)
    {
        tile_id = memory_bus->read_i8(tile_address);
        tile_data_addr = tile_data_start_addr + ((tile_id + 128) * 16);
    }
    else
    {
        tile_id = memory_bus->read_u8(tile_address);
        tile_data_addr = tile_data_start_addr + (tile_id * 16);
    }            

    u8 tile_vertical_line = pos_y % 8;
    tile_vertical_line *= 2; // each vertical line is 2 bytes

    u8 lo = memory_bus->read_u8(tile_data_addr + tile_vertical_line);
    u8 hi = memory_bus->read_u8(tile_data_addr + tile_vertical_line + 1);

    u8 colour_bit = pos_x % 8;
    // Pixel 0 is at bit 7, 1 is at bit 6 and so on. Therefore we need to adjust bit index
    colour_bit -= 7;
    colour_bit *= -1;

    // A single pixel colour is determined by the two bits in the two bytes making a 2 bit integer
    hi = (hi >> colour_bit) & 0x01;
    lo = (lo >> colour_bit) & 0x01;
    u8 colour_num = (hi << 1) | lo;

    return determine_colour(memory_bus, colour_num, BG_COLOUR_PALETTE_ADDRESS);
}

u32
calculate_sprite_pixel(PPU *ppu, Memory_Bus *memory_bus, u8 current_line, u32 pixel)
{
    // We go backwards as priority favours first valid
    for (i8 sprite = SPRITE_COUNT - 1; sprite >= 0; --sprite) 
    {
        if (!ppu->valid_oam_objects[sprite])
        {
            continue;
        }

        if (ppu->pixel > ppu->oam_object[sprite].x_pos + 8 || ppu->pixel < ppu->oam_object[sprite].x_pos)
        {
            continue;
        }

        u8 line = current_line - ppu->oam_object[sprite].y_pos;

        if (ppu->oam_object[sprite].properties.y_flip)
        {
            line -= obj_height(memory_bus);
            line *= -1;
        }

        line *= 2; // 2 bytes per line

        u16 sprite_data_addr = SPRITE_DATA_START_ADDR + (ppu->oam_object[sprite].tile * 16) + line;
        u8 lo = memory_bus->read_u8(sprite_data_addr);
        u8 hi = memory_bus->read_u8(sprite_data_addr + 1);

        u8 colour_bit = ppu->pixel - ppu->oam_object[sprite].x_pos;

        if (!ppu->oam_object[sprite].properties.x_flip)
        {
            colour_bit -= 7;
            colour_bit *= -1;
        }

        hi = (hi >> colour_bit) & 0x01;
        lo = (lo >> colour_bit) & 0x01;
        u8 colour_num = (hi << 1) | lo;

        if (colour_num == WHITE)
        {
            continue;
        }

        pixel = determine_colour(memory_bus, colour_num, ppu->oam_object[sprite].properties.pallete_number ? 0xFF49 : 0xFF48);
    }

    return pixel;
}

u32
calculate_pixel(PPU *ppu, Memory_Bus *memory_bus, u8 current_line)
{
    u32 bg_window_pixel = calculate_bg_pixel(ppu, memory_bus, current_line);
    return calculate_sprite_pixel(ppu, memory_bus, current_line, bg_window_pixel);
}

void 
ppu_cycle(PPU *ppu, Memory_Bus *memory_bus)
{
    u8 lcd_status = memory_bus->read_u8(LCD_STATUS_REGISTER);

    if (!lcd_ppu_enabled(memory_bus))
    {
        memory_bus->memory[LY_REGISTER] = 0;

        lcd_status &= 252;
        lcd_status |= 0x01;
        memory_bus->write_u8(LCD_STATUS_REGISTER, lcd_status);

        ppu->mode = PPU::Mode::PIXEL_TRANSFER;
        ppu->cycles = OAM_CYCLES;

        return;
    }

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
                u8 sprite_height = obj_height(memory_bus);
                u8 count = 0;
                
                for (u8 sprite = 0; sprite < SPRITE_COUNT; ++sprite)
                {
                    u8 offset = sprite * 4;
                    ppu->oam_object[sprite].y_pos = memory_bus->read_i8(OAM_START_ADDRESS + offset) - 16;
                    ppu->oam_object[sprite].x_pos = memory_bus->read_i8(OAM_START_ADDRESS + offset + 1) - 8;
                    ppu->oam_object[sprite].tile = memory_bus->read_u8(OAM_START_ADDRESS + offset + 2);
                    ppu->oam_object[sprite].properties.byte = memory_bus->read_u8(OAM_START_ADDRESS  + offset + 3);

                    i8 y_pos = ppu->oam_object[sprite].y_pos;
                    if (count < 10 && y_pos <= current_line && (y_pos + sprite_height) > current_line)
                    {
                        ppu->valid_oam_objects[sprite] = true;
                        ++count;
                    }
                    else
                    {
                        ppu->valid_oam_objects[sprite] = false;
                    }
                }

                ppu->mode = PPU::Mode::PIXEL_TRANSFER;
            }
            break;
        case PPU::Mode::PIXEL_TRANSFER:
            set_lcd_status_ppu_mode(memory_bus, LCD_STATUS_PPU_MODE_3, lcd_status);

            ppu->frame_buffer[ppu->pixel + (current_line * GAMEBOY_WIDTH)] = calculate_pixel(ppu, memory_bus, current_line);

            if (ppu->pixel == 159)
            {
                ppu->mode = PPU::Mode::HBLANK;
                ppu->pixel = 0;
            }
            else
            {
                ppu->pixel++;
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
                    
                    ppu->draw_frame = true;

                    draw_vram_tiles(ppu, memory_bus);
                    ppu->draw_tile_buffer = true;
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
    ppu->draw_frame = false;
    ppu->draw_tile_buffer = false;
}