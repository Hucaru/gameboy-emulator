#pragma once

#include "types.h"
#include "win32.h"

// General
const u8 GAMEBOY_WIDTH = 160;
const u8 GAMEBOY_HEIGHT = 144;
const u8 RESOLUTION_UPSCALE = 4;

const u16 TILE_COUNT = 384;
const u16 TILE_WINDOW_WIDTH = 192;
const u16 TILE_WINDOW_HEIGHT = 128;

const u32 BACKGROUND_SIZE = 256 * 256;

const u16 INTERRUPT_FLAG = 0xFF0F;
const u16 INTERRUPT_ENABLE= 0xFFFF;
const u8 INTERRUPT_VBLANK = 0x01;
const u8 INTERRUPT_LCD = 0x01 << 1;
const u8 INTERRUPT_TIMER = 0x01 << 2;
const u8 INTERRUPT_SERIAL = 0x01 << 3;
const u8 INTERRUPT_JOYPAD = 0x01 << 4;

// Cartridge
const u16 CARTRIDGE_TITLE = 0x0134;
const u16 SOUND_CONTROLLER_ON_OF = 0xFF26;

// PPU
constexpr u16 LY_REGISTER = 0xFF44;

// Timers
constexpr u16 DIV = 0xFF04;
constexpr u16 TIMA = 0xFF05;
constexpr u16 TMA = 0xFF06;
constexpr u16 TAC = 0xFF07;

struct Cartridge
{
    char *path;
    u64 size;
    char *title;
    u8 old_license_code;
    u8 new_license_code[2];
};

struct PPU
{
    enum class Mode
    {
        OAM,
        PIXEL_TRANSFER,
        HBLANK,
        VBLANK,
    };

    struct OAM_Entry
    {
        union Sprite_Flags
        {
            u8 byte;
            struct
            {
                bool bit0 : 1; // CGB only
                bool bit1 : 1; // CGB only
                bool bit2 : 1; // CGB only
                bool bit3 : 1; // CGB only
                bool pallete_number : 1;
                bool x_flip : 1;
                bool y_flip : 1;
                bool obj_bg_priority : 1;
            };
        };

        i8 y_pos;
        i8 x_pos;
        u8 tile;
        Sprite_Flags properties;
    };

    u16 cycles;
    Mode mode;
    u8 pixel;

    OAM_Entry oam_object[40];
    bool valid_oam_objects[40];

    u32 tile_buffer[TILE_WINDOW_WIDTH * TILE_WINDOW_HEIGHT];
    u32 background_buffer[BACKGROUND_SIZE];
    
    u32 frame_buffer[GAMEBOY_WIDTH * GAMEBOY_HEIGHT];

    bool draw_frame;
    bool draw_tile_buffer;
};

struct CPU
{
    u8 registers[8]; // order: B C D E H L F A
    u16 pc;
    u16 sp;
    u8 remaining_cycles;
    bool interrupts;
};

struct Timers
{
    u32 tima_cycles_remaining;
    u32 div_cycles_remaining;
    u8 mode;
    bool enabled;
};

struct Memory_Bus
{
    u8 memory[0xFFFF];

    void write_u8(u16 address, u8 v);
    u8 read_u8(u16 address);

    void write_i8(u16 address, i8 v);
    i8 read_i8(u16 address);

    void write_u16(u16 address, u16 v);
    u16 read_u16(u16 address);

    Timers *timers;
};

void timers_cycle(Timers *timers, Memory_Bus *memory_bus);
void timers_init(Timers *timers, Memory_Bus *memory_bus);
void timers_set_tac(Timers *timers, u8 mode);

void cpu_init(CPU *cpu, Memory_Bus *memory_bus, bool cgb, u8 old_licence_code, u8 new_license_code[2]);
void cpu_cycle(CPU *cpu, Memory_Bus *memory_bus);

void ppu_init(PPU *ppu, Memory_Bus *memory_bus);
void ppu_cycle(PPU *ppu, Memory_Bus *memory_bus);

struct GameBoy
{
    Memory_Bus memory_bus;
    Cartridge cartridge;
    CPU cpu;
    Timers timers;
    PPU ppu;

    i64 time_since_last_sim;

    bool pause;
    bool step;

    Window *tile_window;
};

void perform_interrupt(Memory_Bus *memory_bus, u8 flag);