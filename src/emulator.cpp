#include "emulator.h"
#include "platform.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

const u8 nintendo_logo[48] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
    0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
    0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

bool 
load_cartridge(Cartridge *cartridge, Memory_Bus *memory_bus)
{
    printf("[Emulator] Loading ROM: %s\n", cartridge->path);
    cartridge->data = read_file(cartridge->path, &cartridge->size);

    if (cartridge->data == NULL)
    {
        message_box("Error", "Error loading rom");
        return false;
    }

    if (cartridge->data[0x0143] == 0xC0)
    {
        message_box("Error", "CGB only ROM");
        return false;
    }

    cartridge->title = reinterpret_cast<char*>(cartridge->data + 0x0134);
    cartridge->old_license_code = cartridge->data[0x014B];

    if (cartridge->old_license_code == 0x33)
    {
        cartridge->new_license_code[0] = cartridge->data[0x0144];
        cartridge->new_license_code[0] = cartridge->data[0x0145];
    }
   
    if (memcmp(&nintendo_logo, cartridge->data + 0x104, sizeof(nintendo_logo) / 2) != 0)
    {
        printf("[Emulator] Failed license check (nintendo logo)\n");
    }

    switch (cartridge->data[0x0147])
    {
        case 0:
            printf("[Cartridge] ROM only\n");
            break;
        case 1:
        case 2:
        case 3:
            cartridge->mbc1 = true;
            printf("[Cartridge] MBC1\n");
            break;
        case 5:
        case 6:
            cartridge->mbc2 = true;
            printf("[Cartridge] MBC2\n");
            break;
        default:
            break;
    }

    cartridge->current_rom_bank = 1;

    memset(&cartridge->ram_banks, 0, sizeof(cartridge->ram_banks));
    cartridge->current_ram_bank = 0;

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
    
    state->memory_bus.cartridge.path = argv[1];
    if (!load_cartridge(&state->memory_bus.cartridge, &state->memory_bus))
    {
        return false;
    }

    state->pause = false;
    state->step = true;

    printf("[Emulator] STEP MODE: %s\n", state->step ? "enabled" : "disabled");

    state->memory_bus.timers = &state->timers;
    state->memory_bus.memory[JOYPAD_REGISTER] = 0x3F;
    state->memory_bus.joypad.state = 0xFF;
    state->memory_bus.joypad.button = false;
    state->memory_bus.joypad.direction = false;

    cpu_init(&state->cpu, &state->memory_bus, false, state->memory_bus.cartridge.old_license_code, state->memory_bus.cartridge.new_license_code);
    timers_init(&state->timers, &state->memory_bus);
    ppu_init(&state->ppu, &state->memory_bus);

    state->tile_window = create_window(TILE_WINDOW_HEIGHT * RESOLUTION_UPSCALE, TILE_WINDOW_WIDTH * RESOLUTION_UPSCALE, "VRAM");
    state->background_window = create_window(BACKGROUND_WINDOW_HEIGHT * BACKGROUND_WINDOW_RESOLUTION_SCALE, BACKGROUND_WINDOW_WIDTH * BACKGROUND_WINDOW_RESOLUTION_SCALE, "Background");

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
        app->window_title[10 + i] = state->memory_bus.cartridge.title[i];
    }
    
    app->window_width = GAMEBOY_WIDTH * RESOLUTION_UPSCALE;
    app->window_height = GAMEBOY_HEIGHT * RESOLUTION_UPSCALE;
    app->application = state;
    app->running = true; 

    return app->running;
}

const i64 dmg_cycle_time_ns = 238;
const i64 simulation_period = 1.6e+7 / 2; // 16 ms

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
    Memory_Bus *memory_bus = &gb->memory_bus;

    i64 cycles_to_simulate = gb->time_since_last_sim / dmg_cycle_time_ns;

    for (i64 i = 0; i < cycles_to_simulate * 2; ++i)
    {
        cpu_cycle(cpu, memory_bus);
        timers_cycle(timers, memory_bus);
        ppu_cycle(ppu, memory_bus);
        handle_input_event(memory_bus);
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

    if (keyboard_up(input_events, Input_events::KEY_CODE::ESC))
    {
        app->running = false;
    }
    else if (keyboard_up(input_events, Input_events::KEY_CODE::SPACE))
    {
        gb->pause = !gb->pause;
    }
    else if (keyboard_up(input_events, Input_events::KEY_CODE::S))
    {
        gb->step = !gb->step;
        printf("[Emulator] STEP MODE: %s\n", gb->step ? "enabled" : "disabled");

        if (!gb->step)
        {
            gb->pause = false;
        }
    }

    set_joypad_state(input_events, &gb->memory_bus.joypad);
}

void 
render_application(App *app, u32 *screen_pixels, i32 width, i32 height) 
{
    GameBoy *gb = reinterpret_cast<GameBoy*>(app->application);

    if (gb->ppu.draw_frame)
    {
        gb->ppu.draw_frame = false;

        std::memset(screen_pixels, 0x00FFFFFF, width * height * sizeof(u32));

        for (i32 i = 0; i < GAMEBOY_WIDTH; ++i)
        {
            for (i32 j = 0 ; j < GAMEBOY_HEIGHT ; ++j)
            {
                u32 pixel = gb->ppu.frame_buffer[i + GAMEBOY_WIDTH * j];

                // Need to flip in x to for the windows buffer
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
                u32 pixel = gb->ppu.tile_buffer[i + TILE_WINDOW_WIDTH * j];

                // Need to flip in x to for the windows buffer
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

    if (gb->ppu.draw_background_buffer)
    {
        window_redraw(gb->background_window);
    }
}

void
perform_interrupt(Memory_Bus *memory_bus, u8 flag)
{
    u8 interrupts = memory_bus->read_u8(INTERRUPT_FLAG);
    interrupts |= flag;
    memory_bus->write_u8(INTERRUPT_FLAG, interrupts);
}