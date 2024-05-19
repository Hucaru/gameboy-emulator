#include "emulator.h"
#include "win32.h"

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <iostream>

const u8 nintendo_logo[48] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

bool load_cartridge(Cartridge *cartridge, Memory_Bus *memory_bus)
{
    printf("[Emulator] Loading ROM: %s\n", cartridge->path);
    u8 *rom = read_file(cartridge->path, &cartridge->size);
    std::memcpy(memory_bus->memory, rom, cartridge->size);

    if (memory_bus->memory == NULL)
    {
        message_box("Error", "Error loading rom");
        return false;
    }

    cartridge->title = reinterpret_cast<char*>(memory_bus->memory + 0x134);
    cartridge->old_license_code = memory_bus->memory[0x014B];

    if (cartridge->old_license_code == 0x33)
    {
        cartridge->new_license_code[0] = memory_bus->memory[0x0144];
        cartridge->new_license_code[0] = memory_bus->memory[0x0145];
    }

    u8 pixels[48][8];
    memset(&pixels, 0, 384);
   
    if (memcmp(&nintendo_logo, memory_bus->memory + 0x104, sizeof(nintendo_logo) / 2) != 0)
    {
        printf("[Emulator] Failed license check (nintendo logo)\n");
        return false;
    }

    return true;
}

bool 
init_application(int argc, char **argv, App *app) 
{
    if (argc < 2)
    {
        return false;
    }

    GameBoy* state = reinterpret_cast<GameBoy*>(malloc(sizeof(GameBoy)));
    
    state->cartridge.path = argv[1];
    if (!load_cartridge(&state->cartridge, &state->memory_bus))
    {
        return false;
    }

    state->pause = false;
    state->step = true;

    printf("[Emulator] STEP MODE: %s\n", state->step ? "enabled" : "disabled");

    cpu_init(&state->cpu, &state->memory_bus, false, state->cartridge.old_license_code, state->cartridge.new_license_code);
    ppu_init(&state->ppu, &state->memory_bus);

    state->tile_window = create_window(TILE_WINDOW_HEIGHT * RESOLUTION_UPSCALE, TILE_WINDOW_WIDTH * RESOLUTION_UPSCALE, "Tile VRAM");

    if (!state->tile_window)
    {
        return false;
    }

    app->window_title = reinterpret_cast<char*>(std::malloc(20));
    std::memset(app->window_title, 0, 20);
    app->window_title[0] = 'G';
    app->window_title[1] = 'a';
    app->window_title[2] = 'm';
    app->window_title[3] = 'e';
    app->window_title[4] = 'B';
    app->window_title[5] = 'o';
    app->window_title[6] = 'y';
    app->window_title[7] = ' ';
    app->window_title[8] = '|';
    app->window_title[9] = ' ';

    for (u8 i = 0; i < 9; ++i)
    {
        app->window_title[10 + i] = state->cartridge.title[i];
    }
    
    app->window_width = GAMEBOY_WIDTH * RESOLUTION_UPSCALE;
    app->window_height = GAMEBOY_HEIGHT * RESOLUTION_UPSCALE;
    app->application = state;
    app->running = true; 

    return app->running;
}

const i64 dmg_cycle_time_ns = 238;
const i64 simulation_period = 1.6e+7; // 16 ms

void
update_application(App *app, i64 delta_time) 
{
    GameBoy *gb = reinterpret_cast<GameBoy*>(app->application);

    if (gb->pause)
    {
        return;
    }

    gb->time_since_last_sim += delta_time;

    // Only emulate cycles once every frame i.e. 16ms/60Hz
    if (gb->time_since_last_sim < simulation_period)
    {
        return;
    }

    CPU *cpu = &gb->cpu;
    Timers *timers = &gb->timers;
    PPU *ppu = &gb->ppu;

    i64 cycles_to_simulate = gb->time_since_last_sim / dmg_cycle_time_ns;

    for (i64 i = 0; i < cycles_to_simulate; ++i)
    {
        cpu_cycle(cpu, &gb->memory_bus);
        timers_cycle(timers);
        ppu_cycle(ppu, &gb->memory_bus);
        
    }

    gb->time_since_last_sim = 0;

    if (gb->step)
    {
        gb->pause = true;
    }
}

void 
handle_input(App *app, Input_events *input_events) 
{
    GameBoy *gb = reinterpret_cast<GameBoy*>(app->application);

    if (check_input(input_events, Input_events::CODES::ESC) == Input_events::STATE::UP)
    {
        app->running = false;
    }
    else if (check_input(input_events, Input_events::CODES::SPACE) == Input_events::STATE::UP)
    {
        gb->pause = !gb->pause;
    }
    else if (check_input(input_events, Input_events::CODES::S) == Input_events::STATE::UP)
    {
        gb->step = !gb->step;
        printf("[Emulator] STEP MODE: %s\n", gb->step ? "enabled" : "disabled");

        if (!gb->step)
        {
            gb->pause = false;
        }
    }
}

void 
render_application(App *app, u32 *screen_pixels, i32 width, i32 height) 
{
    GameBoy *gb = reinterpret_cast<GameBoy*>(app->application);

    if (gb->ppu.draw_game_view)
    {
        gb->ppu.draw_game_view = false;

        std::memset(screen_pixels, 0x00FFFFFF, width * height * sizeof(u32));

        for (i32 i = 0; i < GAMEBOY_WIDTH; ++i)
        {
            for (i32 j = 0 ; j < GAMEBOY_HEIGHT ; ++j)
            {
                u32 pixel = gb->ppu.background_buffer[i + GAMEBOY_WIDTH * j];

                i32 adjusted_i = i * RESOLUTION_UPSCALE;

                for (i32 k = 0; k < RESOLUTION_UPSCALE; ++k)
                {
                    i32 adjusted_j = height - 1 - (j * RESOLUTION_UPSCALE) - k;
                    i32 index = adjusted_i + width * adjusted_j;
                    std::memset(screen_pixels + index , pixel, sizeof(u32) * RESOLUTION_UPSCALE);
                }
            }
        }
    }

    window_redraw(app->window_handle);

    if (gb->ppu.draw_tile_buffer)
    {
        gb->ppu.draw_tile_buffer = false;

        u32 tile_frame_width;
        u32 tile_frame_height;
        u32 *tile_pixels = window_get_frame(gb->tile_window, &tile_frame_width, &tile_frame_height);

        std::memset(tile_pixels, 0x00FFFFFF, tile_frame_width * tile_frame_height * sizeof(u32));

        for (i32 i = 0; i < TILE_WINDOW_WIDTH; ++i)
        {
            for (i32 j = TILE_WINDOW_HEIGHT - 1; j  >=0 ; --j)
            {
                // Need to flip in x to for the windows buffer
                u32 pixel = gb->ppu.tile_buffer[i + TILE_WINDOW_WIDTH * j];

                i32 adjusted_i = (TILE_WINDOW_WIDTH - 1 - i) * RESOLUTION_UPSCALE;

                for (i32 k = 0; k < RESOLUTION_UPSCALE; ++k)
                {
                    i32 adjusted_j = tile_frame_height - 1 - (j * RESOLUTION_UPSCALE) - k;
                    i32 index = adjusted_i + tile_frame_width * adjusted_j;
                    std::memset(tile_pixels + index , pixel, sizeof(u32) * RESOLUTION_UPSCALE);
                }
            }
        }

        window_redraw(gb->tile_window);
    }
}