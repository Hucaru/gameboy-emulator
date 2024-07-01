#pragma once
#include "types.h"

struct Input_events 
{
    enum class KEY_CODE : u8 
    {
        ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        ESC, SPACE, RETURN, BACK, LEFT, RIGHT, UP, DOWN
    };

    enum class KEY_STATE : u8 
    {
        DOWN = 1,
        UP = 2,
        HELD = 3
    };

    struct Gamepad 
    {
        u32 packet_number;
    };

    KEY_STATE keyboard[255];
};

Input_events::KEY_STATE check_keyboard(Input_events *events, Input_events::KEY_CODE);

struct Window;

struct App 
{
    char *window_title;
    Window *window_handle;
    int window_width;
    int window_height;
    void *application;
    bool running;
};

// These are the application callbacks
bool init_application(int argc, char **argv, App *app);
void update_application(App *app, i64 delta_time);
void handle_input(App *app, Input_events *input_events);
void render_application(App *app, u32 *pixels, int width, int height);

// Generic OS utility
Window * create_window(u32 height, u32 width, char *title);
u32 * window_get_frame(Window *window, u32 *width, u32 *height);
void window_redraw(Window *handle);
void update_window_title(void *handle, char *title);
u8 * read_file(char *filename, u64 *file_size);
void message_box(char *title, char *msg);