#pragma once

#include "types.h"
#include "platform.h"

#include <deque>
#include <functional>

// General
constexpr u8 GAMEBOY_WIDTH = 160;
constexpr u8 GAMEBOY_HEIGHT = 144;
constexpr u8 RESOLUTION_UPSCALE = 4;

constexpr u16 TILE_COUNT = 384;
constexpr u16 TILE_WINDOW_WIDTH = 192;
constexpr u16 TILE_WINDOW_HEIGHT = 128;

constexpr u16 BACKGROUND_WINDOW_WIDTH = 256;
constexpr u16 BACKGROUND_WINDOW_HEIGHT = 256;
constexpr u8 BACKGROUND_WINDOW_RESOLUTION_SCALE = 2;

constexpr u16 INTERRUPT_FLAG = 0xFF0F;
constexpr u16 INTERRUPT_ENABLE= 0xFFFF;
constexpr u8 INTERRUPT_VBLANK = 0x01;
constexpr u8 INTERRUPT_LCD = 0x01 << 1;
constexpr u8 INTERRUPT_TIMER = 0x01 << 2;
constexpr u8 INTERRUPT_SERIAL = 0x01 << 3;
constexpr u8 INTERRUPT_JOYPAD = 0x01 << 4;

// Cartridge
constexpr u16 CARTRIDGE_TITLE = 0x0134;
constexpr u16 SOUND_CONTROLLER_ON_OF = 0xFF26;

// PPU
constexpr u16 LY_REGISTER = 0xFF44;

// Timers
constexpr u16 DIV = 0xFF04;
constexpr u16 TIMA = 0xFF05;
constexpr u16 TMA = 0xFF06;
constexpr u16 TAC = 0xFF07;

constexpr u16 DMA_REGISTER = 0xFF46;
constexpr u16 JOYPAD_REGISTER = 0xFF00;
constexpr u16 SERIAL_DATA_TRANSFER = 0xFF01;

constexpr u8 JOYPAD_DIRECTION_REQUEST = 0x10;
constexpr u8 JOYPAD_BUTTON_REQUEST = 0x20;

constexpr u64 CPU_PIPELINE_SIZE = 12;

struct Cartridge
{
    char *path;
    u64 size;
    u8 *data;

    char *title;

    bool mbc1;
    bool mbc2;

    bool rom_bank_enabled;
    bool ram_bank_enabled;
    u8 current_rom_bank;
    u8 current_ram_bank;

    u8 old_license_code;
    u8 new_license_code[2];

    u8 ram_banks[4 * 0x2000];
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
                bool pallete_number : 1; // 0 means palette addr is 0xFF48 otherwise 0xFF49
                bool x_flip : 1;
                bool y_flip : 1;
                bool obj_bg_priority : 1; // 0 above window and bg, 1 behind bg and window unless either has white colour
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
    u8 window_line_counter;
    bool window_used;

    OAM_Entry oam_object[40];
    bool valid_oam_objects[40];

    u32 tile_buffer[TILE_WINDOW_WIDTH * TILE_WINDOW_HEIGHT];
    u32 background_buffer[BACKGROUND_WINDOW_WIDTH * BACKGROUND_WINDOW_HEIGHT];
    
    u32 frame_buffer[GAMEBOY_WIDTH * GAMEBOY_HEIGHT];

    bool draw_frame;
    bool draw_tile_buffer;
};

struct Timers
{
    u32 tima_cycles_remaining;
    u32 div_cycles_remaining;
    u8 mode;
    bool enabled;
};

struct Joypad
{
    u8 state;
    bool button;
    bool direction;
};

struct Memory_Bus
{
    u8 memory[0xFFFF + 1];
    Cartridge cartridge;
    Joypad joypad;
    Timers *timers;

    void write_u8(u16 address, u8 v);
    u8 read_u8(u16 address);

    void write_i8(u16 address, i8 v);
    i8 read_i8(u16 address);

    void write_u16(u16 address, u16 v);
    u16 read_u16(u16 address);
};

struct CPU
{
    u8 registers[8]; // order: B C D E H L F A
    u16 pc;
    u16 sp;
    // Z80 temporary registers
    u8 w;
    u8 z;

    u8 tick;
    bool interrupt_master_enable;
    bool halted;
    bool extended;

    enum class STATE : u8 
    {
        READ_OPCODE, 
        EXECUTE_PIPELINE
    } state;

    struct Pipeline
    {
        
        std::function<void(CPU*,Memory_Bus*)> queue[CPU_PIPELINE_SIZE];
        u64 pos;
        u64 next_insert;

        void push_back(std::function<void(CPU*,Memory_Bus*)> item);
        std::function<void(CPU*,Memory_Bus*)> front();
        void pop_front();
        bool empty();
    };

    // std::deque<std::function<void(CPU*,Memory_Bus*)>> pipeline;
    Pipeline pipeline;
};

void perform_interrupt(Memory_Bus *memory_bus, u8 flag);

void memory_bus_init(Memory_Bus *memory_bus, Timers *timers);

void timers_cycle(Timers *timers, Memory_Bus *memory_bus);
void timers_init(Timers *timers, Memory_Bus *memory_bus);
void timers_set_tac(Timers *timers, u8 mode);

void cpu_init(CPU *cpu, Memory_Bus *memory_bus, bool cgb, u8 old_licence_code, u8 new_license_code[2]);
void cpu_cycle(CPU *cpu, Memory_Bus *memory_bus);

void ppu_init(PPU *ppu, Memory_Bus *memory_bus);
void ppu_cycle(PPU *ppu, Memory_Bus *memory_bus);

void handle_input_event(Memory_Bus *memory_bus);
void set_joypad_state(Input_events *events, Joypad *joypad);

struct GameBoy
{
    Memory_Bus memory_bus;
    CPU cpu;
    Timers timers;
    PPU ppu;

    i64 time_since_last_sim;

    bool pause;
    bool step;

    Window *tile_window;
    Window *background_window;
};